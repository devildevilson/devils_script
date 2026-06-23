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

// Compiled script representation and inspection helpers.
//
// A `container` owns bytecode-like commands, source-backed strings, argument/saved/list
// metadata, and a parallel description tree. `process()` executes the commands against a
// mutable `context`; `describe()` walks the description tree and partially evaluates nodes
// where the current context makes that possible.
//
// The command and description arrays are intentionally kept in lockstep for executable
// instructions. Higher-level block descriptions reference command ranges instead of owning
// commands themselves, which lets tooling reconstruct the script structure without changing
// the compact execution layout.

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

struct container;

// Legacy tree view used by older description callers. Prefer `container::describe`.
struct node_view {
  using fn_t = std::function<bool(const std::string_view &, const std::string_view &, const size_t, const any_stack &, const any_stack&)>;
  
  std::vector<std::tuple<any_stack, any_stack>> table;
  std::vector<size_t> stack;

  bool traverse(const container* scr, const size_t offset, const size_t nest_level, const fn_t& fn);
  bool traverse(const container* scr, const fn_t &fn);
};

struct script_container {
  struct string_ref { size_t start, count; };

  // Source position of the construct that emitted a command, kept 1:1 with `cmds`.
  // Survives strip_description so runtime error messages can report `script '<name>' @ L:C`.
  struct src_loc { uint32_t line, column; };

  struct command {
    function_t fp; 
    int64_t arg; 

    command() noexcept;
    explicit command(function_t fp, bool arg) noexcept; 
    explicit command(function_t fp, double arg) noexcept;
    explicit command(function_t fp, int64_t arg) noexcept;
  };

  struct argument_data {
    string_ref name;
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
  std::vector<src_loc> locs;
  std::vector<argument_data> args;
  std::vector<argument_data> saved;
  std::vector<argument_data> lists;
  std::vector<list_pipeline_op> list_pipeline_ops;
  std::string string_pool;
  std::vector<string_ref> command_names;
  string_ref name{};
  // Static return type of this script (the parse-time RETURN_T), as a stable type-name view.
  // Used by `execute` to type-check a sub-script's return against the caller's expected type.
  std::string_view return_type;

  script_container() noexcept;
  void process(context* ctx) const;
  void shrink_to_fit();
  std::string_view get_string(const size_t start, const size_t count) const;
  std::string_view get_string(const string_ref& str) const;
  std::string_view get_name() const;
  std::string_view get_command_name(const size_t index) const;
  // Source position of the command at ctx->current_index ({0,0} if out of range).
  src_loc loc_at(const context* ctx) const;
  // Throws std::runtime_error prefixed with `script '<name>' @ <line>:<column>: ` using the
  // source position of the command at ctx->current_index. Used by runtime command handlers.
  [[noreturn]] void error_at(const context* ctx, const std::string_view& msg) const;
  size_t find_arg(const std::string_view &name) const;
  std::string_view get_arg_name(const size_t index) const;
  size_t find_saved(const std::string_view& name) const;
  std::string_view get_saved_name(const size_t index) const;
  size_t find_list(const std::string_view &name) const;
  std::string_view get_list_name(const size_t index) const;
};

struct container : public script_container {
  using local_stack_element = std::tuple<std::string_view, stack_element>;
  using description_output_t = std::function<void(const container*, const size_t, const local_stack_element&, const std::span<local_stack_element>&)>;
  using command = script_container::command;
  using argument_data = script_container::argument_data;
  using list_pipeline_kind = script_container::list_pipeline_kind;
  using list_pipeline_op = script_container::list_pipeline_op;

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

  // Structural description node. The tree is stored in prefix order like command_block,
  // but each node references the command range that evaluates it.
  struct block_description {
    script_container::string_ref name;
    script_container::string_ref custom_description;
    size_t size;
    size_t args_count;
    size_t cmd_index;
    size_t cmd_start;
    size_t cmd_end;
    int64_t scope_index;
    bool placeholder;
    bool effect;   // node is a void function/iterator: skip evaluation during describe()
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

  // Per-command link to the block_description node that produced it (or SIZE_MAX for commands
  // with no described node, e.g. internal control-flow jumps). Built by build_description_index.
  // The command's opcode name and effect-ness are derived from this node, never stored per command.
  std::vector<size_t> cmd_node;
  std::vector<block_description> block_descs;
  std::vector<size_t> description_cmd_index_offsets;
  std::vector<size_t> description_cmd_index_nodes;

  container() noexcept;
  script_container strip_description() const&;
  script_container strip_description() &&;
  void shrink_to_fit();
  void make_table(context* ctx, std::vector<std::tuple<any_stack, any_stack>> &table) const;
  void make_table(context* ctx, node_view& viewer) const;
  void build_description_index();
  void describe(context* ctx, const description_callback_t& fn) const;
};

void shrink_to_fit(std::span<script_container> scripts);
void shrink_to_fit(std::span<container> scripts);

// Executable view over a command range; used to pass script subblocks into iterator callbacks.
struct container_view {
  const script_container* scr; 
  size_t start; 
  size_t end;

  container_view(const script_container* scr, const size_t start, const size_t end) noexcept;
  void process(context* ctx) const;
  std::string_view get_string(const size_t start, const size_t count) const;
};

template <typename Signature>
struct script_function;

template <typename R, typename Arg>
struct script_function<R(Arg)> {
  context* ctx;
  const script_container* scr;
  size_t start;
  size_t end;

  script_function() noexcept : ctx(nullptr), scr(nullptr), start(0), end(0) {}
  script_function(std::nullptr_t) noexcept : script_function() {}
  script_function(context* ctx, const script_container* scr, const size_t start, const size_t end) noexcept :
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
