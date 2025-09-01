#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <string>
#include <format>
#include <iostream>
#include "devils_script/common.h"
#include "devils_script/type_traits.h"
#include "devils_script/text.h"
#include "devils_script/container.h"
#include "devils_script/basic_functions.h"
#include "devils_script/template_functions.h"
#include "devils_script/prng.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

// later I need:
// 1. different functions depending on scope
// 2. assert (?)
// 3. better callable for iterators (subblock in common.h ?)
// 4. ????

bool check_is_str_part_of(const std::string_view& big_str, const std::string_view& small_str) noexcept;

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
    enum class assigment_t : uint16_t { standart, validity_check };
    struct block { std::string_view token; size_t args_count; size_t size; };

    std::vector<block> output;
    std::vector<block> stack;
    std::vector<size_t> callstack;
    std::vector<size_t> text_stack;
    std::vector<std::string_view> operators;

    std::tuple<std::string_view, bool> find_rvalue_scope_function(const std::string_view& expr) const;
    void convert(const system* sys, const std::string_view& expr);
    void rearrange_to_poland_notation(const size_t start);
    size_t convert_block(const system* sys, const std::string_view& expr);
    std::tuple<std::string_view, size_t> convert_scope(const std::string_view& expr, block* arr, const size_t max_size) const;
    void clear();
  };

  struct command_block {
    std::span<rpn_conversion_ctx::block> data;

    command_block() noexcept;
    command_block(const std::span<rpn_conversion_ctx::block> &data) noexcept;
    command_block(const command_block& block, const size_t index) noexcept;
    command_block find(const std::string_view &name) const;
    command_block at(const size_t index) const;
    std::string_view name() const;
    size_t args_count() const;
    size_t size() const;
    bool empty() const;
  };

  struct parse_ctx {
    // another time?
    //struct lang_constants { std::string_view custom_description, condition, arg, ctx, count, percent, order_by; };

    function_type ftype;
    std::string_view expected_type;
    std::string_view string_upvalue;
    std::string_view scope_type_upvalue;
    size_t nest_level;
    size_t unlimited_func_index;
    size_t list_index_upvalue;
    size_t prev_chaining;

    std::vector<std::string_view> function_names;
    std::vector<int64_t> scope_stack;
    std::vector<std::string_view> stack_types;

    parse_ctx() noexcept;

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

  struct command_data {
    enum class ftype { operator_t, function_t, invalid };
    enum class associativity { left, right };
    enum class math_ftype { prefix = 1, binary, postfix };
    using init_fn_t = std::function<size_t(const system*, parse_ctx*, container*, const command_block&)>;

    std::string name;
    std::string_view expected_scope;
    std::string_view return_type;
    std::string function_signature;
    int32_t priority;
    int32_t arg_count; // or math_ftype
    associativity assoc;
    ftype type;
    init_fn_t init;
  };

  using custom_init_fn_t = std::function<void(const system*, parse_ctx*, container*, const command_block&, const std::vector<std::string> &)>;

  struct operator_props { int32_t priority; command_data::math_ftype mtype; command_data::associativity assoc; };

  class nest_level_changer {
  public:
    parse_ctx* ctx;
    nest_level_changer(parse_ctx* ctx) noexcept;
    ~nest_level_changer() noexcept;
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

  using argument_callback = std::function<void(parse_ctx*, container*, const size_t, const command_block&)>;

  using err_fn = std::function<void(const std::string &)>;
  enum class safety { unsafe, safe };
  struct options { uint64_t seed; enum safety safety; err_fn error; err_fn warning; options() noexcept; };
  system(const options &opts = options()) noexcept;
  void init_math();
  void init_basic_functions();

  void toggle_safety();
  bool safety() const;
  void raise_error(const std::string &msg) const;
  void raise_warning(const std::string& msg) const;
  uint64_t gen_value() const;
  uint64_t get_seed() const;
  void reseed(const uint64_t val);

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

  template <typename F, F f, typename HT, is_valid_t<HT> vf>
  void setup_description(parse_ctx* ctx, container* scr, const std::string_view& token) const;

  template <typename FROM, typename TO>
  void setup_type_conversion(parse_ctx* ctx, container* scr) const;

  // register_function with default arguments?

  template <typename F, F f, typename HT, is_valid_t<HT> vf = &is_valid<HT>>
    requires(valid_function_type<F> && valid_stack_type_v<HT>)
  void register_function(std::string name, std::vector<std::string> func_args_names = {}, custom_init_fn_t init_f = nullptr);

  template <typename F, F f>
    requires(valid_function_type<F>)
  void register_function(std::string name, std::vector<std::string> func_args_names = {}, custom_init_fn_t init_f = nullptr);

  template <typename F, F f, typename HT, on_effect_t<F, HT> eff, is_valid_t<HT> vf = &is_valid<HT>>
    requires(valid_function_type<F>&& valid_stack_type_v<HT>)
  void register_function(std::string name, std::vector<std::string> func_args_names = {}, custom_init_fn_t init_f = nullptr);

  template <typename F, F f, on_effect_t<F, scope_t<F>> eff>
    requires(valid_function_type<F>)
  void register_function(std::string name, std::vector<std::string> func_args_names = {}, custom_init_fn_t init_f = nullptr);

  template <typename F, F f, typename HT, is_valid_t<HT> vf = &is_valid<HT>>
    requires(valid_function_type<F>&& valid_stack_type_v<HT>)
  void register_operator(std::string name, const std::string_view &properties_as, custom_init_fn_t init_f = nullptr);

  template <typename F, F f>
    requires(valid_function_type<F>)
  void register_operator(std::string name, const std::string_view& properties_as, custom_init_fn_t init_f = nullptr);

  template <typename F, F f, typename HT, is_valid_t<HT> vf = &is_valid<HT>>
    requires(valid_function_type<F>&& valid_stack_type_v<HT>)
  void register_operator(std::string name, const operator_props& properties, custom_init_fn_t init_f = nullptr);

  template <typename F, F f>
    requires(valid_function_type<F>)
  void register_operator(std::string name, const operator_props& properties, custom_init_fn_t init_f = nullptr);

  template <typename F, F f, typename HT, is_valid_t<HT> vf = &is_valid<HT>>
    requires(utils::is_function_v<F> && valid_stack_type_v<HT>)
  void register_function_iter(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f = nullptr);

  template <typename F, F f>
    requires(utils::is_function_v<F>)
  void register_function_iter(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f = nullptr);

  template <typename T>
    requires (std::is_enum_v<T>)
  void register_enum(const std::span<std::tuple<std::string, T>>& values);

  template <typename T>
    requires (std::is_enum_v<T>)
  void register_enum(const std::span<std::tuple<std::string_view, T>>& values);

  template <typename RETURN_T, typename ROOT_T>
  container parse(std::string text);

  void setup_block_description(parse_ctx* ctx, container* scr, const std::string_view& token, const std::string_view& custom_desc, const size_t start) const;

  size_t push_basic_function(parse_ctx* ctx, container* scr, const basicf id, const int64_t arg) const;
  size_t push_string(parse_ctx* ctx, container* scr, const std::string_view &str) const;
  size_t parse_block(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view &override_lvalue = std::string_view()) const;
  size_t parse_block(parse_ctx* ctx, container* scr, const command_block& block, const basicf id) const;
  command_data::ftype get_token_type(const std::string_view& name) const;
  std::tuple<int32_t, int32_t, command_data::associativity, command_data::ftype> get_token_caps(const std::string_view& name) const;
  size_t patch_prev_functions_descriptions(container* scr, const size_t start) const;

  std::vector<std::string_view> make_operators_list() const;
  void scope_exit(parse_ctx* ctx, container* scr, const size_t count) const;
private:
  void check_is_str_part_of_and_throw(const std::string_view& big_str, const std::string_view& small_str) const;

  uint64_t seed; 
  enum safety safet; 
  err_fn error; 
  err_fn warning;
  mutable rpn_conversion_ctx rpn_ctx;
  mutable prng::xoshiro256starstar::state prng_s;
  std::unordered_map<std::string, command_data> mfuncs;
  std::unordered_map<std::string, std::unordered_map<std::string, size_t>> enums;
};







template <typename Arg>
size_t system::parse_args(
  parse_ctx* ctx, 
  container* scr, 
  const command_block& args, 
  const size_t offset, 
  const size_t index, 
  const std::vector<std::string>& func_args_names
) const {
  return parse_args<Arg>(ctx, scr, args, offset, index, func_args_names, nullptr);
}

template <typename Arg>
size_t system::parse_args(
  parse_ctx* ctx, 
  container* scr, 
  const command_block& block, 
  const size_t offset, 
  const size_t index, 
  const std::vector<std::string>& func_args_names, 
  const argument_callback& fn
) const {
  using cur_arg_type = final_stack_el_t<Arg>;
  constexpr bool is_optional = utils::is_optional_v<cur_arg_type>;

  const auto& curfname = ctx->function_names.back();

  if (block.empty()) return offset;
  if (command_block cb(block, offset); cb.name() == custom_description_constant) {
    return parse_args<Arg>(ctx, scr, block, offset + cb.size(), index, func_args_names, fn);
  }

  size_t next_size = 0;
  {
    std::string_view override_lvalue;
    command_block arg_block;
    if (!func_args_names.empty()) {
      if (index >= func_args_names.size()) return offset;
      const auto& name = func_args_names[index];
      if (name == custom_description_constant) raise_error(std::format("Using 'custom_description' as name of an argument is not allowed"));

      arg_block = block.find(name);
    } else {
      arg_block = command_block(block, offset);

      if (arg_block.empty()) return offset;
      if constexpr (!is_optional) {
        if (arg_block.empty()) raise_error(std::format("Could not find argument #{} for function '{}'", index, curfname));
        if constexpr (std::is_enum_v<cur_arg_type>) {
          if (arg_block.args_count() != 1 || arg_block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg #{}", curfname, index));
        }
      }
    }

    const auto& name = !func_args_names.empty() ? func_args_names[index] : std::string();
    next_size = parse_arg<Arg>(ctx, scr, arg_block, index, std::string_view(), override_lvalue, name, fn);
  }

  if (next_size == 0) return 0;
  if (ctx->is_ignore()) {
    if (command_block cb(block, offset + next_size); cb.empty() && index == 0) raise_error(std::format("Seems like there is no data except block that returns 'ignore_value' - it is very dangerous in this language design, function: '{}'", block.name()));
    while (ctx->pop_while_ignore()) {}
    return parse_args<Arg>(ctx, scr, block, offset + next_size, index, func_args_names, fn);
  } else {
    return parse_args<Arg>(ctx, scr, block, offset + next_size, index+1, func_args_names, fn);
  }
}

template <size_t I, size_t LI, size_t SIZE, typename F>
size_t system::parse_args(parse_ctx* ctx, container* scr, const command_block& args, const size_t offset, const std::vector<std::string>& func_args_names) const {
  return parse_args<I, LI, SIZE, F>(ctx, scr, args, offset, func_args_names, nullptr);
}

template <size_t I, size_t LI, size_t SIZE, typename F>
size_t system::parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const std::vector<std::string>& func_args_names, const argument_callback& fn) const {
  constexpr bool is_not_member_func = is_not_member_function<F>;
  using scope_type = scope_t<F>;
  constexpr bool requires_scope = !utils::is_void_v<scope_type>;
  constexpr size_t sig_args_count = utils::function_arguments_count<F>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  using cur_arg_type = final_stack_el_t<utils::function_argument_type<F, I>>;
  constexpr bool is_optional = utils::is_optional_v<cur_arg_type>;

  const auto& curfname = ctx->function_names.back();

  if (command_block cb(block, offset); cb.name() == custom_description_constant) {
    return parse_args<I, LI, F>(ctx, scr, block, offset + cb.size(), func_args_names, fn);
  }

  if constexpr (I < SIZE && !utils::is_void_v<cur_arg_type>) {
    if (block.empty()) raise_error(std::format("Not enought arguments for function '{}'", curfname));

    size_t next_size = 0;
    {
      std::string_view override_block_behaviour;
      command_block arg_block;
      if (!func_args_names.empty()) {
        if (LI >= args_count && LI >= func_args_names.size()) return 0;
        if (LI >= func_args_names.size()) raise_error(std::format("Function '{}' arguments names must be providen for every argument", curfname));
        const auto& name = func_args_names[LI];
        if (name == custom_description_constant) raise_error(std::format("Using 'custom_description' as name of an argument is not allowed"));

        arg_block = block.find(name);
        if (arg_block.empty()) raise_error(std::format("Could not find argument '{}' for function '{}'", name, curfname));
        if constexpr (std::is_enum_v<cur_arg_type>) {
          if (arg_block.args_count() != 1 || arg_block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg '{}'", curfname, name));
        }
      } else {
        arg_block = command_block(block, offset);
        next_size = arg_block.size();
        if (LI >= args_count && arg_block.empty()) return 0;
        if constexpr (!is_optional) {
          if (arg_block.empty()) raise_error(std::format("Could not find argument #{} for function '{}'", LI, curfname));
          if constexpr (std::is_enum_v<cur_arg_type>) {
            if (arg_block.args_count() != 1 || arg_block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg #{}", curfname, LI));
          }
        }
      }

      const auto& name = !func_args_names.empty() ? func_args_names[LI] : std::string();
      next_size = parse_arg<I, LI, F>(ctx, scr, arg_block, std::string_view(), override_block_behaviour, name, fn);
    }

    if (ctx->is_ignore()) {
      if (command_block cb(block, offset + next_size); cb.empty() && LI == 0) raise_error(std::format("Seems like there is no data except block that returns 'ignore_value' - it is very dangerous in this language design, function: '{}'", block.name()));
      while (ctx->pop_while_ignore()) {}
      return parse_args<I, LI, F>(ctx, scr, block, offset + next_size, func_args_names, fn);
    } else {
      return parse_args<I+1, LI+1, F>(ctx, scr, block, offset + next_size, func_args_names, fn);
    }
  }

  return offset;
}

template <size_t I, size_t LI, typename F>
size_t system::parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const std::vector<std::string>& func_args_names) const {
  constexpr size_t sig_args_count = utils::function_arguments_count<F>;
  return parse_args<I, LI, sig_args_count, F>(ctx, scr, block, offset, func_args_names, nullptr);
}

template <size_t I, size_t LI, typename F>
size_t system::parse_args(parse_ctx* ctx, container* scr, const command_block& block, const size_t offset, const std::vector<std::string>& func_args_names, const argument_callback& fn) const {
  constexpr size_t sig_args_count = utils::function_arguments_count<F>;
  return parse_args<I, LI, sig_args_count, F>(ctx, scr, block, offset, func_args_names, fn);
}

template <size_t I, size_t LI, typename F>
size_t system::parse_arg(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view& override_expected, const std::string_view& override_func, const std::string& arg_name, const argument_callback& fn) const {
  using cur_arg_type = final_stack_el_t<utils::function_argument_type<F, I>>;
  return parse_arg<cur_arg_type>(ctx, scr, block, LI, override_expected, override_func, arg_name, fn);
}

template <typename Arg>
size_t system::parse_arg(parse_ctx* ctx, container* scr, const command_block& block, const size_t index, const std::string_view& override_expected, const std::string_view& override_func, const std::string& arg_name, const argument_callback& fn) const {
  using cur_arg_type = final_stack_el_t<Arg>;
  constexpr auto cur_arg_type_name = utils::type_name<cur_arg_type>();
  constexpr bool is_optional = utils::is_optional_v<cur_arg_type>;

  if (block.empty()) return 0;
  if (block.name() == custom_description_constant) return block.size();

  const auto curfname = ctx->function_names.back();

  set_expected_type set(ctx, override_expected.empty() ? cur_arg_type_name : override_expected);
  nest_level_changer nlc(ctx);

  if constexpr (!is_optional) {
    if (!arg_name.empty()) {
      if (block.empty()) raise_error(std::format("Could not find argument '{}' for function '{}'", arg_name, curfname));
      if constexpr (std::is_enum_v<cur_arg_type>) {
        if (block.args_count() != 1 || block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg '{}'", curfname, arg_name));
      }
    } else {
      if (block.empty()) raise_error(std::format("Could not find argument #{} for function '{}'", index, curfname));
      if constexpr (std::is_enum_v<cur_arg_type>) {
        if (block.args_count() != 1 || block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg #{}", curfname, index));
      }
    }
  }
  
  std::string_view override_lvalue;
  if (!arg_name.empty()) {
    if constexpr (is_optional) {
      using opt_t = std::remove_cvref_t<utils::optional_value_t<cur_arg_type>>;
      if constexpr (std::is_enum_v<opt_t>) {
        if (block.args_count() != 1 || block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg '{}'", curfname, arg_name));
      }

      if constexpr (std::is_same_v<bool, opt_t>) override_lvalue = "AND";
      else if constexpr (std::is_fundamental_v<opt_t>) override_lvalue = "ADD";
      else if constexpr (std::is_same_v<std::string_view, opt_t>) override_lvalue = "__string_block";
      else override_lvalue = "__object_block";
    } else {
      if constexpr (std::is_same_v<bool, cur_arg_type>) override_lvalue = "AND";
      else if constexpr (std::is_fundamental_v<cur_arg_type>) override_lvalue = "ADD";
      else if constexpr (std::is_same_v<std::string_view, cur_arg_type>) override_lvalue = "__string_block";
      else override_lvalue = "__object_block";
    }
  }

  if (!override_func.empty()) override_lvalue = override_func;

  const size_t start = scr->block_descs.size();

  if constexpr (is_optional) {
    using cur_t = std::remove_cvref_t<utils::optional_value_t<cur_arg_type>>;
    set_expected_type set(ctx, utils::type_name<cur_t>());

    if (block.empty()) {
      push_basic_function(ctx, scr, basicf::pushinvalid, 0);
      ctx->push<cur_t>();
    } else {
      if constexpr (std::is_enum_v<cur_t>) { // special case, expecting rvalue with string
        const auto enum_str = command_block(block, 1).name();
        constexpr auto enum_name = utils::type_name<std::remove_cvref_t<cur_t>>();
        const auto itr = enums.find(std::string(enum_name));
        if (itr == enums.end()) raise_error(std::format("Could not find enum type '{}', did you registered them?", enum_name));
        const auto inner_itr = itr->second.find(std::string(enum_str));
        if (inner_itr == itr->second.end()) raise_error(std::format("Could not find value '{}' in registered enum type '{}'", enum_str, enum_name));
        push_basic_function(ctx, scr, basicf::pushint, std::bit_cast<int64_t>(inner_itr->second));
      } else parse_block(ctx, scr, block, override_lvalue);
    }
  } else {
    if constexpr (std::is_enum_v<cur_arg_type>) { // special case, expecting rvalue with string
      const auto enum_str = command_block(block, 1).name();
      constexpr auto enum_name = utils::type_name<std::remove_cvref_t<cur_arg_type>>();
      const auto itr = enums.find(std::string(enum_name));
      if (itr == enums.end()) raise_error(std::format("Could not find enum type '{}', did you registered them?", enum_name));
      const auto inner_itr = itr->second.find(std::string(enum_str));
      if (inner_itr == itr->second.end()) raise_error(std::format("Could not find value '{}' in registered enum type '{}'", enum_str, enum_name));
      push_basic_function(ctx, scr, basicf::pushint, std::bit_cast<int64_t>(inner_itr->second));
    } else parse_block(ctx, scr, block, override_lvalue);
  }

  if (!arg_name.empty()) { // special case - named argument description
    const auto cd = block.find(custom_description_constant);
    const auto cd_str = command_block(cd, 1).name();
    const auto tok = block.name() == "__empty_lvalue" ? override_lvalue : block.name();
    setup_block_description(ctx, scr, tok, cd_str, start);
  }

  if constexpr (std::is_fundamental_v<cur_arg_type>) {
    if (!ctx->is_ignore()) {
      if (ctx->is_bool()) setup_type_conversion<bool, cur_arg_type>(ctx, scr);
      if (ctx->is_integral()) setup_type_conversion<int64_t, cur_arg_type>(ctx, scr);
      if (ctx->is_number()) setup_type_conversion<double, cur_arg_type>(ctx, scr);
    }
  }

  if (!is_typeless_v<cur_arg_type> && !ctx->is_ignore() && !ctx->is<cur_arg_type>())
    raise_error(std::format("Could not parse argument {} for function '{}': function expects '{}', but got '{}'", index, curfname, cur_arg_type_name, ctx->top()));

  if (fn) std::invoke(fn, ctx, scr, index, block);

  return block.size();
}

template <size_t I, size_t LI, typename F>
size_t system::parse_arg(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view& override_expected, const basicf& override_block_behaviour, const std::string& arg_name, const argument_callback& fn) const {
  using cur_arg_type = final_stack_el_t<utils::function_argument_type<F, I>>;
  return parse_arg<cur_arg_type>(ctx, scr, block, LI, override_expected, override_block_behaviour, arg_name, fn);
}

template <typename Arg>
size_t system::parse_arg(parse_ctx* ctx, container* scr, const command_block& block, const size_t index, const std::string_view& override_expected, const basicf& override_block_behaviour, const std::string& arg_name, const argument_callback& fn) const {
  using cur_arg_type = final_stack_el_t<Arg>;
  constexpr auto cur_arg_type_name = utils::type_name<cur_arg_type>();
  constexpr bool is_optional = utils::is_optional_v<cur_arg_type>;

  if (block.empty()) return 0;
  if (block.name() == custom_description_constant) return block.size();

  const auto& curfname = ctx->function_names.back();

  set_expected_type set(ctx, override_expected.empty() ? cur_arg_type_name : override_expected);
  nest_level_changer nlc(ctx);

  if constexpr (!is_optional) {
    if (!arg_name.empty()) {
      if (block.empty()) raise_error(std::format("Could not find argument '{}' for function '{}'", arg_name, curfname));
      if constexpr (std::is_enum_v<cur_arg_type>) {
        if (block.args_count() != 1 || block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg '{}'", curfname, arg_name));
      }
    } else {
      if (block.empty()) raise_error(std::format("Could not find argument #{} for function '{}'", index, curfname));
      if constexpr (std::is_enum_v<cur_arg_type>) {
        if (block.args_count() != 1 || block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg #{}", curfname, index));
      }
    }
  }

  basicf local_override_block_behaviour;
  if (!arg_name.empty()) {
    if constexpr (is_optional) {
      using opt_t = std::remove_cvref_t<utils::optional_value_t<cur_arg_type>>;
      if constexpr (std::is_enum_v<opt_t>) {
        if (block.args_count() != 1 || block.size() != 2) raise_error(std::format("Bad enum special case in function '{}' arg '{}'", curfname, arg_name));
      }

      if constexpr (std::is_same_v<bool, opt_t>) local_override_block_behaviour = basicf::AND;
      else if constexpr (std::is_fundamental_v<opt_t>) local_override_block_behaviour = basicf::ADD;
      else if constexpr (std::is_same_v<std::string_view, opt_t>) local_override_block_behaviour = basicf::string_block;
      else local_override_block_behaviour = basicf::object_block;
    } else {
      if constexpr (std::is_same_v<bool, cur_arg_type>) local_override_block_behaviour = basicf::AND;
      else if constexpr (std::is_fundamental_v<cur_arg_type>) local_override_block_behaviour = basicf::ADD;
      else if constexpr (std::is_same_v<std::string_view, cur_arg_type>) local_override_block_behaviour = basicf::string_block;
      else local_override_block_behaviour = basicf::object_block;
    }
  }

  if (override_block_behaviour != basicf::invalid) local_override_block_behaviour = override_block_behaviour;

  const size_t start = scr->block_descs.size();

  if constexpr (is_optional) {
    using cur_t = std::remove_cvref_t<utils::optional_value_t<cur_arg_type>>;
    set_expected_type set(ctx, utils::type_name<cur_t>());

    if (block.empty()) {
      push_basic_function(ctx, scr, basicf::pushinvalid, 0);
      ctx->push<cur_t>();
    } else {
      if constexpr (std::is_enum_v<cur_t>) { // special case, expecting rvalue with string
        const auto enum_str = command_block(block, 1).name();
        constexpr auto enum_name = utils::type_name<std::remove_cvref_t<cur_t>>();
        const auto itr = enums.find(std::string(enum_name));
        if (itr == enums.end()) raise_error(std::format("Could not find enum type '{}', did you registered them?", enum_name));
        const auto inner_itr = itr->second.find(std::string(enum_str));
        if (inner_itr == itr->second.end()) raise_error(std::format("Could not find value '{}' in registered enum type '{}'", enum_str, enum_name));
        push_basic_function(ctx, scr, basicf::pushint, std::bit_cast<int64_t>(inner_itr->second));
      } else parse_block(ctx, scr, block, local_override_block_behaviour);
    }
  } else {
    if constexpr (std::is_enum_v<cur_arg_type>) { // special case, expecting rvalue with string
      const auto enum_str = command_block(block, 1).name();
      constexpr auto enum_name = utils::type_name<std::remove_cvref_t<cur_arg_type>>();
      const auto itr = enums.find(std::string(enum_name));
      if (itr == enums.end()) raise_error(std::format("Could not find enum type '{}', did you registered them?", enum_name));
      const auto inner_itr = itr->second.find(std::string(enum_str));
      if (inner_itr == itr->second.end()) raise_error(std::format("Could not find value '{}' in registered enum type '{}'", enum_str, enum_name));
      push_basic_function(ctx, scr, basicf::pushint, std::bit_cast<int64_t>(inner_itr->second));
    } else parse_block(ctx, scr, block, local_override_block_behaviour);
  }

  if (!arg_name.empty()) { // special case - named argument description
    const auto cd = block.find(custom_description_constant);
    const auto cd_str = command_block(cd, 1).name();
    const auto tok = block.name();
    setup_block_description(ctx, scr, tok, cd_str, start);
  }

  if constexpr (std::is_fundamental_v<cur_arg_type>) {
    if (!ctx->is_ignore()) {
      if (ctx->is_bool()) setup_type_conversion<bool, cur_arg_type>(ctx, scr);
      if (ctx->is_integral()) setup_type_conversion<int64_t, cur_arg_type>(ctx, scr);
      if (ctx->is_number()) setup_type_conversion<double, cur_arg_type>(ctx, scr);
    }
  }

  if (!is_typeless_v<cur_arg_type> && !ctx->is_ignore() && !ctx->is<cur_arg_type>())
    raise_error(std::format("Could not parse argument {} for function '{}': function expects '{}', but got '{}'", index, curfname, cur_arg_type_name, ctx->top()));

  if (fn) std::invoke(fn, ctx, scr, index, block);

  return block.size();
}

template<typename F, F f, typename HT, is_valid_t<HT> vf>
void system::setup_description(parse_ctx* ctx, container* scr, const std::string_view& token) const {
  constexpr bool is_not_member_func = is_not_member_function<F>;
  using scope_type = HT;
  constexpr bool requires_scope = !utils::is_void_v<scope_type>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  using first_argument = utils::function_argument_type<F, first_argument_index>;
  constexpr auto uftype = get_user_function_type<F>();
  constexpr size_t sig_args_count = utils::function_arguments_count<F>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  constexpr bool is_special_case = args_count == 1 && (std::is_same_v<std::string_view, first_argument> || std::is_same_v<std::string, first_argument>);
  constexpr bool change_scope_function = uftype == user_function_type::object && args_count == 0;
  using ret_type = std::remove_cvref_t<utils::function_result_type<F>>;
  constexpr auto ret_type_name = utils::type_name<ret_type>();

  if (scr->globals.empty()) raise_error(std::format("scr->globals is empty????"));

  std::string_view name = token;
  if (name == "__empty_lvalue") name = ctx->function_names.back();

  //const auto curfname = ctx->function_names.back();
  if (!check_is_str_part_of(scr->globals[0], name)) {
    const auto id = find_basicf(name);
    if (id == basicf::invalid) raise_error(std::format("Function name '{}' could not easily store at description struct, use another method", name));

    container::command_description desc(
      { static_cast<size_t>(id), SIZE_MAX }, args_count,
      requires_scope, is_not_member_func, !utils::is_void_v<ret_type>, ctx->nest_level, SIZE_MAX
    );
    scr->descs.emplace_back(desc);
  } else {
    //if (!fargs.empty()) check_is_str_part_of_and_throw(scr->globals[0], fargs);

    const size_t fname_start = name.data() - scr->globals[0].data();
    const size_t fname_size = name.size();

    container::command_description desc(
      { fname_start, fname_size }, args_count,
      requires_scope, is_not_member_func, !utils::is_void_v<ret_type>, ctx->nest_level, SIZE_MAX
    );
    scr->descs.emplace_back(desc);
  }

  if (scr->descs.size() != scr->cmds.size())
    raise_error(std::format("After parsing function '{}' script description array size {} and commands array size {} are not equal", token, scr->descs.size(), scr->cmds.size()));

  if (scr->get_string(scr->descs.back().name.start, scr->descs.back().name.count) != name)
    raise_error(std::format("After parsing function '{}' script last description '{}' is not same as command name '{}'", token, scr->get_string(scr->descs.back().name.start, scr->descs.back().name.count), token));
}

template <typename FROM, typename TO>
void system::setup_type_conversion(parse_ctx* ctx, container* scr) const {
  if (std::is_same_v<FROM, TO>) return;
  if (!std::is_fundamental_v<FROM> || !std::is_fundamental_v<TO>) raise_error(std::format("Could not convert from '{}' to '{}'", utils::type_name<FROM>(), utils::type_name<TO>()));

  const function_t fs[] = { &convert_unsafe<FROM, TO>, &convert<FROM, TO> };
  scr->cmds.push_back(container::command(fs[size_t(safety())], 0ll));

  container::command_description desc(
    { static_cast<size_t>(basicf::conversion), SIZE_MAX},
    1, false, true, true, ctx->nest_level, SIZE_MAX
  );
  scr->descs.emplace_back(desc);

  if (!ctx->is<FROM>()) raise_error(std::format("Wrong FROM type '{}' - stack last type is '{}'", ctx->stack_types.back(), utils::type_name<std::remove_cvref_t<FROM>>()));
  ctx->stack_types.back() = utils::type_name<final_stack_el_t<TO>>();
}

template <typename F, F f, typename HT, is_valid_t<HT> vf>
  requires(valid_function_type<F> && valid_stack_type_v<HT>)
void system::register_function(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  register_function<F, f, HT, on_effect_t<F, HT>(nullptr), vf>(std::move(name), std::move(func_args_names), std::move(init_f));
}

template <typename F, F f>
  requires(valid_function_type<F>)
void system::register_function(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using scope_type = scope_t<F>;
  constexpr auto eff_ptr = on_effect_t<F, scope_type>(nullptr);
  constexpr auto val_ptr = &is_valid<scope_type>;
  register_function<F, f, scope_type, eff_ptr, val_ptr>(std::move(name), std::move(func_args_names), std::move(init_f));
}

template <typename F, F f, typename HT, on_effect_t<F, HT> eff, is_valid_t<HT> vf>
  requires(valid_function_type<F> && valid_stack_type_v<HT>)
void system::register_function(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  const auto name_view = std::string_view(name);
  if (!text::is_valid_function_name(name_view)) raise_error(std::format("'{}' is not valid function name", name_view));

  const auto itr = mfuncs.find(name);
  if (itr != mfuncs.end()) raise_error(std::format("Function '{}' is already registered", name));

  auto func = [func_args_names = std::move(func_args_names), init_f = std::move(init_f)]
    (const system* sys, parse_ctx* ctx, container* scr, const command_block& args) -> size_t 
  {
    using memder_of = utils::function_member_of<F>;
    using scope_type = std::conditional_t<utils::is_void_v<HT>, memder_of, std::remove_cvref_t<HT>>;
    using ret_type = final_stack_el_t<utils::function_result_type<F>>;
    constexpr bool is_not_member_func = utils::is_void_v<memder_of>;
    constexpr bool requires_scope = !utils::is_void_v<scope_type>;
    constexpr size_t sig_args_count = utils::function_arguments_count<F>;
    constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
    constexpr auto uftype = get_user_function_type<F>();
    constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
    using first_argument = final_stack_el_t<utils::function_argument_type<F, first_argument_index>>;
    constexpr size_t function_args_count = args_count;
    constexpr bool unlimited_args = args_count == 2 &&
      std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_argument_type<F, first_argument_index + 1>>>&&
      std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_result_type<F>>>;
      //&& std::is_fundamental_v<utils::function_argument_type<F, first_argument_index>>; // more types?
    constexpr auto scope_type_name = utils::type_name<scope_type>();
    const auto curfname = ctx->function_names.back();

    const bool expected_void = ctx->expected_type == utils::type_name<void>();
    if (uftype == user_function_type::effect && !expected_void) sys->raise_error(std::format("Function effect '{}' called in not effect context", curfname));
    if (!utils::is_void_v<scope_type> && !ctx->is_scope<scope_type>())
      sys->raise_error(std::format("Trying to call function '{}' in wrong scope context: {} != {}", curfname, ctx->current_scope_type(), scope_type_name));

    // standart function when '!init_f' ?
    if (!init_f) {
      size_t curpos = scr->descs.size();

      // useless checking
      /*if (type_is_object(ctx->expected_type) && !type_is_any_type_object(ctx->expected_type) && !type_is_any_type(ctx->expected_type)) {
        if (ctx->expected_type != utils::type_name<ret_type>()) sys->raise_error(std::format("Expected type '{}' is not same as return type '{}'", ctx->expected_type, utils::type_name<ret_type>()));
      }*/

      //if (type_is_string(ctx->expected_type) && !type_is_string(utils::type_name<ret_type>()))
      //  sys->raise_error(std::format("Expected type '{}' is not same as return type '{}'", ctx->expected_type, utils::type_name<ret_type>()));

      if constexpr (!unlimited_args) {
        if (args_count != 0 && args.args_count() > args_count) sys->raise_error(std::format("Too many arguments for function '{}'", args.name()));
      }

      int64_t scope_index = INT64_MAX;
      if constexpr (requires_scope) {
        scope_index = ctx->scope_stack.back();
        if (ctx->current_scope_type() != utils::type_name<element_view>() && !ctx->is_scope<scope_type>())
          sys->raise_error(std::format("Stack index {} contains '{}' type, but function '{}' required '{}' type", scope_index, ctx->stack_types[scope_index], curfname, scope_type_name));
        if (ctx->current_scope_type() == utils::type_name<element_view>()) {
          if (!ctx->scope_type_upvalue.empty() && ctx->scope_type_upvalue != scope_type_name) sys->raise_error(std::format("Several functions with different scopes found? prev: '{}' cur: '{}'", ctx->scope_type_upvalue, scope_type_name));
          ctx->scope_type_upvalue = scope_type_name;
        }
      }

      std::vector<size_t> jumps;
      if constexpr (unlimited_args) {
        const size_t prev_index = ctx->unlimited_func_index;
        ctx->unlimited_func_index = ctx->function_names.size() - 1;

        // command_block b(args, 1) is first argument
        sys->parse_args<first_argument>(ctx, scr, args, 1, 0, func_args_names, [&](parse_ctx* ctx, container* scr, const size_t index, const command_block& arg) {
          if (index == 0) return;

          constexpr function_t fs[] = { &userfunc_unsafe<F, f, HT, vf, eff>, &userfunc<F, f, HT, vf, eff> };
          scr->cmds.push_back(container::command(fs[size_t(sys->safety())], scope_index));
          sys->setup_description<F, f, HT, vf>(ctx, scr, args.name());
          sys->patch_prev_functions_descriptions(scr, curpos);
          ctx->pop();
          ctx->pop();
          ctx->push<ret_type>();
          curpos = scr->cmds.size();
        });

        ctx->unlimited_func_index = prev_index;
      } else {
        size_t offset = 1;
        offset = sys->parse_args<first_argument_index, 0, F>(ctx, scr, args, offset, func_args_names);
        constexpr function_t fs[] = { &userfunc_unsafe<F, f, HT, vf, eff>, &userfunc<F, f, HT, vf, eff> };
        scr->cmds.push_back(container::command(fs[size_t(sys->safety())], scope_index));
        sys->setup_description<F, f, HT, vf>(ctx, scr, args.name());
        sys->patch_prev_functions_descriptions(scr, curpos);

        if constexpr (std::is_same_v<scope_type, internal::thisctxlist>) {
          if (ctx->list_index_upvalue == SIZE_MAX) sys->raise_error(std::format("Trying to use list function '{}' without list context on stack", curfname));
          const size_t id = ctx->list_index_upvalue;
          if (scr->lists[id].type.empty()) {
            scr->lists[id].type = ctx->top();
          } else {
            if (scr->lists[id].type != ctx->top()) sys->raise_error(std::format("Wrong type for list #{}^ expected '{}', got '{}'", id, scr->lists[id].type, ctx->top()));
          }
        }

        for (size_t i = 0; i < args_count; ++i) { ctx->pop(); }

        const size_t stack_size = ctx->stack_types.size();
        if constexpr (!utils::is_void_v<ret_type>) ctx->push<ret_type>();
        //const auto remaining = command_block(args, offset);
        if constexpr (uftype == user_function_type::object) {
          // 'offset < args.size()' has more sense than 'ctx->ftype == function_type::lvalue'
          // parse scope block AFTER the scope function 
          if (offset < args.size() && ctx->ftype == function_type::lvalue) {
            ctx->scope_stack.push_back(ctx->stack_types.size()-1);
            while (offset < args.size()) {
              const auto remaining = command_block(args, offset);
              offset += remaining.size();

              sys->parse_block(ctx, scr, remaining);
              while (ctx->pop_while_ignore()) {}
            }
            sys->scope_exit(ctx, scr, 1);
          }
        }

        // no value?
        if (stack_size == ctx->stack_types.size()) ctx->push<ignore_value>();
      }

      for (const auto i : jumps) { scr->cmds[i].arg = scr->cmds.size(); }
    } else std::invoke(init_f, sys, ctx, scr, args, func_args_names);

      
    // additional checks?
    return args.size();
  };

  using scope_type = std::remove_cvref_t<HT>;
  using ret_type = final_stack_el_t<utils::function_result_type<F>>;
  constexpr bool is_not_member_func = utils::is_void_v<utils::function_member_of<F>>;
  constexpr bool requires_scope = !utils::is_void_v<scope_type>;
  constexpr size_t sig_args_count = utils::function_arguments_count<F>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  constexpr auto uftype = get_user_function_type<F>();
  constexpr auto scope_type_name = utils::type_name<scope_type>();
  constexpr auto parse_ftype = command_data::ftype::function_t;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  constexpr bool unlimited_args = args_count == 2 &&
    std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_argument_type<F, first_argument_index + 1>>>&&
    std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_result_type<F>>>;
  //&& std::is_fundamental_v<utils::function_argument_type<F, first_argument_index>>;

  command_data mcd{
    name, scope_type_name, utils::type_name<ret_type>(), utils::make_function_sig_string<F>(), 
    15, unlimited_args ? -1 : int32_t(args_count), command_data::associativity::right, parse_ftype,
    std::move(func)
  };

  mfuncs[name] = std::move(mcd);
}

template <typename F, F f, on_effect_t<F, scope_t<F>> eff>
  requires(valid_function_type<F>)
void system::register_function(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using scope_type = scope_t<F>;
  register_function<F, f, scope_type, eff, &is_valid<scope_type>>(std::move(name), std::move(func_args_names), std::move(init_f));
}

template <typename F, F f, typename HT, is_valid_t<HT> vf>
  requires(valid_function_type<F>&& valid_stack_type_v<HT>)
void system::register_operator(std::string name, const std::string_view& properties_as, custom_init_fn_t init_f) {
  const auto itr = mfuncs.find(std::string(properties_as));
  if (itr == mfuncs.end()) raise_error(std::format("Could not find function '{}'", properties_as));

  operator_props ps;
  ps.priority = itr->second.priority;
  ps.assoc = itr->second.assoc;
  ps.mtype = static_cast<decltype(ps.mtype)>(itr->second.arg_count);

  register_operator<F, f, HT, vf>(std::move(name), ps, std::move(init_f));
}

template <typename F, F f>
  requires(valid_function_type<F>)
void system::register_operator(std::string name, const std::string_view& properties_as, custom_init_fn_t init_f) {
  using scope_type = scope_t<F>;
  register_operator<F, f, scope_type, &is_valid<scope_type>>(std::move(name), properties_as, std::move(init_f));
}

template <typename F, F f, typename HT, is_valid_t<HT> vf>
  requires(valid_function_type<F> && valid_stack_type_v<HT>)
void system::register_operator(std::string name, const operator_props& ps, custom_init_fn_t init_f) {
  using scope_type = std::remove_cvref_t<HT>;
  using ret_type = final_stack_el_t<utils::function_result_type<F>>;
  constexpr bool is_not_member_func = utils::is_void_v<utils::function_member_of<F>>;
  constexpr auto scope_type_name = utils::type_name<scope_type>();
  constexpr auto uftype = get_user_function_type<F>();
  constexpr bool requires_scope = !utils::is_void_v<scope_type>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  using first_argument = final_stack_el_t<utils::function_argument_type<F, first_argument_index>>;
  constexpr size_t sig_args_count = utils::function_arguments_count<F>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  constexpr size_t function_args_count = args_count;
  constexpr auto parse_ftype = command_data::ftype::operator_t;
  constexpr bool unlimited_args = args_count == 2 &&
    std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_argument_type<F, first_argument_index + 1>>>&&
    std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_result_type<F>>>;
    //std::is_fundamental_v<utils::function_argument_type<F, first_argument_index>>;

  static_assert(args_count == 1 || args_count == 2, "Operators must have only 1 or 2 arguments");
  static_assert(!utils::is_void_v<ret_type> && !std::is_same_v<ret_type, ignore_value>, "Operators must have a proper return value");
  // is args must be fundamental? i think not
  // is args must be same type? probably not

  const auto name_view = std::string_view(name);
  const bool valid_name = text::is_valid_operator_name(name_view);
  if (!valid_name) raise_error(std::format("'{}' is not valid function name", name_view));
  if (text::is_valid_function_name(name_view) && text::is_special_operator(name_view)) 
    raise_error(std::format("Do not mix math operator symbols and common function name"));

  const auto itr = mfuncs.find(name);
  if (itr != mfuncs.end()) raise_error(std::format("Function '{}' is already registered", name));

  command_data mcd{
    name, scope_type_name, utils::type_name<ret_type>(), utils::make_function_sig_string<F>(), ps.priority, static_cast<int32_t>(ps.mtype), ps.assoc, parse_ftype,
    [init_f = std::move(init_f)](const system* sys, parse_ctx* ctx, container* scr, const command_block& args) -> size_t {
      constexpr auto scope_type_name = utils::type_name<scope_type>();
      const auto curfname = ctx->function_names.back();

      const bool expected_void = ctx->expected_type == utils::type_name<void>();
      if (expected_void) sys->raise_error(std::format("Operator '{}' called in effect context", curfname));

      if (!init_f) {
        int64_t scope_index = INT64_MAX;
        if constexpr (requires_scope) {
          scope_index = ctx->scope_stack.back();
          if (!ctx->is_scope<scope_type>()) sys->raise_error(std::format("Stack index {} contains '{}' type, but function '{}' required '{}' type", scope_index, ctx->current_scope_type(), curfname, scope_type_name));
        }

        if constexpr (!unlimited_args) {
          if (args.args_count() > args_count) sys->raise_error(std::format("Too many arguments for function '{}'", args.name()));
        }

        std::vector<size_t> jumps;
        if constexpr (unlimited_args) {
          size_t curpos = scr->cmds.size();
          sys->parse_args<first_argument>(ctx, scr, args, 1, 0, {}, [&](parse_ctx* ctx, container* scr, const size_t index, const command_block& arg) {
            if (index == 0) return;

            constexpr function_t fs[] = { &mathfunc_unsafe<F, f, HT, vf>, &mathfunc<F, f, HT, vf> };
            scr->cmds.push_back(container::command(fs[size_t(sys->safety())], scope_index));
            sys->setup_description<F, f, HT, vf>(ctx, scr, args.name());
            sys->patch_prev_functions_descriptions(scr, curpos);
            ctx->pop();
            ctx->pop();
            ctx->push<ret_type>();
            curpos = scr->cmds.size();
          });
        } else {
          size_t curpos = scr->cmds.size();
          size_t offset = 1;
          offset = sys->parse_args<first_argument_index, 0, F>(ctx, scr, args, offset, {});
          constexpr function_t fs[] = { &mathfunc_unsafe<F, f, HT, vf>, &mathfunc<F, f, HT, vf> };
          scr->cmds.push_back(container::command(fs[size_t(sys->safety())], scope_index));
          sys->setup_description<F, f, HT, vf>(ctx, scr, args.name());
          sys->patch_prev_functions_descriptions(scr, curpos);

          for (size_t i = 0; i < args_count; ++i) { ctx->pop(); }
          ctx->push<ret_type>();
          if constexpr (uftype == user_function_type::object) {
            // parse scope block AFTER the scope function 
            if (ctx->ftype == function_type::lvalue) {
              ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
              sys->parse_block(ctx, scr, command_block(args, offset));
              sys->scope_exit(ctx, scr, 1);
            }
          }
        }

        for (const auto i : jumps) { scr->cmds[i].arg = scr->cmds.size(); }
      } else std::invoke(init_f, sys, ctx, scr, args, std::vector<std::string>{});

      
      // additional checks?
      return args.size();
    }
  };

  mfuncs[name] = std::move(mcd);
}

template <typename F, F f>
  requires(valid_function_type<F>)
void system::register_operator(std::string name, const operator_props& properties, custom_init_fn_t init_f) {
  using scope_type = scope_t<F>;
  register_operator<F, f, scope_type, &is_valid<scope_type>>(std::move(name), properties, std::move(init_f));
}


template<typename F, F f, typename HT, is_valid_t<HT> vf>
  requires(utils::is_function_v<F> && valid_stack_type_v<HT>)
void system::register_function_iter(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using scope_type = std::remove_cvref_t<HT>;
  constexpr auto scope_type_name = utils::type_name<scope_type>();
  constexpr auto uftype = get_user_function_type<F, utils::function_result_type<F>, true>();
  constexpr auto parse_ftype = command_data::ftype::function_t;
  using ret_type = final_stack_el_t<utils::function_result_type<F>>;

  const auto name_view = std::string_view(name);
  if (!text::is_valid_function_name(name_view)) raise_error(std::format("'{}' is not valid function name", name_view));
  const auto itr = mfuncs.find(name);
  if (itr != mfuncs.end()) raise_error(std::format("Function '{}' is already registered", name));

  command_data cd{
    name, scope_type_name, utils::type_name<ret_type>(), utils::make_function_sig_string<F>(), 15, 50, command_data::associativity::right, parse_ftype,
    [func_args_names = std::move(func_args_names), init_f = std::move(init_f)]
      (const system* sys, parse_ctx* ctx, container* scr, const command_block& args)
    {
      constexpr bool is_not_member_func = is_not_member_function<F>;
      using scope_type = HT;
      constexpr bool requires_scope = !utils::is_void_v<scope_type>;
      constexpr auto scope_type_name = utils::type_name<scope_type>();
      constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
      using first_argument = std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>;
      constexpr auto uftype = get_user_function_type<F, utils::function_result_type<F>, true>();
      constexpr size_t sig_args_count = utils::function_arguments_count<F>;
      constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
      const auto curfname = ctx->function_names.back();
      using ret_type = final_stack_el_t<utils::function_result_type<F>>;

      if constexpr (!is_valid_argument_function_v<first_argument>) sys->raise_error(std::format("At least one argument must be a function in iterator function '{}'", curfname));

      int64_t scope_index = INT64_MAX;
      if (!init_f) {
        if constexpr (requires_scope) {
          scope_index = ctx->scope_stack.back();
          if (scope_type_name != ctx->stack_types[scope_index]) sys->raise_error(std::format("Stack index {} contains '{}' type, but function '{}' required '{}' type", scope_index, ctx->stack_types[scope_index], curfname, scope_type_name));
        }
      }

      const bool expected_void = type_is_void(ctx->expected_type);
      if (expected_void && uftype != user_function_type::iterator_effect) sys->raise_error(std::format("Effect iterator is outside of effect scriptblock?"));

      bool has_count = false;

      if (init_f) {
        std::invoke(init_f, sys, ctx, scr, args, func_args_names);
      } else {
        constexpr function_t fs[] = { &useriter_unsafe<F, f, HT, vf>, &useriter<F, f, HT, vf> };
        scr->cmds.emplace_back(container::command(fs[size_t(sys->safety())], scope_index));
        sys->setup_description<F, f, HT, vf>(ctx, scr, args.name());

        std::vector<size_t> jumps;
        utils::static_for<args_count>([&](auto index) {
          const size_t jump_index = sys->push_basic_function(ctx, scr, basicf::jump, 0);
          jumps.push_back(jump_index);
        });

        //const size_t jump_end = sys->push_basic_function(ctx, scr, basicf::jump, 0); // is needed?

        size_t section_start = scr->cmds.size();
        utils::static_for<args_count>([&](auto index) {
          constexpr size_t cur_index = first_argument_index + index;
          using fn_t = std::remove_cvref_t<utils::function_argument_type<F, cur_index>>;
          if constexpr (!is_valid_argument_function_v<fn_t>)
            sys->raise_error(std::format("All arguments for iterator '{}' expected to be a function argument", curfname));
          using value_type = std::remove_cvref_t<utils::function_result_type<fn_t>>;
          using input_type = std::remove_cvref_t<utils::function_argument_type<fn_t, 0>>;

          if (index >= func_args_names.size()) sys->raise_error(std::format("Not enought arg names for function '{}'", curfname));
          const auto& name = func_args_names[index];

          const auto child = args.find(name);
          // child.empty() means nullptr to std function
          if (child.empty()) { return; }

          ctx->push<input_type>();
          ctx->scope_stack.push_back(ctx->stack_types.size()-1);
          sys->parse_arg<value_type>(ctx, scr, child, index, std::string_view(), std::string_view(), name);
          sys->scope_exit(ctx, scr, 1);

          if constexpr (!utils::is_void_v<value_type>) {
            if (ctx->top() != utils::type_name<value_type>()) throw std::runtime_error(std::format("Function's '{}' child '{}' returns type '{}', but '{}' was expected", curfname, child.name(), ctx->top(), utils::type_name<value_type>()));
          }

          ctx->pop();

          scr->cmds[jumps[index]].arg = scr->cmds.size();
          section_start = scr->cmds.size();
        });

        ctx->push<ret_type>();
      }

      return args.size();
    }
  };

  mfuncs[name] = std::move(cd);
}

template <typename F, F f>
  requires(utils::is_function_v<F>)
void system::register_function_iter(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using scope_type = scope_t<F>;
  register_function_iter<F, f, scope_type, &is_valid<scope_type>>(std::move(name), std::move(func_args_names), std::move(init_f));
}

template <typename T>
  requires (std::is_enum_v<T>)
void system::register_enum(const std::span<std::tuple<std::string, T>>& values) {
  using enum_t = std::remove_cvref_t<T>;
  constexpr auto enum_name = std::string(utils::type_name<enum_t>());
  const auto itr = enums.find(enum_name);
  if (itr != enums.end()) raise_error(std::format("Enum '{}' is already registered", enum_name));

  for (const auto &[name, val] : values) {
    itr->second[name] = std::bit_cast<size_t>(val);
  }
}

template <typename T>
  requires (std::is_enum_v<T>)
void system::register_enum(const std::span<std::tuple<std::string_view, T>>& values) {
  using enum_t = std::remove_cvref_t<T>;
  constexpr auto enum_name = std::string(utils::type_name<enum_t>());
  const auto itr = enums.find(enum_name);
  if (itr != enums.end()) raise_error(std::format("Enum '{}' is already registered", enum_name));

  for (const auto& [name, val] : values) {
    itr->second[std::string(name)] = std::bit_cast<size_t>(val);
  }
}

template <typename RETURN_T, typename ROOT_T>
container system::parse(std::string text) {
  using ret_type = final_stack_el_t<RETURN_T>;
  using root_type = final_stack_el_t<ROOT_T>;

  container scr;
  parse_ctx ctx;
  scr.globals.emplace_back(std::move(text));
  scr.prng_state = gen_value();
  const auto script_block = std::string_view(scr.globals[0]);

  rpn_ctx.operators = make_operators_list();
  auto cmds = rpn_ctx.convert_block(this, script_block);
  auto output = rpn_ctx.output;
  rpn_ctx.clear();

       if constexpr (utils::is_void_v<ret_type>)                 output.emplace(output.begin(), rpn_conversion_ctx::block{ "__effect_block", cmds, output.size()+1 });
  else if constexpr (std::is_same_v<bool, ret_type>)             output.emplace(output.begin(), rpn_conversion_ctx::block{ "AND", cmds, output.size()+1 });
  else if constexpr (std::is_fundamental_v<ret_type>)            output.emplace(output.begin(), rpn_conversion_ctx::block{ "ADD", cmds, output.size()+1 });
  else if constexpr (std::is_same_v<std::string_view, ret_type>) output.emplace(output.begin(), rpn_conversion_ctx::block{ "__string_block", cmds, output.size()+1 });
  else                                                           output.emplace(output.begin(), rpn_conversion_ctx::block{ "__object_block", cmds, output.size()+1 });

  /*const auto print_block = [](const std::vector<system::rpn_conversion_ctx::block>& arr) {
    for (const auto& b : arr) {
      std::cout << std::format("({},{},{}) ", b.token, b.args_count, b.size);
    }
    std::cout << "\n";
    std::cout << "size: " << arr.size() << "\n";
  };

  print_block(output);*/

  ctx.unlimited_func_index = 0;
  ctx.nest_level = 0;
  ctx.expected_type = utils::type_name<ret_type>();
  set_function_type sft(&ctx, function_type::lvalue);

  auto script_cmds = command_block(std::span<rpn_conversion_ctx::block>(output));

  if constexpr (!utils::is_void_v<root_type>) {
    constexpr auto root_name = utils::type_name<root_type>();
    scr.args.push_back({ { static_cast<size_t>(basicf::root), SIZE_MAX }, root_name});
    push_basic_function(&ctx, &scr, basicf::pushroot, 0);
    ctx.scope_stack.push_back(ctx.stack_types.size()-1);
  }

  {
    set_expected_type set(&ctx, utils::type_name<ret_type>());
    parse_block(&ctx, &scr, script_cmds);
  }

  while (ctx.pop_while_ignore()) {}

  if constexpr (!utils::is_void_v<root_type>) {
    if (ctx.scope_stack.size() != 1) raise_error(std::format("There is not closed scope of type '{}' on stack", ctx.stack_types[ctx.scope_stack.back()]));
    scope_exit(&ctx, &scr, 1);
  }

  if constexpr (!utils::is_void_v<ret_type>) {
    constexpr auto return_name = utils::type_name<ret_type>();
    if (ctx.top() != return_name) raise_error(std::format("Invalid return type '{}' expected '{}', stack size {}", ctx.stack_types.back(), return_name, ctx.stack_types.size()));
    push_basic_function(&ctx, &scr, basicf::pushreturn, 0);
  }

  if (ctx.stack_types.size() != 0) raise_error(std::format("Script is not properly ended, {} values on stack", ctx.stack_types.size()));

  return scr;
}

template <typename T>
bool system::parse_ctx::is_scope() const {
  using basic_T = final_stack_el_t<T>;
  if constexpr (std::is_pointer_v<basic_T>) {
    using no_ptr_t = std::remove_cvref_t<std::remove_pointer_t<basic_T>>;
    return current_scope_type() == utils::type_name<no_ptr_t*>() || current_scope_type() == utils::type_name<const no_ptr_t*>();
  } else {
    return current_scope_type() == utils::type_name<basic_T>();
  }
}

template <typename T>
bool system::parse_ctx::is() const {
  using basic_T = final_stack_el_t<T>;
  if constexpr (std::is_pointer_v<basic_T>) {
    using no_ptr_t = std::remove_cvref_t<std::remove_pointer_t<basic_T>>;
    return top() == utils::type_name<no_ptr_t*>() || top() == utils::type_name<const no_ptr_t*>();
  } else {
    return top() == utils::type_name<basic_T>();
  }
}

template <typename T>
void system::parse_ctx::push() {
  using basic_T = final_stack_el_t<T>;
  push(utils::type_name<basic_T>());
}

// not needed anymore?
template <typename F, typename RT, bool is_iterator_func>
constexpr system::user_function_type system::get_user_function_type() {
  using ret_type = final_stack_el_t<std::conditional_t<is_iterator_func, RT, utils::function_result_type<F>>>;
  user_function_type t = user_function_type::effect;
  if constexpr (is_iterator_func) {
    if constexpr (utils::is_void_v<ret_type>) t = user_function_type::iterator_effect;
    else if constexpr (std::is_same_v<bool, ret_type>) t = user_function_type::iterator_condition;
    else if constexpr (std::is_fundamental_v<ret_type>) t = user_function_type::iterator_arithmetic;
    else raise_error(std::format("Iterator that returns '{}' is not supported", utils::type_name<ret_type>()));
  } else {
    if constexpr (utils::is_void_v<ret_type>) t = user_function_type::effect;
    else if constexpr (std::is_same_v<bool, ret_type>) t = user_function_type::condition;
    else if constexpr (std::is_fundamental_v<ret_type>) t = user_function_type::arithmetic;
    else if constexpr (std::is_same_v<std::string_view, ret_type>) t = user_function_type::string;
    else t = user_function_type::object;
  }
  return t;
}

constexpr std::string_view system::get_user_function_type_name(const user_function_type t) {
  switch (t) {
    case user_function_type::effect: return "effect";
    case user_function_type::condition: return "condition";
    case user_function_type::arithmetic: return "arithmetic";
    case user_function_type::string: return "string";
    case user_function_type::object: return "scope";
    case user_function_type::iterator_effect: return "iterator_effect";
    case user_function_type::iterator_condition: return "iterator_condition";
    case user_function_type::iterator_arithmetic: return "iterator_arithmetic";
  }
  return std::string_view();
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}