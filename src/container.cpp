#include "devils_script/container.h"

#include <bit>
#include <format>
#include <algorithm>
#include <string>
#include "devils_script/context.h"
#include <iostream>

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

container::command::command() noexcept : fp(nullptr), arg(0) {}
container::command::command(function_t fp, bool arg) noexcept : fp(fp), arg(arg) {}
container::command::command(function_t fp, double arg) noexcept : fp(fp), arg(std::bit_cast<int64_t>(arg)) {}
container::command::command(function_t fp, int64_t arg) noexcept : fp(fp), arg(arg) {}

container::command_description::command_description() noexcept : name({ 0,0 }), /*rvalue({0,0}),*/ argument_count(0), requires_scope(false), is_not_member_function(false), has_return(false), effect(false), nest_level(0), parent(SIZE_MAX) {}
container::command_description::command_description(
  const global_string_view& name,
  //const global_string_view& rvalue,
  uint32_t argument_count,
  bool requires_scope,
  bool is_not_member_function,
  bool has_return,
  bool effect,
  size_t nest_level,
  size_t parent
) noexcept :
  name(name), /*rvalue(rvalue),*/ argument_count(argument_count), requires_scope(requires_scope),
  is_not_member_function(is_not_member_function), has_return(has_return), effect(effect), nest_level(nest_level), parent(parent)
{}

// magic number
container::container() noexcept : prng_state(0x9e3779b97f4a7c15ULL) {}
void container::process(context* ctx) const {
  const container* prev_script = ctx->current_script;
  ctx->current_script = this;
  for (; ctx->current_index < cmds.size(); ++ctx->current_index) {
    const auto& cmd = cmds[ctx->current_index];
    std::invoke(cmd.fp, cmd.arg, ctx, this);
  }
  ctx->current_script = prev_script;
}

void container::make_table(context* ctx, std::vector<std::tuple<any_stack, any_stack>>& table) const {
  table.clear();
  table.resize(block_descs.size());

  size_t counter = 0;
  for (size_t i = 0; i < cmds.size(); ++i) {
    const auto& cmd = cmds[ctx->current_index];
    const auto& desc = descs[i];

    const size_t curplace = i;

    const bool no_jump = i >= ctx->current_index;
    if (no_jump && !desc.effect) {
      std::invoke(cmd.fp, cmd.arg, ctx, this); // doesnt need to be invoked if desc.effect
    }
    ctx->current_index += size_t(no_jump);

    while (counter < block_descs.size() && curplace == block_descs[counter].cmd_index) {
      const auto si = block_descs[counter].scope_index;
      if (no_jump) {
        table[counter] = std::make_tuple(
          ctx->stack.safe_get<any_stack>(),
          si >= 0 ? ctx->stack.safe_get<any_stack>(si) : any_stack() 
        );
      }
      counter += 1;
    }
  }

  // the first script block
  if (!ctx->return_type().empty() && !table.empty()) {
    any_stack s;
    if (get_arg_name(0) == "root") s = ctx->get_arg<any_stack>(0);
    table.back() = std::make_tuple(ctx->get_return<any_stack>(), s);
  }
}

void container::make_table(context* ctx, node_view& viewer) const {
  make_table(ctx, viewer.table);
}

void container::build_description_index() {
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

  size_t counter = 0;
  for (size_t i = 0; i < cmds.size(); ++i) {
    const bool reached = i >= work.current_index;
    bool ok = reached;
    std::string error;

    if (reached && !descs[i].effect) {
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
      if (ok && reached && !descs[i].effect && work.stack.size() > 0) {
        entry.has_value = true;
        entry.value = work.stack.get<any_stack>();
        const auto si = block_descs[node].scope_index;
        if (si >= 0 && !work.stack.invalid(si)) entry.scope = work.stack.get<any_stack>(si);
      } else if (!error.empty()) {
        entry.error = error;
      }
    };

    if (description_cmd_index_offsets.size() == cmds.size() + 1) {
      for (size_t offset = description_cmd_index_offsets[i]; offset < description_cmd_index_offsets[i + 1]; ++offset) {
        const size_t node = description_cmd_index_nodes[offset];
        if (get_string(block_descs[node].name) != get_string(descs[i].name)) continue;
        record(node);
      }
    } else {
      for (size_t node = 0; node < block_descs.size(); ++node) {
        const auto& bd = block_descs[node];
        if (bd.cmd_start != i || bd.cmd_start == bd.cmd_index) continue;
        if (get_string(bd.name) != get_string(descs[i].name)) continue;
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

std::string_view container::get_string(const size_t start, const size_t count) const {
  if (count == SIZE_MAX) return to_string(static_cast<basicf>(start));

  if (globals.size() == 0) return std::string_view();
  if (start + count > globals[0].size()) return std::string_view();
  return std::string_view(globals[0]).substr(start, count);
}

std::string_view container::get_string(const command_description::global_string_view& str) const {
  return get_string(str.start, str.count);
}

std::string disassemble(const container& scr) {
  std::string out;
  for (size_t i = 0; i < scr.cmds.size(); ++i) {
    const auto& desc = scr.descs[i];
    const int64_t arg = scr.cmds[i].arg;
    const std::string_view name = scr.get_string(desc.name);

    // A desc whose name has count == SIZE_MAX is a basic instruction; its `start` is the
    // basicf id (see push_basic_function). User-function descs carry a real global string.
    const bool is_basic = desc.name.count == SIZE_MAX;
    const basicf op = is_basic ? static_cast<basicf>(desc.name.start) : basicf::invalid;

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

size_t container::find_arg(const std::string_view& name) const {
  size_t i = 0;
  for (; i < args.size() && get_string(args[i].name) != name; ++i) {}
  return i < args.size() ? i : SIZE_MAX;
}

std::string_view container::get_arg_name(const size_t index) const {
  if (index >= args.size()) return std::string_view();
  return get_string(args[index].name);
}

size_t container::find_saved(const std::string_view& name) const {
  size_t i = 0;
  for (; i < saved.size() && get_string(saved[i].name) != name; ++i) {}
  return i < saved.size() ? i : SIZE_MAX;
}

std::string_view container::get_saved_name(const size_t index) const {
  if (index >= saved.size()) return std::string_view();
  return get_string(saved[index].name);
}

size_t container::find_list(const std::string_view& name) const {
  size_t i = 0;
  for (; i < lists.size() && get_string(lists[i].name) != name; ++i) {}
  return i < lists.size() ? i : SIZE_MAX;
}

std::string_view container::get_list_name(const size_t index) const {
  if (index >= lists.size()) return std::string_view();
  return get_string(lists[index].name);
}

container_view::container_view(const container* scr, const size_t start, const size_t end) noexcept :
  scr(scr), start(start), end(end)
{}

void container_view::process(context* ctx) const {
  const container* prev_script = ctx->current_script;
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
  //for (auto itr = stack.rbegin()+cur_stack_size; itr != stack.rend() && ret; ++itr) {
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

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
