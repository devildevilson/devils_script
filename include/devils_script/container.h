#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <span>
#include <stdexcept>
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

  bool traverse(const container* scr, const size_t offset, const size_t nest_level, const fn_t& fn);
  bool traverse(const container* scr, const fn_t &fn);
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
    bool effect;
    size_t nest_level; // is it needed? dont think so
    size_t parent;

    command_description() noexcept;
    command_description(
      const global_string_view &name,
      uint32_t argument_count,
      bool requires_scope,
      bool is_not_member_function,
      bool has_return,
      bool effect,
      size_t nest_level,
      size_t parent
    ) noexcept;
  };

  enum class description_node_kind {
    unknown,
    literal,
    function,
    effect,
    operator_t,
    iterator,
    block,
    argument,
    scope,
    control_flow,
    conversion,
    instruction
  };

  // script description data, structure similar to rpn_conversion_ctx::block but upside down
  struct block_description {
    command_description::global_string_view name;
    command_description::global_string_view custom_description;
    size_t size;
    size_t args_count;
    size_t cmd_index;
    size_t cmd_start;
    size_t cmd_end;
    int64_t scope_index; // why int64_t?
    bool placeholder;
    description_node_kind kind;
  };

  enum class description_value_state {
    unavailable,
    value,
    placeholder
  };

  struct description_entry {
    size_t node;
    std::string_view name;
    std::string_view custom_description;
    size_t nest_level;
    description_node_kind kind;
    description_value_state state;
    any_stack value;
    any_stack scope;
    std::string error;
  };

  using description_callback_t = std::function<void(const description_entry&)>;

  struct argument_data {
    command_description::global_string_view name;
    std::string_view type;
  };

  enum class list_pipeline_kind {
    add_to,
    clear,
    filter,
    map,
    count,
    empty,
    any,
    all,
    none,
    count_if,
    sum,
    min,
    max,
    average,
    first,
    last
  };

  struct list_pipeline_op {
    list_pipeline_kind kind;
    size_t list_index;
    std::string_view input_type;
    size_t value_start;
    size_t value_end;
    size_t default_start;
    size_t default_end;
    size_t end;
  };

  uint64_t prng_state;

  std::vector<command> cmds;
  std::vector<command_description> descs;
  std::vector<block_description> block_descs;
  std::vector<size_t> description_cmd_index_offsets;
  std::vector<size_t> description_cmd_index_nodes;

  // first is always script text
  std::vector<std::string> globals;
  std::vector<argument_data> args;
  std::vector<argument_data> saved;
  std::vector<argument_data> lists;
  std::vector<list_pipeline_op> list_pipeline_ops;

  container() noexcept;
  void process(context* ctx) const; // dont forget 'ctx->clear()' and 'ctx->create_lists(this)'
  void make_table(context* ctx, std::vector<std::tuple<any_stack, any_stack>> &table) const;
  void make_table(context* ctx, node_view& viewer) const;
  void build_description_index();
  void describe(context* ctx, const description_callback_t& fn) const;
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

template <typename Signature>
struct script_function;

template <typename R, typename Arg>
struct script_function<R(Arg)> {
  context* ctx;
  const container* scr;
  size_t start;
  size_t end;

  script_function() noexcept : ctx(nullptr), scr(nullptr), start(0), end(0) {}
  script_function(std::nullptr_t) noexcept : script_function() {}
  script_function(context* ctx, const container* scr, const size_t start, const size_t end) noexcept :
    ctx(ctx), scr(scr), start(start), end(end)
  {}

  script_function& operator=(std::nullptr_t) noexcept {
    ctx = nullptr;
    scr = nullptr;
    start = 0;
    end = 0;
    return *this;
  }

  explicit operator bool() const noexcept { return ctx != nullptr && scr != nullptr && start < end; }

  R operator()(Arg in) const;
};

// Human-readable disassembly of the compiled command array — one line per instruction:
// `idx: opcode <arg>`, with branch targets rendered as `-> N`. For debugging and for
// golden tests that pin the output of the compilation step (see tests/disasm.cpp).
std::string disassemble(const container& scr);

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
