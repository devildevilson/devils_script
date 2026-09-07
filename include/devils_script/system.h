#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <string>
#include <format>
#include <iostream>
#include <functional>
#include <optional>
#include <span>
#include <tuple>
#include <unordered_map>
#include <deque>
#include <vector>
#include "devils_script/common.h"
#include "devils_script/type_traits.h"
#include "devils_script/container.h"
#include "devils_script/basic_functions.h"
#include "devils_script/template_functions.h"
#include "devils_script/prng.h"
#include "devils_script/script_ast.h"
#include "tavl/parser.h"

// Main compiler/registry facade.
//
// A `system` stores the registered functions, operators, enum parsers, diagnostics, and
// parser defaults used to compile scripts into `container` objects. The registry is intended
// to be built once and reused; per-parse mutable state lives in `parse_ctx` so parsing can be
// performed through const member functions.
//
// Compilation is split into three stages:
// 1. tavl parses source text into a structural AST with operator precedence applied.
// 2. `rpn_conversion_ctx` normalizes that AST into compact prefix `command_block` ranges.
// 3. Semantic dispatch resolves functions/scopes/types and emits VM commands plus
//    description metadata.
//
// Non-obvious implementation detail: a scope-changing function can be followed by a script
// block (`scope_fn = { ... }` or `scope_fn:child = { ... }`). The compiler temporarily pushes
// the produced scope type into `parse_ctx::scope_stack`, compiles the nested block, and emits
// nullable guards for `?=` calls when the return type supports validity checks.

namespace devils_script {


class system {
public:
  enum class function_type { lvalue, rvalue };
  enum class user_function_type {
    effect,
    condition,
    arithmetic,
    string,
    object,
    iterator_effect,
    iterator_condition,
    iterator_arithmetic,
  };

  template <typename F, typename RT = void, bool is_iterator_func = false>
  constexpr static user_function_type get_user_function_type();
  constexpr static std::string_view get_user_function_type_name(const user_function_type t);

  struct rpn_conversion_ctx {
    enum class block_kind : uint8_t {
      node,
      string_literal,
      nullable_call,
      braced_call,
      nullable_braced_call,
      scope_path,
      scope_path_call,
    };

    struct token_ref {
      size_t offset = SIZE_MAX;
      size_t size = 0;
      size_t line = 0;
      size_t column = 0;
    };

    struct block {
      token_ref token;
      size_t size;
      block_kind kind = block_kind::node;
    };

    std::vector<block> output;
    std::string token_storage;

    std::tuple<token_ref, size_t> convert_scope(const std::string_view& expr, block* arr, const size_t max_size, size_t line = 0, size_t column = 0);
    std::string_view token_text(const token_ref& token) const noexcept;
    token_ref store_token(std::string_view text, size_t line = 0, size_t column = 0);

    // Path N: build the same rpn block stream as convert_block, but from a tavl AST (make_script_ast)
    // instead of text. Reuses convert_scope for lvalue scope-path splitting; tavl already supplies math
    // precedence so the shunting-yard (convert/convert_block) is bypassed. Returns the root row count.
    size_t normalize(const std::vector<tavl::node>& tree, std::string_view src);
    size_t normalize_block(const tavl::node* block, std::string_view src);
    void normalize_row(const tavl::node* row, std::string_view src);
    size_t normalize_expr(const tavl::node* n, std::string_view src);
    token_ref normalize_token(const tavl::node* n, std::string_view src);

    void clear();
  };

  struct command_block {
    std::span<rpn_conversion_ctx::block> data;
    const std::string* token_storage = nullptr;

    command_block() noexcept;
    command_block(const std::span<rpn_conversion_ctx::block> &data, const std::string* token_storage) noexcept;
    command_block(const command_block& block, const size_t index) noexcept;
    command_block find(const std::string_view &name) const;
    command_block at(const size_t index) const;
    std::string_view name() const;
    size_t line() const;
    size_t column() const;
    size_t args_count() const;
    size_t size() const;
    bool nullable() const;
    bool string_literal() const;
    bool braced_args() const;
    bool empty() const;

    // Forward range over the direct child blocks, auto-skipping `custom_description`
    // entries. Replaces the hand-rolled `child = command_block(args, offset);
    // offset += child.size(); if (child.name() == custom_description_constant) continue;`
    // idiom that was repeated across the clause-folding functions.
    struct child_iterator {
      const command_block* parent;
      size_t offset;
      void skip_desc() {
        while (offset < parent->size() && command_block(*parent, offset).name() == custom_description_constant)
          offset += command_block(*parent, offset).size();
      }
      command_block operator*() const { return command_block(*parent, offset); }
      child_iterator& operator++() { offset += command_block(*parent, offset).size(); skip_desc(); return *this; }
      bool operator==(const child_iterator& o) const { return offset == o.offset; }
      bool operator!=(const child_iterator& o) const { return offset != o.offset; }
      // true when no further non-description child follows the current one.
      bool is_last() const { child_iterator n = *this; ++n; return n.offset >= parent->size(); }
    };
    struct children_view {
      const command_block* parent;
      child_iterator begin() const { child_iterator it{parent, 1}; it.skip_desc(); return it; }
      child_iterator end() const { return child_iterator{parent, parent->size()}; }
    };
    children_view children() const { return children_view{this}; }
  };

  struct parse_context {
    function_type ftype;
    std::string_view expected_type;
    std::string_view string_upvalue;
    std::string_view scope_type_upvalue;
    size_t nest_level;
    size_t source_line;     // 1-based source position of the block currently being emitted
    size_t source_column;   // (used to stamp script_container::locs for error reporting)
    size_t unlimited_func_index;
    size_t list_index_upvalue;
    size_t prev_chaining;
    size_t description_placeholder_depth;

    // Peak parse-time operand-stack depth reached so far (max of stack_types.size() plus, at each
    // `execute` site, the sub-script's own peak stacked above the caller's frame). Reset per parse in
    // init(); stored into container::max_stack at parse end. `max_child_saved` is the deepest
    // max_saved among all executed sub-scripts; container::max_saved = saved.size() + max_child_saved.
    size_t max_stack_depth;
    size_t max_child_saved;
    size_t max_child_lists;   // deepest max_lists among executed sub-scripts; see container::max_lists
    // Upper bounds enforced at parse end (default to the context's stack/local-vars sizes). A script
    // whose computed max_stack/max_saved exceeds these is rejected so it can never overrun the runtime
    // stacks. Set before parsing to carve scripts into nesting classes with smaller budgets.
    size_t max_stack_limit;
    size_t max_saved_limit;
    int64_t conversion_cost;

    std::vector<std::string_view> function_names;
    std::vector<int64_t> scope_stack;
    std::vector<std::string_view> stack_types;

    // Command slots that carry immediate data instead of an instruction (iterator callback ranges,
    // list_pipeline metadata). They are never executed, and the peephole must not read them as
    // opcodes - an iterator range slot in particular holds a `jump` fp but is not a jump.
    std::vector<size_t> data_slots;

    // Every place where a compiled command index is stored inside a command argument. Recorded as
    // codegen writes it, because some of these fields cannot be recovered afterwards: the nullable
    // scope guard packs its target into half an argument behind a template fp that no opcode table
    // knows, and list_pipeline stores its section bounds relative to its own command.
    struct cmd_index_field {
      size_t cmd;             // command whose argument holds the index
      size_t base;            // index is stored relative to this command (SIZE_MAX = absolute)
      uint8_t half;           // 0 = whole argument, 1 = low half of pack2, 2 = high half
      bool zero_is_absent;    // a stored 0 means "no such section", not "command 0"
    };
    std::vector<cmd_index_field> cmd_index_fields;

    // `context` pushes whose scope turned out to be dead: the block compiled to
    // `context; <direct ctx read>; erase`, and the read takes the context from `context*` itself.
    // The peephole drops the push and its unwind. `scope_slot` is the operand-stack slot the push
    // occupied, so description nodes that named it as their scope can stop naming a slot that is no
    // longer there. See system::optimize_commands.
    struct dead_context_push { size_t cmd; size_t unwind; int64_t scope_slot; };
    std::vector<dead_context_push> dead_context_pushes;

    // per-parse mutable scratch — moved off `system` so the registry stays const/shareable
    rpn_conversion_ctx rpn_ctx;
    script_ast_context script_ast_ctx;
    std::vector<tavl::node> script_ast_nodes;
    prng::xoshiro256starstar::state prng_s;
    std::string_view root_block_name;
    std::string_view return_type;
    std::string_view root_type;
    bool initialized;

    parse_context() noexcept;

    template <typename RETURN_T, typename ROOT_T>
    void init(const system& sys, container& c);

    uint64_t gen_value();           // advances this parse's PRNG (seeded from system at parse start)

    bool is_func_subblock() const;
    void push_func(const std::string_view &name);
    void pop_func();

    size_t current_scope_index() const;
    std::string_view current_scope_type() const;
    template <typename T>
    bool is_scope() const;

    template <typename T>
    bool is() const;

    bool is_ignore() const;
    bool is_bool() const;
    bool is_integral() const;
    bool is_number() const;
    bool is_fundamental() const;
    bool is_string() const;
    bool is_object() const;

    bool pop_while_ignore();

    template <typename T>
    void push();
    void push(const std::string_view &type);
    void pop();
    void erase(const size_t index);
    std::string_view top() const;
  };

  using parse_ctx = parse_context;

  // Codegen sink: wraps (system, parse_ctx, container) and centralizes instruction emission +
  // forward-jump backpatching, so function init-callbacks stop hand-rolling the
  // `std::vector<size_t> jumps; ...; cmds[i].arg = cmds.size()` dance. Thin facade over
  // push_basic_function (the insn table) — does not own stack_types yet (still in parse_ctx).
  struct emitter {
    const system* sys;
    parse_ctx* ctx;
    container* scr;

    // A forward-jump target: a set of pending jump-sites all resolved to one address by bind().
    struct label { std::vector<size_t> sites; };

    size_t emit(const basicf op, const int64_t arg = 0) const;  // -> push_basic_function
    size_t emit_string(const std::string_view& str) const;      // -> push_string

    label make_label() const;
    void jump_to(const basicf op, label& l) const;     // emit `op` (placeholder target), record its site
    void mark(label& l, const size_t cmd_index) const; // record an already-emitted cmd as a jump-site
    void bind(label& l) const;                         // patch every recorded site to current cmds.size()

    // Emit one guarded clause of a control-flow combinator. When `guarded`, a `condjump`
    // on the boolean test the caller just emitted skips the body (to a fresh per-clause
    // label) if it is false; then `emit_body()` runs; then an unconditional jump to the
    // shared `end`; finally the skip label is bound past the body. Centralizes the
    // condjump/jump/bind ordering shared by select and random.
    template <typename Body>
    void guarded_clause(label& end, const bool guarded, Body&& emit_body) const {
      label skip = make_label();
      if (guarded) jump_to(basicf::condjump, skip);
      emit_body();
      jump_to(basicf::jump, end);
      if (guarded) bind(skip);
    }
  };

  struct command_data {
    enum class ftype { operator_t, function_t, invalid };
    enum class associativity { left, right };
    enum class math_ftype { prefix = 1, binary, postfix };
    using init_fn_t = std::function<size_t(emitter&, const command_block&)>;

    std::string name;
    std::string_view expected_scope;
    std::string_view return_type;
    std::vector<std::string_view> argument_types;
    std::string function_signature;
    int32_t priority;
    int32_t arg_count; // or math_ftype
    associativity assoc;
    ftype type;
    init_fn_t init;
    container::description_node_kind description_kind = container::description_node_kind::unknown;
    // Set for everything registered by init_math()/init_basic_functions(). Constant folding only
    // touches names whose every overload carries this flag, so a user registration under a
    // built-in name (which may have a different result or effects) is never folded away.
    bool builtin = false;
  };

  using custom_init_fn_t = std::function<void(emitter&, const command_block&, const std::vector<std::string> &)>;

  struct operator_props { int32_t priority; command_data::math_ftype mtype; command_data::associativity assoc; };

  struct arithmetic_type_data {
    std::string_view type;
    std::string block_name;
    int32_t priority;
  };

  struct conversion_data {
    std::string_view from;
    std::string_view to;
    int32_t cost;
    function_t safe;
    function_t unsafe;
  };

  class nest_level_changer {
  public:
    parse_ctx* ctx;
    nest_level_changer(parse_ctx* ctx) noexcept;
    ~nest_level_changer() noexcept;
  };

  // Tracks the source position of the block currently being emitted so that emitted commands
  // can be stamped into script_container::locs. Restores the previous position on scope exit,
  // which leaves a parent block's position in effect again after its children are processed.
  class source_position_changer {
  public:
    parse_ctx* ctx;
    size_t prev_line, prev_column;
    source_position_changer(parse_ctx* ctx, const size_t line, const size_t column) noexcept;
    ~source_position_changer() noexcept;
  };

  class function_name_changer {
  public:
    parse_ctx* ctx;
    function_name_changer(parse_ctx* ctx, const std::string_view &str) noexcept;
    ~function_name_changer() noexcept;
  };

  class set_expected_type {
  public:
    parse_ctx* ctx;
    std::string_view expected;
    set_expected_type(parse_ctx* ctx, const std::string_view &expected) noexcept;
    ~set_expected_type() noexcept;
  };

  class set_function_type {
  public:
    parse_ctx* ctx;
    function_type t;
    set_function_type(parse_ctx* ctx, const function_type t) noexcept;
    ~set_function_type() noexcept;
  };

  class push_list_index_upvalue {
  public:
    parse_ctx* ctx;
    size_t prev_id;
    push_list_index_upvalue(parse_ctx* ctx, const size_t id) noexcept;
    ~push_list_index_upvalue() noexcept;
  };

  class change_chain_index {
  public:
    parse_ctx* ctx;
    change_chain_index(parse_ctx* ctx) noexcept;
    ~change_chain_index() noexcept;
  };

  class description_placeholder {
  public:
    parse_ctx* ctx;
    description_placeholder(parse_ctx* ctx) noexcept;
    ~description_placeholder() noexcept;
  };

  using argument_callback = std::function<void(parse_ctx*, container*, const size_t, const command_block&)>;

  using err_fn = std::function<void(const std::string &)>;
  // Maps a script name to a pre-compiled, caller-owned sub-script for the `execute` builtin.
  // The registry must outlive any container parsed against it. Returns nullptr when unknown.
  using script_resolver_t = std::function<const script_container*(std::string_view)>;
  enum class safety { unsafe, safe };
  struct options { uint64_t seed; enum safety safety; bool optimize; err_fn error; err_fn warning; options() noexcept; };
  system(const options &opts = options()) noexcept;
  void init_math();
  void init_basic_functions();

  template <typename T>
    requires(valid_stack_el_type_v<T>)
  void register_arithmetic_type(std::string block_name = "ADD", int32_t priority = 0);

  template <typename FROM, typename TO>
    requires(valid_stack_el_type_v<FROM> && valid_stack_el_type_v<TO>)
  void register_implicit_conversion(int32_t cost = 1);

  bool is_arithmetic_type(const std::string_view& type) const noexcept;
  // True when `name` is registered and every one of its overloads came from init_math() /
  // init_basic_functions(). Constant folding uses this as its permission check.
  bool is_builtin_function(const std::string_view& name) const;
  std::string_view arithmetic_block_for(const std::string_view& type) const noexcept;
  std::optional<int32_t> implicit_conversion_cost(const std::string_view& from, const std::string_view& to) const;
  bool can_convert_implicitly(const std::string_view& from, const std::string_view& to) const;
  void setup_type_conversion(parse_ctx* ctx, container* scr, const std::string_view& from, const std::string_view& to) const;

  void toggle_safety();
  bool safety() const;
  // Peephole optimization of the compiled command stream. On by default; turning it off compiles the
  // literal lowering, which is what the optimizer is differentially tested against.
  void toggle_optimizations();
  bool optimizations() const;
  void raise_error(const std::string &msg) const;
  void raise_warning(const std::string& msg) const;
  uint64_t get_seed() const;
  void reseed(const uint64_t val);
  std::string dump_registered_functions() const;

  // Installs the resolver used by the `execute` builtin to look up sub-scripts by name during
  // parsing. Set this once (before parsing) on the otherwise-const registry.
  void set_script_resolver(script_resolver_t resolver);
  // Resolves a sub-script by name through the installed resolver (nullptr if none/unknown).
  const script_container* resolve_script(const std::string_view& name) const;

  template <typename Arg>
  size_t parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const size_t index, const std::vector<std::string>& func_args_names) const;

  template <typename Arg>
  size_t parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const size_t index, const std::vector<std::string>& func_args_names, const argument_callback& fn) const;

  template <size_t I, size_t LI, size_t COUNT, typename F>
  size_t parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const std::vector<std::string>& func_args_names) const;

  template <size_t I, size_t LI, size_t COUNT, typename F>
  size_t parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const std::vector<std::string>& func_args_names, const argument_callback& fn) const;

  template <size_t I, size_t LI, typename F>
  size_t parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const std::vector<std::string>& func_args_names) const;

  template <size_t I, size_t LI, typename F>
  size_t parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const std::vector<std::string>& func_args_names, const argument_callback &fn) const;

  template <size_t I, size_t LI, typename F>
  size_t parse_arg(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view& override_expected, const std::string_view& override_func, const std::string& arg_name, const argument_callback& fn = nullptr) const;

  template <typename Arg>
  size_t parse_arg(parse_ctx* ctx, container* scr, const command_block& block, const size_t index, const std::string_view& override_expected, const std::string_view& override_func, const std::string& arg_name, const argument_callback& fn = nullptr) const;

  template <size_t I, size_t LI, typename F>
  size_t parse_arg(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view& override_expected, const basicf& override_block_behaviour, const std::string& arg_name, const argument_callback& fn = nullptr) const;

  template <typename Arg>
  size_t parse_arg(parse_ctx* ctx, container* scr, const command_block& block, const size_t index, const std::string_view& override_expected, const basicf& override_block_behaviour, const std::string& arg_name, const argument_callback& fn = nullptr) const;

  template <typename FROM, typename TO>
  void setup_type_conversion(parse_ctx* ctx, container* scr) const;

  // Emits a safety-aware call instruction for a registered function/operator. The structural
  // description node (with name/effect) is produced separately by setup_block_description.
  template <auto f, typename HT, is_valid_t<HT> vf>
    requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
  void emit_call_instruction(parse_ctx* ctx, container* scr, function_t safe, function_t unsafe, const int64_t scope_index) const;

  // Applies a call's declared stack effect: consume `pops` argument slots, push the result type.
  template <typename RetT>
  void apply_call_stack_effect(parse_ctx* ctx, const size_t pops) const;

  // Stores the parse-time peak stack / saved-frame usage onto the container and rejects a script that
  // exceeds parse_context::max_stack_limit / max_saved_limit. Called once at the end of each parse.
  void finalize_resource_usage(parse_ctx& ctx, container& c) const;

  // Emits a push_command_name opcode (carrying `name` as a packed string-pool ref) and tracks the
  // extra string_view on the parse stack. Used right before an effect call so its on_effect callback
  // reads the name off the stack — replacing the per-command command_names side table.
  void emit_command_name(parse_ctx* ctx, container* scr, const std::string_view& name) const;

  template <auto f>
    requires(valid_function_type<decltype(f)>)
  void register_function(std::string name, std::vector<std::string> func_args_names = {}, custom_init_fn_t init_f = nullptr);

  template <auto f, typename HT, is_valid_t<HT> vf = &is_valid<HT>>
    requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
  void register_function(std::string name, std::vector<std::string> func_args_names = {}, custom_init_fn_t init_f = nullptr);

  template <auto f, typename HT, on_effect_t<decltype(f), HT> eff, is_valid_t<HT> vf = &is_valid<HT>>
    requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
  void register_function(std::string name, std::vector<std::string> func_args_names = {}, custom_init_fn_t init_f = nullptr);

  template <auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff>
    requires(valid_function_type<decltype(f)>)
  void register_function(std::string name, std::vector<std::string> func_args_names = {}, custom_init_fn_t init_f = nullptr);

  template <auto f>
    requires(valid_function_type<decltype(f)>)
  void register_operator(std::string name, const std::string_view& properties_as, custom_init_fn_t init_f = nullptr);

  template <auto f, typename HT, is_valid_t<HT> vf = &is_valid<HT>>
    requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
  void register_operator(std::string name, const std::string_view& properties_as, custom_init_fn_t init_f = nullptr);

  template <auto f>
    requires(valid_function_type<decltype(f)>)
  void register_operator(std::string name, const operator_props& properties, custom_init_fn_t init_f = nullptr);

  template <auto f, typename HT, is_valid_t<HT> vf = &is_valid<HT>>
    requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
  void register_operator(std::string name, const operator_props& properties, custom_init_fn_t init_f = nullptr);

  template <auto f>
    requires(utils::is_function_v<decltype(f)>)
  void register_function_iter(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f = nullptr);

  template <auto f, typename HT, is_valid_t<HT> vf = &is_valid<HT>>
    requires(utils::is_function_v<decltype(f)> && valid_stack_type_v<HT>)
  void register_function_iter(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f = nullptr);

  void register_function(command_data data);

  template <typename T>
    requires (std::is_enum_v<T>)
  void register_enum(const std::span<std::tuple<std::string, T>>& values);

  template <typename T>
    requires (std::is_enum_v<T>)
  void register_enum(const std::span<std::tuple<std::string_view, T>>& values);

  template <typename T, typename F>
    requires (std::is_enum_v<T> && std::is_invocable_r_v<std::optional<T>, F, std::string_view>)
  void register_enum(F fn);

  template <typename RETURN_T, typename ROOT_T>
  container parse(std::string_view name, std::string_view text) const;

  std::tuple<tavl::event, tavl::error> parse(std::string_view name, tavl::parser& p, parse_context& ctx, container& c) const;

  template <typename RETURN_T, typename ROOT_T>
  std::tuple<tavl::event, tavl::error> parse(std::string_view name, tavl::parser& p, parse_context& ctx, container& c) const;

  // Pre-reserves the container's growable storage from the normalized block stream (before
  // codegen) to cut reallocations during compilation. Additive, so it also works correctly for
  // streaming multi-batch parses. `block_count` is the number of rpn blocks, `token_bytes` the
  // size of the token-text pool produced for this batch. Estimates are heuristic upper-ish
  // bounds; shrink_to_fit trims any slack afterwards.
  void reserve_from_hint(container* scr, const size_t block_count, const size_t token_bytes) const;

  void setup_block_description(
    parse_ctx* ctx,
    container* scr,
    const std::string_view& token,
    const std::string_view& custom_desc,
    const size_t start,
    const size_t cmd_start = SIZE_MAX,
    const container::description_node_kind kind = container::description_node_kind::unknown
  ) const;

  size_t push_basic_function(parse_ctx* ctx, container* scr, const basicf id, const int64_t arg) const;
  size_t push_string(parse_ctx* ctx, container* scr, const std::string_view &str) const;
  script_container::string_ref store_string(container* scr, const std::string_view& str) const;
  void compact_source_storage(container* scr) const;

  // Records that command `cmd` stores a compiled command index in its argument, so the peephole can
  // relocate it. `base` is SIZE_MAX for an absolute index, or the command the index is relative to.
  void record_cmd_index(parse_ctx* ctx, const size_t cmd, const uint8_t half = 0,
                        const size_t base = SIZE_MAX, const bool zero_is_absent = false) const;
  // Marks `count` slots starting at `first` as immediate data rather than instructions.
  void record_data_slots(parse_ctx* ctx, const size_t first, const size_t count) const;
  // Records a `context` push at `cmd`, unwound by the `erase` at `unwind` and occupying operand-stack
  // slot `scope_slot`, whose scope turned out to be dead (see parse_context::dead_context_push).
  void record_dead_context_push(parse_ctx* ctx, const size_t cmd, const size_t unwind, const int64_t scope_slot) const;

  // Post-codegen peephole over the finished command stream. Fuses scope unwinds, drops dead context
  // pushes and jumps that fall through, then relocates every stored command index and the
  // description ranges. Runs before build_description_index(); a no-op when optimizations are off.
  void optimize_commands(parse_ctx& ctx, container& scr) const;
  std::string_view static_string_arg(const command_block& block, const std::string_view& name) const;
  size_t push_enum_literal(parse_ctx* ctx, container* scr, const std::string_view& enum_type, const std::string_view& value) const;
  std::optional<int64_t> resolve_enum(const std::string_view& enum_type, const std::string_view& value) const;
  std::optional<int64_t> resolve_enum(const std::string_view& value) const;
  size_t dispatch_node(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view &override_lvalue = std::string_view()) const;
  size_t fold_block(parse_ctx* ctx, container* scr, const command_block& block, const basicf id) const;
  command_data::ftype get_token_type(const std::string_view& name) const;
  std::tuple<int32_t, int32_t, command_data::associativity, command_data::ftype> get_token_caps(const std::string_view& name) const;

  // Register this system's operators into a tavl parser (name + precedence + fixity + assoc), so
  // make_script_ast lexes/folds them correctly. Data-driven from mfuncs; `=`/`?=` (structural call
  // operators, not in mfuncs) are added at the lowest precedence.
  void configure_parser(tavl::parser& p) const;
  void scope_exit(parse_ctx* ctx, container* scr, const size_t count) const;
private:
  struct conversion_path {
    int32_t cost;
    std::vector<const conversion_data*> edges;
  };
  std::optional<conversion_path> find_conversion_path(std::string_view from, std::string_view to) const;
  const command_data* resolve_function(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view& name) const;

  uint64_t seed;
  enum safety safet;
  bool optimize;
  err_fn error;
  err_fn warning;
  // function name first; overloads are resolved by scope, argument types and conversion cost
  std::unordered_map<std::string, std::vector<command_data>> mfuncs;
  std::unordered_map<std::string, arithmetic_type_data> arithmetic_types;
  std::unordered_map<std::string, std::vector<conversion_data>> implicit_conversions;
  std::unordered_map<std::string, std::function<std::optional<int64_t>(std::string_view)>> enums;
  script_resolver_t script_resolver;
  // Raised only for the duration of init_math() / init_basic_functions(); stamped onto every
  // command_data they register so folding can tell built-ins from user registrations.
  bool registering_builtins = false;
};

}

#include "devils_script/system_templates.h"
