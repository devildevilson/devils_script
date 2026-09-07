#include "devils_script/system.h"

#include <algorithm>

// Post-codegen peephole over the compiled command stream.
//
// Codegen emits one instruction per language construct, which leaves short, mechanical sequences
// behind: a scope unwind is a run of single-slot `erase`s, a `ctx:saved:x` read pushes a context
// object nothing reads, and combinator lowering leaves jumps that land on the next instruction.
// This pass rewrites those sequences and then compacts the array.
//
// Compacting is the hard part, because a command index is stored in more places than the jump
// arguments: iterator callback ranges, the nullable scope guard's packed target, list_pipeline's
// relative section bounds and the description tree's command ranges all point into `cmds`. Some of
// those fields cannot be recognized after the fact - an iterator range slot holds a `jump` function
// pointer but is data, and the scope guard hides its target in half an argument behind a template
// instantiation no opcode table knows. Codegen therefore records the data slots and the index fields
// into parse_context as it writes them, and this pass is the only place that renumbers them.
//
// Every transform is optional: with optimizations off the literal lowering runs unchanged, which is
// what the optimizer is differentially tested against.

namespace devils_script {

namespace {

int32_t low_half(const int64_t packed) noexcept { return std::get<0>(unpack2(packed)); }
int32_t high_half(const int64_t packed) noexcept { return std::get<1>(unpack2(packed)); }

}  // namespace

void system::record_cmd_index(parse_ctx* ctx, const size_t cmd, const uint8_t half, const size_t base, const bool zero_is_absent) const {
  if (!optimize) return;
  ctx->cmd_index_fields.push_back(parse_ctx::cmd_index_field{ cmd, base, half, zero_is_absent });
}

void system::record_data_slots(parse_ctx* ctx, const size_t first, const size_t count) const {
  if (!optimize) return;
  for (size_t i = 0; i < count; ++i) ctx->data_slots.push_back(first + i);
}

void system::record_dead_context_push(parse_ctx* ctx, const size_t cmd, const size_t unwind, const int64_t scope_slot) const {
  if (!optimize) return;
  ctx->dead_context_pushes.push_back(parse_ctx::dead_context_push{ cmd, unwind, scope_slot });
}

void system::optimize_commands(parse_ctx& pctx, container& scr) const {
  if (!optimize) return;
  const size_t count = scr.cmds.size();
  if (count == 0) return;

  std::vector<uint8_t> is_data(count, 0);
  for (const size_t slot : pctx.data_slots) {
    if (slot < count) is_data[slot] = 1;
  }

  // Reads the command index a recorded field currently holds, or nothing when the field says "no
  // such section". Range bounds are exclusive, so `count` itself is a legal value.
  const auto read_field = [&](const parse_ctx::cmd_index_field& f) -> std::optional<size_t> {
    if (f.cmd >= count) return std::nullopt;
    const int64_t arg = scr.cmds[f.cmd].arg;
    const int64_t stored = f.half == 0 ? arg : (f.half == 1 ? int64_t(low_half(arg)) : int64_t(high_half(arg)));
    if (f.zero_is_absent && stored == 0) return std::nullopt;
    const int64_t absolute = f.base == SIZE_MAX ? stored : stored + int64_t(f.base);
    if (absolute < 0 || size_t(absolute) > count) return std::nullopt;
    return size_t(absolute);
  };

  // Commands something can enter at: a branch target, a callback-section bound, the script entry.
  // A transform may drop one of those only if whatever pointed at it ends up in the same place.
  std::vector<uint8_t> is_entry(count + 1, 0);
  is_entry[0] = 1;
  is_entry[count] = 1;
  for (const auto& field : pctx.cmd_index_fields) {
    if (const auto target = read_field(field)) is_entry[*target] = 1;
  }
  // An iterator's first callback starts right after its block of range slots; every later section
  // starts at the previous section's end, which the loop above already marked.
  for (const size_t slot : pctx.data_slots) {
    if (slot + 1 <= count) is_entry[slot + 1] = 1;
  }

  // describe() samples a node's value and scope right after the command `block_description::cmd_index`
  // names, and starts matching a node at `cmd_start`. Those commands are observation points of a
  // public API, not just instructions: a scope unwind's individual `erase`s are exactly where the
  // enclosing nodes report their scope. Removing one moves the sample and changes what describe()
  // reports, so an observed command stays put even when it would otherwise fuse away.
  std::vector<uint8_t> is_observed(count, 0);
  for (const auto& bd : scr.block_descs) {
    if (bd.cmd_start < count) is_observed[bd.cmd_start] = 1;
    if (bd.cmd_index < count) is_observed[bd.cmd_index] = 1;
  }

  std::vector<uint8_t> keep(count, 1);
  const auto opcode = [&](const size_t i) {
    return is_data[i] ? basicf::invalid : find_basicf_by_fp(scr.cmds[i].fp);
  };

  // --- a context push nothing reads -----------------------------------------
  // `pushctxvalue`, `pushargvalue` and `pushlist` take the context out of `context*` itself, so a
  // block that compiled to `context; <one of those>; erase` never uses the pushed object. Codegen
  // recorded the shape when it produced one; all that is left is to check nothing branches inside.
  //
  // The description tree does observe these commands - the `ctx` node starts at the push and ends at
  // the unwind - but relocation lands both ends on the surviving read, which reports the same value.
  // What cannot survive is a node naming the pushed object as its *scope*: that slot is gone, and
  // reading the slot number again would report whatever moved into it. Such a node loses its scope,
  // which is what actually happened to it.
  for (const auto& push : pctx.dead_context_pushes) {
    const size_t cmd = push.cmd;
    const size_t unwind = push.unwind;
    if (unwind >= count || unwind <= cmd) continue;
    if (!keep[cmd] || !keep[unwind]) continue;
    if (opcode(unwind) != basicf::erase) continue;
    // Anything that branches into the block would arrive with the push already skipped, so leave a
    // block with a foreign entry point alone.
    bool entered = false;
    for (size_t i = cmd + 1; i <= unwind && !entered; ++i) entered = is_entry[i];
    if (entered) continue;

    keep[cmd] = 0;
    keep[unwind] = 0;
    for (auto& bd : scr.block_descs) {
      if (bd.scope_index != push.scope_slot) continue;
      if (bd.cmd_start < cmd || bd.cmd_index > unwind) continue;
      bd.scope_index = -1;
    }
  }

  // --- fuse scope unwinds ---------------------------------------------------
  // A run of single-slot `erase`s removes one stack slot per dispatch. When the slots it removes are
  // contiguous - a repeated index, or the descending chain a scope-path exit emits - the whole run
  // is one `erase_range`, which shifts the stack tail once instead of once per slot.
  for (size_t i = 0; i < count;) {
    // The first command of the run is observed too: it stays, but after fusing it performs the whole
    // unwind, so whatever describe() would have sampled right after it changes as well.
    if (!keep[i] || is_observed[i] || opcode(i) != basicf::erase) { ++i; continue; }

    size_t run_end = i + 1;
    while (run_end < count && keep[run_end] && !is_entry[run_end] && !is_observed[run_end] && opcode(run_end) == basicf::erase) ++run_end;

    const int64_t first_arg = scr.cmds[i].arg;
    const bool descending = run_end > i + 1 && scr.cmds[i + 1].arg == first_arg - 1;
    size_t fused = 1;
    while (i + fused < run_end && scr.cmds[i + fused].arg == (descending ? first_arg - int64_t(fused) : first_arg)) ++fused;

    const int64_t lowest = descending ? first_arg - int64_t(fused) + 1 : first_arg;
    if (fused > 1 && lowest >= 0 && lowest <= INT32_MAX && fused <= size_t(INT32_MAX)) {
      scr.cmds[i] = container::command(&erase_range, pack2(int32_t(lowest), int32_t(fused)));
      for (size_t j = i + 1; j < i + fused; ++j) keep[j] = 0;
    }
    i += fused;
  }

  // --- jumps that fall through ----------------------------------------------
  // An unconditional jump onto the next surviving command. Dropping it is safe even when something
  // branches to the jump itself, because the relocation below sends those references to the command
  // the jump would have reached. Conditional jumps are left alone: they also consume a value.
  for (size_t i = 0; i < count; ++i) {
    if (!keep[i] || is_data[i] || is_observed[i] || opcode(i) != basicf::jump) continue;
    const int64_t target = scr.cmds[i].arg;
    if (target < 0 || size_t(target) > count) continue;
    size_t next = i + 1;
    while (next < count && !keep[next]) ++next;
    if (size_t(target) == next) keep[i] = 0;
  }

  // --- jump threading -------------------------------------------------------
  // A branch whose target is itself an unconditional jump can go straight to that jump's target.
  // Nothing is removed, so this costs no relocation - but the skipped jump stops being executed, and
  // describe() walks the stream by the same targets, so an observed jump is left in the chain.
  const auto is_branch = [&](const size_t i) {
    if (is_data[i]) return false;
    switch (find_basicf_by_fp(scr.cmds[i].fp)) {
      case basicf::jump:
      case basicf::condjump:
      case basicf::condjump_get:
      case basicf::condjumpt_get:
      case basicf::andjump:
      case basicf::orjump: return true;
      default: return false;
    }
  };
  for (const auto& field : pctx.cmd_index_fields) {
    if (field.half != 0 || field.base != SIZE_MAX || field.cmd >= count) continue;
    if (!keep[field.cmd] || !is_branch(field.cmd)) continue;
    int64_t target = scr.cmds[field.cmd].arg;
    // Bounded by the command count, so a cycle of jumps cannot spin here.
    for (size_t step = 0; step < count; ++step) {
      if (target < 0 || size_t(target) >= count) break;
      const size_t t = size_t(target);
      if (!keep[t] || is_data[t] || is_observed[t] || opcode(t) != basicf::jump) break;
      const int64_t next = scr.cmds[t].arg;
      if (next == target) break;
      target = next;
    }
    scr.cmds[field.cmd].arg = target;
  }

  // --- relocation -----------------------------------------------------------

  size_t survivors = 0;
  for (size_t i = 0; i < count; ++i) survivors += keep[i];
  if (survivors == count) return;


  // `next_at` sends an index to the first surviving command at or after it, which is what a branch
  // target or an exclusive range bound needs. `last_at` sends it to the last surviving command at or
  // before it, for the inclusive end a description node stores.
  std::vector<size_t> next_at(count + 1, 0);
  std::vector<size_t> last_at(count + 1, 0);
  {
    size_t emitted = 0;
    for (size_t i = 0; i < count; ++i) {
      next_at[i] = emitted;
      emitted += keep[i];
      last_at[i] = emitted == 0 ? 0 : emitted - 1;
    }
    next_at[count] = emitted;
    last_at[count] = emitted == 0 ? 0 : emitted - 1;
  }
  const auto reloc = [&](const size_t i) { return i <= count ? next_at[i] : i; };
  const auto reloc_last = [&](const size_t i) { return i <= count ? last_at[i] : i; };

  // Read every field before anything moves, then write it back into its new slot.
  std::vector<std::optional<size_t>> targets;
  targets.reserve(pctx.cmd_index_fields.size());
  for (const auto& field : pctx.cmd_index_fields) targets.push_back(read_field(field));

  std::vector<container::command> cmds;
  std::vector<script_container::src_loc> locs;
  cmds.reserve(survivors);
  locs.reserve(std::min(scr.locs.size(), survivors));
  for (size_t i = 0; i < count; ++i) {
    if (!keep[i]) continue;
    cmds.push_back(scr.cmds[i]);
    if (i < scr.locs.size()) locs.push_back(scr.locs[i]);
  }
  scr.cmds = std::move(cmds);
  scr.locs = std::move(locs);

  std::vector<parse_ctx::cmd_index_field> fields;
  fields.reserve(pctx.cmd_index_fields.size());
  for (size_t f = 0; f < pctx.cmd_index_fields.size(); ++f) {
    auto field = pctx.cmd_index_fields[f];
    if (field.cmd >= count || !keep[field.cmd]) continue;  // the field's own command is gone
    const size_t slot = reloc(field.cmd);
    const size_t base = field.base == SIZE_MAX ? SIZE_MAX : reloc(field.base);
    if (targets[f].has_value()) {
      const int64_t stored = base == SIZE_MAX
        ? int64_t(reloc(*targets[f]))
        : int64_t(reloc(*targets[f])) - int64_t(base);
      auto& arg = scr.cmds[slot].arg;
      if (field.half == 0) arg = stored;
      else if (field.half == 1) arg = pack2(int32_t(stored), high_half(arg));
      else arg = pack2(low_half(arg), int32_t(stored));
    }
    field.cmd = slot;
    field.base = base;
    fields.push_back(field);
  }
  pctx.cmd_index_fields = std::move(fields);

  std::vector<size_t> slots;
  slots.reserve(pctx.data_slots.size());
  for (const size_t slot : pctx.data_slots) {
    if (slot < count && keep[slot]) slots.push_back(reloc(slot));
  }
  pctx.data_slots = std::move(slots);

  std::vector<parse_ctx::dead_context_push> pushes;
  for (const auto& push : pctx.dead_context_pushes) {
    if (push.cmd < count && keep[push.cmd]) pushes.push_back(parse_ctx::dead_context_push{ reloc(push.cmd), reloc(push.unwind), push.scope_slot });
  }
  pctx.dead_context_pushes = std::move(pushes);

  for (auto& bd : scr.block_descs) {
    bd.cmd_start = reloc(bd.cmd_start);
    bd.cmd_index = reloc_last(bd.cmd_index);
    bd.cmd_end = reloc(bd.cmd_end);
  }
}

}  // namespace devils_script
