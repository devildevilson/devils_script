#include "devils_script/container.h"

#include <bit>
#include <format>
#include <algorithm>
#include <string>
#include "devils_script/context.h"
#include <iostream>
#include <utility>

namespace devils_script {

script_container::command::command() noexcept : fp(nullptr), arg(0) {}
script_container::command::command(function_t fp, bool arg) noexcept : fp(fp), arg(arg) {}
script_container::command::command(function_t fp, double arg) noexcept : fp(fp), arg(std::bit_cast<int64_t>(arg)) {}
script_container::command::command(function_t fp, int64_t arg) noexcept : fp(fp), arg(arg) {}

script_container::script_container() noexcept : prng_state(0x9e3779b97f4a7c15ULL) {}

container::container() noexcept = default;

void script_container::process(context* ctx) const {
  // Fail early (and with a clear message) when the context is too small for this script's parse-time
  // peak usage, instead of overflowing mid-run with a bare "Stack overflow". The frame bases are zero
  // at top level and bumped by `execute` for sub-scripts, so this also validates each nested frame.
  if (ctx->frame_base + max_stack > ctx->stack._data.size())
    error_at(ctx, std::format("context operand stack too small: needs {} slots at frame base {}, but the context provides only {}", max_stack, ctx->frame_base, ctx->stack._data.size()));
  if (ctx->saved_base + max_saved > ctx->saved_stack._data.size())
    error_at(ctx, std::format("context saved-value stack too small: needs {} slots at base {}, but the context provides only {}", max_saved, ctx->saved_base, ctx->saved_stack._data.size()));

  const script_container* prev_script = ctx->current_script;
  ctx->current_script = this;
  for (; ctx->current_index < cmds.size(); ++ctx->current_index) {
    const auto& cmd = cmds[ctx->current_index];
    std::invoke(cmd.fp, cmd.arg, ctx, this);
  }
  ctx->current_script = prev_script;
}

void script_container::shrink_to_fit() {
  cmds.shrink_to_fit();
  locs.shrink_to_fit();
  args.shrink_to_fit();
  saved.shrink_to_fit();
  lists.shrink_to_fit();
  string_pool.shrink_to_fit();
}

script_container container::strip_description() const& {
  return static_cast<const script_container&>(*this);
}

script_container container::strip_description() && {
  return std::move(static_cast<script_container&>(*this));
}

void container::shrink_to_fit() {
  script_container::shrink_to_fit();
  cmd_node.shrink_to_fit();
  block_descs.shrink_to_fit();
  description_cmd_index_offsets.shrink_to_fit();
  description_cmd_index_nodes.shrink_to_fit();
}

void shrink_to_fit(std::span<script_container> scripts) {
  for (auto& script : scripts) script.shrink_to_fit();
}

void shrink_to_fit(std::span<container> scripts) {
  for (auto& script : scripts) script.shrink_to_fit();
}

void container::make_table(context* ctx, std::vector<std::tuple<any_stack, any_stack>>& table) const {
  table.assign(block_descs.size(), {});
  describe(ctx, [&](const description_entry& entry) {
    table[entry.node] = std::make_tuple(entry.value, entry.scope);
  });
}

void container::make_table(context* ctx, node_view& viewer) const {
  make_table(ctx, viewer.table);
}

void container::build_description_index() {
  // Pad source locations for any commands not stamped during emission (e.g. the trailing
  // pushreturn or internal control-flow jumps emitted outside a described node).
  if (locs.size() < cmds.size()) locs.resize(cmds.size(), src_loc{ 0, 0 });

  // Link each command to the innermost description node whose command range [cmd_start, cmd_index]
  // encloses it. This recovers the command's opcode/function name (the construct that produced it)
  // for both result commands and intermediate ones (e.g. scope-chain calls, unlimited-fold ops).
  cmd_node.assign(cmds.size(), SIZE_MAX);
  std::vector<size_t> best_span(cmds.size(), SIZE_MAX);
  for (size_t node = 0; node < block_descs.size(); ++node) {
    const auto& bd = block_descs[node];
    if (bd.cmd_start > bd.cmd_index) continue;
    const size_t span = bd.cmd_index - bd.cmd_start;
    for (size_t i = bd.cmd_start; i <= bd.cmd_index && i < cmds.size(); ++i)
      if (span < best_span[i]) { best_span[i] = span; cmd_node[i] = node; }
  }

  description_cmd_index_offsets.assign(cmds.size() + 1, 0);
  description_cmd_index_nodes.clear();

  for (size_t node = 0; node < block_descs.size(); ++node) {
    const auto& bd = block_descs[node];
    if (bd.cmd_start >= cmds.size()) continue;
    if (bd.cmd_start == bd.cmd_index) continue;
    description_cmd_index_offsets[bd.cmd_start + 1] += 1;
  }

  for (size_t i = 1; i < description_cmd_index_offsets.size(); ++i)
    description_cmd_index_offsets[i] += description_cmd_index_offsets[i - 1];

  description_cmd_index_nodes.resize(description_cmd_index_offsets.back());
  auto cursor = description_cmd_index_offsets;
  for (size_t node = 0; node < block_descs.size(); ++node) {
    const auto& bd = block_descs[node];
    if (bd.cmd_start >= cmds.size()) continue;
    if (bd.cmd_start == bd.cmd_index) continue;
    description_cmd_index_nodes[cursor[bd.cmd_start]++] = node;
  }
}

void container::describe(context* ctx, const description_callback_t& fn) const {
  struct eval_entry {
    bool visited = false;
    bool has_value = false;
    any_stack value;
    any_stack scope;
    std::string error;
  };

  std::vector<eval_entry> table(block_descs.size());
  context work = *ctx;
  work.current_index = 0;

  // Effect-ness is a property of the producing node (a void function/iterator): only its result
  // command is the effect to skip — argument pushes inside it must still be evaluated.
  const auto cmd_is_effect = [&](const size_t i) {
    const size_t n = i < cmd_node.size() ? cmd_node[i] : SIZE_MAX;
    return n != SIZE_MAX && block_descs[n].effect && block_descs[n].cmd_index == i;
  };

  size_t counter = 0;
  for (size_t i = 0; i < cmds.size(); ++i) {
    const bool reached = i >= work.current_index;
    bool ok = reached;
    std::string error;

    if (reached && !cmd_is_effect(i)) {
      context before = work;
      try {
        const auto& cmd = cmds[i];
        std::invoke(cmd.fp, cmd.arg, &work, this);
        work.current_index += 1;
      } catch (const std::exception& e) {
        ok = false;
        error = e.what();
        work = std::move(before);
        work.current_index = i + 1;
      } catch (...) {
        ok = false;
        error = "unknown evaluation error";
        work = std::move(before);
        work.current_index = i + 1;
      }
    } else if (reached) {
      work.current_index += 1;
    }

    const auto record = [&](const size_t node) {
      auto& entry = table[node];
      if (entry.has_value) return;
      entry.visited = reached;
      if (ok && reached && !cmd_is_effect(i) && work.stack.size() > 0) {
        entry.has_value = true;
        entry.value = work.stack.get<any_stack>();
        const auto si = block_descs[node].scope_index;
        if (si >= 0 && !work.stack.invalid(si)) entry.scope = work.stack.get<any_stack>(si);
      } else if (!error.empty()) {
        entry.error = error;
      }
    };

    // Command name for matching: basic ops from the fp, user functions from the producing node.
    const auto cmd_name = [&](const size_t k) -> std::string_view {
      const basicf bf = find_basicf_by_fp(cmds[k].fp);
      if (bf != basicf::invalid) return to_string(bf);
      return k < cmd_node.size() && cmd_node[k] != SIZE_MAX ? get_string(block_descs[cmd_node[k]].name) : std::string_view();
    };

    if (description_cmd_index_offsets.size() == cmds.size() + 1) {
      for (size_t offset = description_cmd_index_offsets[i]; offset < description_cmd_index_offsets[i + 1]; ++offset) {
        const size_t node = description_cmd_index_nodes[offset];
        if (get_string(block_descs[node].name) != cmd_name(i)) continue;
        record(node);
      }
    } else {
      for (size_t node = 0; node < block_descs.size(); ++node) {
        const auto& bd = block_descs[node];
        if (bd.cmd_start != i || bd.cmd_start == bd.cmd_index) continue;
        if (get_string(bd.name) != cmd_name(i)) continue;
        record(node);
      }
    }

    while (counter < block_descs.size() && i == block_descs[counter].cmd_index) {
      record(counter);
      counter += 1;
    }
  }

  const auto traverse = [&](const auto& self, const size_t offset, const size_t nest_level) -> bool {
    const auto& desc = block_descs[offset];
    const auto& val = table[offset];

    const auto state = desc.placeholder
      ? description_value_state::placeholder
      : (val.has_value ? description_value_state::value : description_value_state::unavailable);

    description_entry entry{
      offset,
      get_string(desc.name),
      get_string(desc.custom_description),
      nest_level,
      desc.kind,
      state,
      val.value,
      val.scope,
      val.error
    };

    std::invoke(fn, entry);
    if (!entry.custom_description.empty()) return true;

    const size_t stack_start = offset >= desc.size ? offset - desc.size + 1 : 0;
    std::vector<size_t> children;
    size_t child_offset = 1;
    while (child_offset < desc.size) {
      const size_t child = offset - child_offset;
      if (child < stack_start) break;
      children.push_back(child);
      child_offset += block_descs[child].size;
    }

    std::reverse(children.begin(), children.end());
    for (const size_t child : children) self(self, child, nest_level + 1);
    return true;
  };

  if (!block_descs.empty()) traverse(traverse, block_descs.size() - 1, 0);
}

std::string_view script_container::get_string(const size_t start, const size_t count) const {
  if (count == SIZE_MAX) return to_string(static_cast<basicf>(start));

  if (start + count > string_pool.size()) return std::string_view();
  return std::string_view(string_pool).substr(start, count);
}

std::string_view script_container::get_string(const string_ref& str) const {
  if (str.count == SIZE_MAX) return to_string(static_cast<basicf>(str.start));
  return get_string(str.start, str.count);
}

std::string_view script_container::get_name() const {
  return get_string(name);
}

script_container::src_loc script_container::loc_at(const context* ctx) const {
  const size_t i = ctx->current_index;
  return i < locs.size() ? locs[i] : src_loc{ 0, 0 };
}

void script_container::error_at(const context* ctx, const std::string_view& msg) const {
  const src_loc loc = loc_at(ctx);
  throw std::runtime_error(std::format("script '{}' @ {}:{}: {}", get_name(), loc.line, loc.column, msg));
}

std::string_view container::get_command_name(const size_t index) const {
  // Names are not stored per command: basic ops resolve from their function pointer, user functions
  // and builtins (assert/trace/...) from their producing description node via cmd_node.
  if (index >= cmds.size()) return std::string_view();
  const basicf bf = find_basicf_by_fp(cmds[index].fp);
  if (bf != basicf::invalid) return to_string(bf);
  return index < cmd_node.size() && cmd_node[index] != SIZE_MAX ? get_string(block_descs[cmd_node[index]].name) : std::string_view();
}

std::string disassemble(const container& scr) {
  std::string out;
  for (size_t i = 0; i < scr.cmds.size(); ++i) {
    const int64_t arg = scr.cmds[i].arg;

    // Opcode identity is recovered from the function pointer (basic ops) or, for user functions,
    // from the producing description node (via cmd_node) — never stored per command.
    const basicf op = find_basicf_by_fp(scr.cmds[i].fp);
    std::string_view name;
    if (op != basicf::invalid) name = to_string(op);
    else if (i < scr.cmd_node.size() && scr.cmd_node[i] != SIZE_MAX) name = scr.get_string(scr.block_descs[scr.cmd_node[i]].name);

    std::string argstr;
    switch (op) {
      case basicf::jump: case basicf::condjump: case basicf::condjump_get:
      case basicf::condjumpt_get: case basicf::andjump: case basicf::orjump:
        argstr = std::format(" -> {}", arg); break;
      case basicf::pushvalue:
        argstr = std::format(" {}", std::bit_cast<double>(arg)); break;
      case basicf::pushbool:
        argstr = std::format(" {}", arg != 0 ? "true" : "false"); break;
      default:
        if (arg != 0) argstr = std::format(" {}", arg);
        break;
    }

    out += std::format("{:>3}: {}{}\n", i, name, argstr);
  }
  return out;
}

size_t script_container::find_arg(const std::string_view& name) const {
  size_t i = 0;
  for (; i < args.size() && get_string(args[i].name) != name; ++i) {}
  return i < args.size() ? i : SIZE_MAX;
}

std::string_view script_container::get_arg_name(const size_t index) const {
  if (index >= args.size()) return std::string_view();
  return get_string(args[index].name);
}

size_t script_container::find_saved(const std::string_view& name) const {
  size_t i = 0;
  for (; i < saved.size() && get_string(saved[i].name) != name; ++i) {}
  return i < saved.size() ? i : SIZE_MAX;
}

std::string_view script_container::get_saved_name(const size_t index) const {
  if (index >= saved.size()) return std::string_view();
  return get_string(saved[index].name);
}

size_t script_container::find_list(const std::string_view& name) const {
  size_t i = 0;
  for (; i < lists.size() && get_string(lists[i].name) != name; ++i) {}
  return i < lists.size() ? i : SIZE_MAX;
}

std::string_view script_container::get_list_name(const size_t index) const {
  if (index >= lists.size()) return std::string_view();
  return get_string(lists[index].name);
}

container_view::container_view(const script_container* scr, const size_t start, const size_t end) noexcept :
  scr(scr), start(start), end(end)
{}

void container_view::process(context* ctx) const {
  const script_container* prev_script = ctx->current_script;
  ctx->current_script = scr;
  for (ctx->current_index = start; ctx->current_index < end; ++ctx->current_index) {
    const auto& cmd = scr->cmds[ctx->current_index];
    std::invoke(cmd.fp, cmd.arg, ctx, scr);
  }
  ctx->current_script = prev_script;
}

std::string_view container_view::get_string(const size_t start, const size_t count) const {
  return scr->get_string(start, count);
}

bool node_view::traverse(const container* scr, const size_t offset, const size_t nest_level, const fn_t& fn) {
  const auto& d = scr->block_descs;

  const auto& [val, scope] = table[offset];
  const bool fnret = std::invoke(fn, scr->get_string(d[offset].name), scr->get_string(d[offset].custom_description), nest_level, val, scope);
  if (!fnret) return false;

  const size_t cur_stack_size = stack.size();

  size_t curoffset = 1;
  while (curoffset < d[offset].size) {
    const size_t start_index = offset - curoffset;
    stack.push_back(start_index);
    curoffset += d[offset - curoffset].size;
  }

  std::reverse(stack.begin()+cur_stack_size, stack.end());

  bool ret = true;
  const size_t size_part = stack.size();
  for (size_t i = cur_stack_size; i < size_part; ++i) {
    const auto& newoffset = stack[i];
    ret = traverse(scr, newoffset, nest_level+1, fn);
  }

  stack.resize(cur_stack_size);

  return ret;
}

bool node_view::traverse(const container* scr, const fn_t& fn) {
  const auto ret = traverse(scr, scr->block_descs.size()-1, 0, fn);
  stack.clear();
  return ret;
}

}
