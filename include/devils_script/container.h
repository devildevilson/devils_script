#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <span>
#include "devils_script/common.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

struct container;

// simple script viewer, you probably need something better
struct node_view {
  using fn_t = std::function<bool(const std::string_view &, const std::string_view &, const size_t, const any_stack &, const any_stack&)>;
  
  std::vector<std::tuple<any_stack, any_stack>> table;
  std::vector<size_t> stack;

  bool traverse(container* scr, const size_t offset, const size_t nest_level, const fn_t& fn);
  bool traverse(container* scr, const fn_t &fn);
};

struct container {
  using local_stack_element = std::tuple<std::string_view, stack_element>;
  using description_output_t = std::function<void(const container*, const size_t, const local_stack_element&, const std::span<local_stack_element>&)>;

  struct command { 
    function_t fp; 
    int64_t arg; 

    command() noexcept;
    explicit command(function_t fp, bool arg) noexcept; 
    explicit command(function_t fp, double arg) noexcept;
    explicit command(function_t fp, int64_t arg) noexcept;
  };
  
  // every command description
  struct command_description {
    struct global_string_view { size_t start, count; };

    // unfortunately needs to be rewritten =(
    global_string_view name;
    uint32_t argument_count;
    bool requires_scope;
    bool is_not_member_function;
    bool has_return;
    size_t nest_level; // is it needed? dont think so
    size_t parent;

    command_description() noexcept;
    command_description(
      const global_string_view &name,
      uint32_t argument_count,
      bool requires_scope,
      bool is_not_member_function,
      bool has_return,
      size_t nest_level,
      size_t parent
    ) noexcept;
  };

  // script description data, structure similar to rpn_conversion_ctx::block but upside down
  struct block_description {
    command_description::global_string_view name;
    command_description::global_string_view custom_description;
    size_t size;
    size_t args_count;
    size_t cmd_index;
    int64_t scope_index; // why int64_t?
  };

  struct argument_data {
    command_description::global_string_view name;
    std::string_view type;
  };

  uint64_t prng_state;

  std::vector<command> cmds;
  std::vector<command_description> descs;
  std::vector<block_description> block_descs;

  // first is always script text
  std::vector<std::string> globals;
  std::vector<argument_data> args;
  std::vector<argument_data> saved;
  std::vector<argument_data> lists;

  container() noexcept;
  void process(context* ctx) const; // dont forget 'ctx->clear()' and 'ctx->create_lists(this)'
  //void describe(context* ctx, const description_output_t& f) const;
  void make_table(context* ctx, std::vector<std::tuple<any_stack, any_stack>> &table) const;
  void make_table(context* ctx, node_view& viewer) const;
  std::string_view get_string(const size_t start, const size_t count) const;
  std::string_view get_string(const command_description::global_string_view& str) const;
  size_t find_arg(const std::string_view &name) const;
  std::string_view get_arg_name(const size_t index) const;
  size_t find_saved(const std::string_view& name) const;
  std::string_view get_saved_name(const size_t index) const;
  size_t find_list(const std::string_view &name) const;
  std::string_view get_list_name(const size_t index) const;
};

// for subscripts in iterators
struct container_view {
  const container* scr; 
  size_t start; 
  size_t end;

  container_view(const container* scr, const size_t start, const size_t end) noexcept;
  void process(context* ctx) const;
  std::string_view get_string(const size_t start, const size_t count) const;
};

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}