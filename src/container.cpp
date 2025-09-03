#include "devils_script/container.h"

#include <bit>
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
  for (; ctx->current_index < cmds.size(); ++ctx->current_index) {
    const auto& cmd = cmds[ctx->current_index];
    std::invoke(cmd.fp, cmd.arg, ctx, this);
  }
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

std::string_view container::get_string(const size_t start, const size_t count) const {
  if (count == SIZE_MAX) return to_string(static_cast<basicf>(start));

  if (globals.size() == 0) return std::string_view();
  if (start + count > globals[0].size()) return std::string_view();
  return std::string_view(globals[0]).substr(start, count);
}

std::string_view container::get_string(const command_description::global_string_view& str) const {
  return get_string(str.start, str.count);
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
  if (index >= args.size()) return std::string_view();
  return get_string(args[index].name);
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
  for (ctx->current_index = start; ctx->current_index < end; ++ctx->current_index) {
    const auto& cmd = scr->cmds[ctx->current_index];
    std::invoke(cmd.fp, cmd.arg, ctx, scr);
  }
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