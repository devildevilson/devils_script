#pragma once

// Template implementation of `system` registration and parse helpers.
//
// This file is included from `system.h` because most registration code depends on the exact
// user callback type. It performs parse-time argument checking, emits call instructions,
// registers function/operator metadata, and provides typed `system::parse` entry points.
//
// The code intentionally keeps semantic type information in `parse_ctx::stack_types` while
// emitting commands. That lets the compiler validate script blocks before execution and lets
// safe/unsafe runtime opcodes share the same generated command stream.

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

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
      }
    }

    const auto& name = !func_args_names.empty() ? func_args_names[index] : std::string();
    next_size = parse_arg<Arg>(ctx, scr, arg_block, index, std::string_view(), std::string_view(), name, fn);
  }

  if (next_size == 0) return 0;
  if (ctx->is_ignore()) {
    if (command_block cb(block, offset + next_size); cb.empty() && index == 0) 
      raise_error(std::format("Seems like there is no data except block that returns 'ignore_value' - it is very dangerous in this language design, function: '{}'", block.name()));
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
      } else {
        arg_block = command_block(block, offset);
        next_size = arg_block.size();
        if (LI >= args_count && arg_block.empty()) return 0;
        if constexpr (!is_optional) {
          if (arg_block.empty()) raise_error(std::format("Could not find argument #{} for function '{}'", LI, curfname));
        }
      }

      const auto& name = !func_args_names.empty() ? func_args_names[LI] : std::string();
      next_size = parse_arg<I, LI, F>(ctx, scr, arg_block, std::string_view(), override_block_behaviour, name, fn);
    }

    if (ctx->is_ignore()) {
      if (command_block cb(block, offset + next_size); cb.empty() && LI == 0) 
        raise_error(std::format("Seems like there is no data except block that returns 'ignore_value' - it is very dangerous in this language design, function: '{}'", block.name()));
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
    } else {
      if (block.empty()) raise_error(std::format("Could not find argument #{} for function '{}'", index, curfname));
    }
  }
  
  std::string_view override_lvalue;
  if (!arg_name.empty()) {
    if constexpr (is_optional) {
      using opt_t = std::remove_cvref_t<utils::optional_value_t<cur_arg_type>>;
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
      if constexpr (std::is_enum_v<cur_t>) {
        const auto enum_str = block.size() == 1 ? block.name() : command_block(block, 1).name();
        constexpr auto enum_name = utils::type_name<std::remove_cvref_t<cur_t>>();
        push_enum_literal(ctx, scr, enum_name, enum_str);
      } else dispatch_node(ctx, scr, block, override_lvalue);
    }
  } else {
    if constexpr (std::is_enum_v<cur_arg_type>) {
      const auto enum_str = block.size() == 1 ? block.name() : command_block(block, 1).name();
      constexpr auto enum_name = utils::type_name<std::remove_cvref_t<cur_arg_type>>();
      push_enum_literal(ctx, scr, enum_name, enum_str);
    } else dispatch_node(ctx, scr, block, override_lvalue);
  }

  if (!arg_name.empty()) { // special case - named argument description
    const auto cd = block.find(custom_description_constant);
    const auto cd_str = static_string_arg(cd, custom_description_constant);
    const auto tok = block.name() == "__empty_lvalue" ? override_lvalue : block.name();
    setup_block_description(ctx, scr, tok, cd_str, start, SIZE_MAX, container::description_node_kind::argument);
  }

  if constexpr (std::is_fundamental_v<cur_arg_type>) {
    if (!ctx->is_ignore()) {
      if (ctx->is_bool()) setup_type_conversion<bool, cur_arg_type>(ctx, scr);
      if (ctx->is_integral()) setup_type_conversion<int64_t, cur_arg_type>(ctx, scr);
      if (ctx->is_number()) setup_type_conversion<double, cur_arg_type>(ctx, scr);
    }
  }

  if constexpr (std::is_enum_v<cur_arg_type>) {
    if (!ctx->is_ignore() && !ctx->is<int64_t>())
      raise_error(std::format("Could not parse argument {} for function '{}': function expects enum '{}', but got '{}'", index, curfname, cur_arg_type_name, ctx->top()));
  } else if (!is_typeless_v<cur_arg_type> && !ctx->is_ignore() && !ctx->is<cur_arg_type>()) {
    raise_error(std::format("Could not parse argument {} for function '{}': function expects '{}', but got '{}'", index, curfname, cur_arg_type_name, ctx->top()));
  }

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
    }
  }

  basicf local_override_block_behaviour;
  if (!arg_name.empty()) {
    if constexpr (is_optional) {
      using opt_t = std::remove_cvref_t<utils::optional_value_t<cur_arg_type>>;
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
      if constexpr (std::is_enum_v<cur_t>) {
        const auto enum_str = block.size() == 1 ? block.name() : command_block(block, 1).name();
        constexpr auto enum_name = utils::type_name<std::remove_cvref_t<cur_t>>();
        push_enum_literal(ctx, scr, enum_name, enum_str);
      } else fold_block(ctx, scr, block, local_override_block_behaviour);
    }
  } else {
    if constexpr (std::is_enum_v<cur_arg_type>) {
      const auto enum_str = block.size() == 1 ? block.name() : command_block(block, 1).name();
      constexpr auto enum_name = utils::type_name<std::remove_cvref_t<cur_arg_type>>();
      push_enum_literal(ctx, scr, enum_name, enum_str);
    } else fold_block(ctx, scr, block, local_override_block_behaviour);
  }

  if (!arg_name.empty()) { // special case - named argument description
    const auto cd = block.find(custom_description_constant);
    const auto cd_str = static_string_arg(cd, custom_description_constant);
    const auto tok = block.name();
    setup_block_description(ctx, scr, tok, cd_str, start, SIZE_MAX, container::description_node_kind::argument);
  }

  if constexpr (std::is_fundamental_v<cur_arg_type>) {
    if (!ctx->is_ignore()) {
      if (ctx->is_bool()) setup_type_conversion<bool, cur_arg_type>(ctx, scr);
      if (ctx->is_integral()) setup_type_conversion<int64_t, cur_arg_type>(ctx, scr);
      if (ctx->is_number()) setup_type_conversion<double, cur_arg_type>(ctx, scr);
    }
  }

  if constexpr (std::is_enum_v<cur_arg_type>) {
    if (!ctx->is_ignore() && !ctx->is<int64_t>())
      raise_error(std::format("Could not parse argument {} for function '{}': function expects enum '{}', but got '{}'", index, curfname, cur_arg_type_name, ctx->top()));
  } else if (!is_typeless_v<cur_arg_type> && !ctx->is_ignore() && !ctx->is<cur_arg_type>()) {
    raise_error(std::format("Could not parse argument {} for function '{}': function expects '{}', but got '{}'", index, curfname, cur_arg_type_name, ctx->top()));
  }

  if (fn) std::invoke(fn, ctx, scr, index, block);

  return block.size();
}

template <typename FROM, typename TO>
void system::setup_type_conversion(parse_ctx* ctx, container* scr) const {
  if (std::is_same_v<FROM, TO>) return;
  if (!std::is_fundamental_v<FROM> || !std::is_fundamental_v<TO>) raise_error(std::format("Could not convert from '{}' to '{}'", utils::type_name<FROM>(), utils::type_name<TO>()));

  const function_t fs[] = { &convert_unsafe<FROM, TO>, &convert<FROM, TO> };
  scr->cmds.push_back(container::command(fs[size_t(safety())], INT64_C(0)));

  if (!ctx->is<FROM>()) raise_error(std::format("Wrong FROM type '{}' - stack last type is '{}'", ctx->stack_types.back(), utils::type_name<std::remove_cvref_t<FROM>>()));
  ctx->stack_types.back() = utils::type_name<final_stack_el_t<TO>>();
}

template <auto f, typename HT, is_valid_t<HT> vf>
  requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
void system::emit_call_instruction(parse_ctx*, container* scr, function_t safe, function_t unsafe, const int64_t scope_index) const {
  scr->cmds.push_back(container::command(safety() ? safe : unsafe, scope_index));
}

inline void system::emit_command_name(parse_ctx* ctx, container* scr, const std::string_view& name) const {
  const auto ref = store_string(scr, name);
  scr->cmds.push_back(container::command(&push_command_name, packstrid(uint32_t(ref.start), uint32_t(ref.count))));
  ctx->push<std::string_view>();
}

template <typename RetT>
void system::apply_call_stack_effect(parse_ctx* ctx, const size_t pops) const {
  for (size_t i = 0; i < pops; ++i) ctx->pop();
  if constexpr (!utils::is_void_v<RetT>) ctx->push<RetT>();
}

template <auto f, typename HT, is_valid_t<HT> vf>
  requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
void system::register_function(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using F = decltype(f);
  register_function<f, HT, on_effect_t<F, HT>(nullptr), vf>(std::move(name), std::move(func_args_names), std::move(init_f));
}

template <auto f>
  requires(valid_function_type<decltype(f)>)
void system::register_function(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using F = decltype(f);
  using scope_type = scope_t<F>;
  constexpr auto eff_ptr = on_effect_t<F, scope_type>(nullptr);
  constexpr auto val_ptr = &is_valid<scope_type>;
  register_function<f, scope_type, eff_ptr, val_ptr>(std::move(name), std::move(func_args_names), std::move(init_f));
}

template <auto f, typename HT, on_effect_t<decltype(f), HT> eff, is_valid_t<HT> vf>
  requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
void system::register_function(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using F = decltype(f);

  auto func = [func_args_names = std::move(func_args_names), init_f = std::move(init_f)]
    (emitter& e, const command_block& args) -> size_t
  {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    using memder_of = utils::function_member_of<F>;
    using scope_type = std::conditional_t<utils::is_void_v<HT>, memder_of, std::remove_cvref_t<HT>>;
    using ret_type = script_stack_el_t<utils::function_result_type<F>>;
    constexpr bool is_not_member_func = utils::is_void_v<memder_of>;
    constexpr bool requires_scope = !utils::is_void_v<scope_type>;
    constexpr size_t sig_args_count = utils::function_arguments_count<F>;
    constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
    constexpr auto uftype = get_user_function_type<F>();
    constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
    using first_argument = final_stack_el_t<utils::function_argument_type<F, first_argument_index>>;
    constexpr bool unlimited_args = args_count == 2 &&
      std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_argument_type<F, first_argument_index + 1>>>&&
      std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_result_type<F>>>;
    constexpr auto stn = scope_type_name<scope_type>();
    const auto curfname = ctx->function_names.back();

    const bool expected_void = ctx->expected_type == utils::type_name<void>();
    if (uftype == user_function_type::effect && !expected_void) sys->raise_error(std::format("Function effect '{}' called in not effect context", curfname));
    if (!utils::is_void_v<scope_type> && !ctx->is_scope<scope_type>())
      sys->raise_error(std::format("Trying to call function '{}' in wrong scope context: {} != {}", curfname, ctx->current_scope_type(), stn));

    if (!init_f) {
      if constexpr (!unlimited_args) {
        if (args_count != 0 && args.args_count() > args_count) sys->raise_error(std::format("Too many arguments for function '{}'", args.name()));
      }

      int64_t scope_index = INT64_MAX;
      if constexpr (requires_scope) {
        scope_index = ctx->scope_stack.back();
        if (ctx->current_scope_type() != utils::type_name<element_view>() && !ctx->is_scope<scope_type>())
          sys->raise_error(std::format("Stack index {} contains '{}' type, but function '{}' required '{}' type", scope_index, ctx->stack_types[scope_index], curfname, stn));
        if (ctx->current_scope_type() == utils::type_name<element_view>()) {
          if (!ctx->scope_type_upvalue.empty() && ctx->scope_type_upvalue != stn) sys->raise_error(std::format("Several functions with different scopes found? prev: '{}' cur: '{}'", ctx->scope_type_upvalue, stn));
          ctx->scope_type_upvalue = stn; // for context saved and context args
        }
      }

      std::vector<size_t> jumps;
      if constexpr (unlimited_args) {
        const size_t prev_index = ctx->unlimited_func_index;
        ctx->unlimited_func_index = ctx->function_names.size() - 1;

        // command_block b(args, 1) is first argument
        sys->parse_args<first_argument>(ctx, scr, args, 1, 0, func_args_names, [&](parse_ctx* ctx, container* scr, const size_t index, const command_block&) {
          if (index == 0) return;

          // For an effect, push its name on top of the two fold operands so on_effect can read it.
          if constexpr (eff != nullptr) sys->emit_command_name(ctx, scr, curfname);
          sys->emit_call_instruction<f, HT, vf>(ctx, scr, &userfunc<f, HT, vf, eff>, &userfunc_unsafe<f, HT, vf, eff>, scope_index);
          sys->apply_call_stack_effect<ret_type>(ctx, eff != nullptr ? 3 : 2);
        });

        ctx->unlimited_func_index = prev_index;
      } else {
        size_t offset = 1;
        offset = sys->parse_args<first_argument_index, 0, F>(ctx, scr, args, offset, func_args_names);
        const size_t stack_size_with_args = ctx->stack_types.size();
        // For an effect, push its name on top of the args so on_effect can read it as a stack value.
        if constexpr (eff != nullptr) sys->emit_command_name(ctx, scr, curfname);
        sys->emit_call_instruction<f, HT, vf>(ctx, scr, &userfunc<f, HT, vf, eff>, &userfunc_unsafe<f, HT, vf, eff>, scope_index);

        if constexpr (std::is_same_v<scope_type, internal::thisctxlist>) {
          if (ctx->list_index_upvalue == SIZE_MAX) sys->raise_error(std::format("Trying to use list function '{}' without list context on stack", curfname));
          const size_t id = ctx->list_index_upvalue;
          if (scr->lists[id].type.empty()) {
            scr->lists[id].type = ctx->top();
          } else {
            if (scr->lists[id].type != ctx->top()) sys->raise_error(std::format("Wrong type for list #{}^ expected '{}', got '{}'", id, scr->lists[id].type, ctx->top()));
          }
        }

        sys->apply_call_stack_effect<ret_type>(ctx, eff != nullptr ? args_count + 1 : args_count);
        const size_t stack_size = stack_size_with_args - args_count; // size after consuming args, before the result push
        if constexpr (uftype == user_function_type::object) {
          // Scope-returning functions may be followed by a nested block. If the function
          // consumed no explicit args, the whole block belongs to the new scope; otherwise
          // only the remaining child range is compiled in that scope.
          if (offset < args.size() && offset == 1) { // no args
            ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
            constexpr bool supports_nullable = !utils::is_void_v<ret_type> && !std::is_same_v<ret_type, ignore_value> && !is_typeless_v<ret_type>;
            const bool nullable = supports_nullable && args.nullable();
            const size_t guard_index = scr->cmds.size();
            if constexpr (supports_nullable) if (nullable) {
              constexpr function_t guard = &detail::nullable_scope_guard<ret_type, &is_valid<ret_type>>;
              const int32_t mode = type_is_bool(ctx->expected_type) ? 1 : (type_is_fundamental(ctx->expected_type) ? 2 : 0);
              scr->cmds.push_back(container::command(guard, pack2(0, mode)));
            }
            sys->fold_block(ctx, scr, args, basicf::invalid);
            sys->scope_exit(ctx, scr, 1);
            if (nullable) scr->cmds[guard_index].arg = pack2(static_cast<int32_t>(scr->cmds.size()), std::get<1>(unpack2(scr->cmds[guard_index].arg)));
          } else if (offset < args.size() && ctx->ftype == function_type::lvalue) { // was args
            const auto remaining = command_block(args, offset);
            ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
            constexpr bool supports_nullable = !utils::is_void_v<ret_type> && !std::is_same_v<ret_type, ignore_value> && !is_typeless_v<ret_type>;
            const bool nullable = supports_nullable && args.nullable();
            const size_t guard_index = scr->cmds.size();
            if constexpr (supports_nullable) if (nullable) {
              constexpr function_t guard = &detail::nullable_scope_guard<ret_type, &is_valid<ret_type>>;
              const int32_t mode = type_is_bool(ctx->expected_type) ? 1 : (type_is_fundamental(ctx->expected_type) ? 2 : 0);
              scr->cmds.push_back(container::command(guard, pack2(0, mode)));
            }
            sys->dispatch_node(ctx, scr, remaining);
            sys->scope_exit(ctx, scr, 1);
            if (nullable) scr->cmds[guard_index].arg = pack2(static_cast<int32_t>(scr->cmds.size()), std::get<1>(unpack2(scr->cmds[guard_index].arg)));

            offset += remaining.size();
            if (offset < args.size()) 
              sys->raise_warning(std::format("Found dead code in function '{}', this block '{}' would be ignored", curfname, command_block(args, offset).name()));
          }
        }

        if (stack_size == ctx->stack_types.size()) ctx->push<ignore_value>();
      }

      for (const auto i : jumps) { scr->cmds[i].arg = scr->cmds.size(); }
    } else std::invoke(init_f, e, args, func_args_names);

    return args.size();
  };

  using scope_type = std::remove_cvref_t<HT>;
  using ret_type = script_stack_el_t<utils::function_result_type<F>>;
  constexpr bool is_not_member_func = utils::is_void_v<utils::function_member_of<F>>;
  constexpr bool requires_scope = !utils::is_void_v<scope_type>;
  constexpr size_t sig_args_count = utils::function_arguments_count<F>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  constexpr auto parse_ftype = command_data::ftype::function_t;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  constexpr bool unlimited_args = args_count == 2 &&
    std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_argument_type<F, first_argument_index + 1>>>&&
    std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_result_type<F>>>;

  constexpr auto stn = scope_type_name<scope_type>();

  command_data mcd{
    name, stn, utils::type_name<ret_type>(), utils::make_function_sig_string<F>(),
    15, unlimited_args ? INT32_MAX : int32_t(args_count), command_data::associativity::right, parse_ftype,
    std::move(func),
    utils::is_void_v<ret_type> ? container::description_node_kind::effect : container::description_node_kind::function
  };

  register_function(std::move(mcd));
}

template <auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff>
  requires(valid_function_type<decltype(f)>)
void system::register_function(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using F = decltype(f);
  using scope_type = scope_t<F>;
  register_function<f, scope_type, eff, &is_valid<scope_type>>(std::move(name), std::move(func_args_names), std::move(init_f));
}

template <auto f, typename HT, is_valid_t<HT> vf>
  requires(valid_function_type<decltype(f)>&& valid_stack_type_v<HT>)
void system::register_operator(std::string name, const std::string_view& properties_as, custom_init_fn_t init_f) {
  const auto itr = mfuncs.find(std::string(properties_as));
  if (itr == mfuncs.end()) raise_error(std::format("Could not find function '{}'", properties_as));
  if (itr->second.empty()) raise_error(std::format("Could not find function '{}'", properties_as));

  operator_props ps;
  ps.priority = itr->second.begin()->second.priority;
  ps.assoc = itr->second.begin()->second.assoc;
  ps.mtype = static_cast<decltype(ps.mtype)>(itr->second.begin()->second.arg_count);

  register_operator<f, HT, vf>(std::move(name), ps, std::move(init_f));
}

template <auto f>
  requires(valid_function_type<decltype(f)>)
void system::register_operator(std::string name, const std::string_view& properties_as, custom_init_fn_t init_f) {
  using F = decltype(f);
  using scope_type = scope_t<F>;
  register_operator<f, scope_type, &is_valid<scope_type>>(std::move(name), properties_as, std::move(init_f));
}

template <auto f, typename HT, is_valid_t<HT> vf>
  requires(valid_function_type<decltype(f)> && valid_stack_type_v<HT>)
void system::register_operator(std::string name, const operator_props& ps, custom_init_fn_t init_f) {
  using F = decltype(f);
  using scope_type = std::remove_cvref_t<HT>;
  using ret_type = final_stack_el_t<utils::function_result_type<F>>;
  constexpr bool is_not_member_func = utils::is_void_v<utils::function_member_of<F>>;
  constexpr auto uftype = get_user_function_type<F>();
  constexpr bool requires_scope = !utils::is_void_v<scope_type>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  using first_argument = final_stack_el_t<utils::function_argument_type<F, first_argument_index>>;
  constexpr size_t sig_args_count = utils::function_arguments_count<F>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  constexpr auto parse_ftype = command_data::ftype::operator_t;
  constexpr bool unlimited_args = args_count == 2 &&
    std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_argument_type<F, first_argument_index + 1>>>&&
    std::is_same_v<std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>, std::remove_cvref_t<utils::function_result_type<F>>>;

  constexpr auto stn = scope_type_name<scope_type>();

  static_assert(args_count == 1 || args_count == 2, "Operators must have only 1 or 2 arguments");
  static_assert(!utils::is_void_v<ret_type> && !std::is_same_v<ret_type, ignore_value>, "Operators must have a proper return value");

  command_data mcd{
    name, stn, utils::type_name<ret_type>(), utils::make_function_sig_string<F>(), ps.priority, static_cast<int32_t>(ps.mtype), ps.assoc, parse_ftype,
    [init_f = std::move(init_f)](emitter& e, const command_block& args) -> size_t {
      [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
      constexpr auto stn = scope_type_name<scope_type>();
      const auto curfname = ctx->function_names.back();

      const bool expected_void = ctx->expected_type == utils::type_name<void>();
      if (expected_void) sys->raise_error(std::format("Operator '{}' called in effect context", curfname));

      if (!init_f) {
        int64_t scope_index = INT64_MAX;
        if constexpr (requires_scope) {
          scope_index = ctx->scope_stack.back();
          if (!ctx->is_scope<scope_type>()) sys->raise_error(std::format("Stack index {} contains '{}' type, but function '{}' required '{}' type", scope_index, ctx->current_scope_type(), curfname, stn));
        }

        if constexpr (!unlimited_args) {
          if (args.args_count() > args_count) sys->raise_error(std::format("Too many arguments for function '{}'", args.name()));
        }

        std::vector<size_t> jumps;
        if constexpr (unlimited_args) {
          sys->parse_args<first_argument>(ctx, scr, args, 1, 0, {}, [&](parse_ctx* ctx, container* scr, const size_t index, const command_block&) {
            if (index == 0) return;

            sys->emit_call_instruction<f, HT, vf>(ctx, scr, &mathfunc<f, HT, vf>, &mathfunc_unsafe<f, HT, vf>, scope_index);
            sys->apply_call_stack_effect<ret_type>(ctx, 2);
          });
        } else {
          size_t offset = 1;
          offset = sys->parse_args<first_argument_index, 0, F>(ctx, scr, args, offset, {});
          sys->emit_call_instruction<f, HT, vf>(ctx, scr, &mathfunc<f, HT, vf>, &mathfunc_unsafe<f, HT, vf>, scope_index);
          sys->apply_call_stack_effect<ret_type>(ctx, args_count);
          if constexpr (uftype == user_function_type::object) {
            // Scope-returning operators follow the same nested-block rules as functions.
            if (offset < args.size() && offset == 1) { // no args
              ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
              sys->fold_block(ctx, scr, args, basicf::invalid);
              sys->scope_exit(ctx, scr, 1);
            } else if (offset < args.size() && ctx->ftype == function_type::lvalue) { // was args
              const auto remaining = command_block(args, offset);
              ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
              sys->dispatch_node(ctx, scr, remaining);
              sys->scope_exit(ctx, scr, 1);

              offset += remaining.size();
              if (offset < args.size()) 
                sys->raise_warning(std::format("Found dead code in function '{}', this block '{}' would be ignored", curfname, command_block(args, offset).name()));
            }
          }
        }

        for (const auto i : jumps) { scr->cmds[i].arg = scr->cmds.size(); }
      } else std::invoke(init_f, e, args, std::vector<std::string>{});

      return args.size();
    },
    container::description_node_kind::operator_t
  };

  register_function(std::move(mcd));
}

template <auto f>
  requires(valid_function_type<decltype(f)>)
void system::register_operator(std::string name, const operator_props& properties, custom_init_fn_t init_f) {
  using F = decltype(f);
  using scope_type = scope_t<F>;
  register_operator<f, scope_type, &is_valid<scope_type>>(std::move(name), properties, std::move(init_f));
}


template<auto f, typename HT, is_valid_t<HT> vf>
  requires(utils::is_function_v<decltype(f)> && valid_stack_type_v<HT>)
void system::register_function_iter(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using F = decltype(f);
  using scope_type = std::remove_cvref_t<HT>;
  constexpr auto parse_ftype = command_data::ftype::function_t;
  using ret_type = final_stack_el_t<utils::function_result_type<F>>;

  constexpr auto stn = scope_type_name<scope_type>();

  command_data cd{
    name, stn, utils::type_name<ret_type>(), utils::make_function_sig_string<F>(), 15, 50, command_data::associativity::right, parse_ftype,
    [func_args_names = std::move(func_args_names), init_f = std::move(init_f)]
      (emitter& e, const command_block& args)
    {
      [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
      constexpr bool is_not_member_func = is_not_member_function<F>;
      using scope_type = HT;
      constexpr bool requires_scope = !utils::is_void_v<scope_type>;
      constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
      using first_argument = std::remove_cvref_t<utils::function_argument_type<F, first_argument_index>>;
      constexpr auto uftype = get_user_function_type<F, utils::function_result_type<F>, true>();
      constexpr size_t sig_args_count = utils::function_arguments_count<F>;
      constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
      const auto curfname = ctx->function_names.back();
      using ret_type = final_stack_el_t<utils::function_result_type<F>>;
      constexpr auto stn = scope_type_name<scope_type>();

      if constexpr (!is_valid_argument_function_v<first_argument>) sys->raise_error(std::format("At least one argument must be a function in iterator function '{}'", curfname));

      int64_t scope_index = INT64_MAX;
      if (!init_f) {
        if constexpr (requires_scope) {
          scope_index = ctx->scope_stack.back();
          if (!ctx->is_scope<scope_type>()) sys->raise_error(std::format("Stack index {} contains '{}' type, but function '{}' required '{}' type", scope_index, ctx->current_scope_type(), curfname, stn));
        }
      }

      const bool expected_void = type_is_void(ctx->expected_type);
      if (expected_void && uftype != user_function_type::iterator_effect) sys->raise_error(std::format("Effect iterator is outside of effect scriptblock?"));

      if (init_f) {
        std::invoke(init_f, e, args, func_args_names);
      } else {
        constexpr function_t fs[] = { &useriter_unsafe<f, HT, vf>, &useriter<f, HT, vf> };
        scr->cmds.emplace_back(container::command(fs[size_t(sys->safety())], scope_index));

        std::vector<size_t> jumps;
        utils::static_for<args_count>([&](auto) {
          const size_t jump_index = sys->push_basic_function(ctx, scr, basicf::jump, 0);
          jumps.push_back(jump_index);
        });

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
          {
            description_placeholder placeholder(ctx);
            sys->parse_arg<value_type>(ctx, scr, child, index, std::string_view(), std::string_view(), name);
          }
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
    },
    container::description_node_kind::iterator
  };

  register_function(std::move(cd));
}

template <auto f>
  requires(utils::is_function_v<decltype(f)>)
void system::register_function_iter(std::string name, std::vector<std::string> func_args_names, custom_init_fn_t init_f) {
  using F = decltype(f);
  using scope_type = scope_t<F>;
  register_function_iter<f, scope_type, &is_valid<scope_type>>(std::move(name), std::move(func_args_names), std::move(init_f));
}

template <typename T>
  requires (std::is_enum_v<T>)
void system::register_enum(const std::span<std::tuple<std::string, T>>& values) {
  using enum_t = std::remove_cvref_t<T>;
  std::unordered_map<std::string, enum_t> table;
  for (const auto& [name, val] : values) {
    table.emplace(name, val);
  }
  register_enum<enum_t>([table = std::move(table)](const std::string_view name) -> std::optional<enum_t> {
    const auto itr = table.find(std::string(name));
    if (itr == table.end()) return std::nullopt;
    return itr->second;
  });
}

template <typename T>
  requires (std::is_enum_v<T>)
void system::register_enum(const std::span<std::tuple<std::string_view, T>>& values) {
  using enum_t = std::remove_cvref_t<T>;
  std::unordered_map<std::string, enum_t> table;
  for (const auto& [name, val] : values) {
    table.emplace(std::string(name), val);
  }
  register_enum<enum_t>([table = std::move(table)](const std::string_view name) -> std::optional<enum_t> {
    const auto itr = table.find(std::string(name));
    if (itr == table.end()) return std::nullopt;
    return itr->second;
  });
}

template <typename T, typename F>
  requires (std::is_enum_v<T> && std::is_invocable_r_v<std::optional<T>, F, std::string_view>)
void system::register_enum(F fn) {
  using enum_t = std::remove_cvref_t<T>;
  const auto enum_name = std::string(utils::type_name<enum_t>());
  if (enums.contains(enum_name)) raise_error(std::format("Enum '{}' is already registered", enum_name));

  enums.emplace(enum_name, [fn = std::move(fn)](const std::string_view name) -> std::optional<int64_t> {
    const auto val = std::invoke(fn, name);
    if (!val.has_value()) return std::nullopt;
    using underlying_t = std::underlying_type_t<enum_t>;
    return static_cast<int64_t>(static_cast<underlying_t>(*val));
  });
}

template <typename RETURN_T, typename ROOT_T>
void system::parse_context::init(const system& sys, container& c) {
  using ret_type = script_stack_el_t<RETURN_T>;
  using root_type_t = final_stack_el_t<ROOT_T>;

       if constexpr (utils::is_void_v<ret_type>)                 root_block_name = "__effect_block";
  else if constexpr (std::is_same_v<bool, ret_type>)             root_block_name = "AND";
  else if constexpr (std::is_fundamental_v<ret_type>)            root_block_name = "ADD";
  else if constexpr (std::is_same_v<std::string_view, ret_type>) root_block_name = "__string_block";
  else                                                           root_block_name = "__object_block";

  return_type = scope_type_name<ret_type>();
  expected_type = return_type;
  c.return_type = return_type;
  root_type = utils::is_void_v<root_type_t> ? std::string_view() : scope_type_name<root_type_t>();
  unlimited_func_index = 0;
  nest_level = 0;
  // Per-parse peak trackers (the limits in max_stack_limit / max_saved_limit are NOT reset here so a
  // caller can tighten them on a freshly-constructed parse_context before parsing).
  max_stack_depth = 0;
  max_child_saved = 0;
  max_child_lists = 0;
  ftype = function_type::lvalue;
  prng_s = prng::xoshiro256starstar::init(sys.get_seed());
  c.prng_state = gen_value();
  initialized = true;

  if constexpr (!utils::is_void_v<root_type_t>) {
    if (c.args.empty()) {
      c.args.push_back({ { static_cast<size_t>(basicf::root), SIZE_MAX }, root_type });
      sys.push_basic_function(this, &c, basicf::pushroot, 0);
      scope_stack.push_back(stack_types.size()-1);
    }
  }
}

template <typename RETURN_T, typename ROOT_T>
std::tuple<tavl::event, tavl::error> system::parse(std::string_view name, tavl::parser& p, parse_context& ctx, container& c) const {
  if (!ctx.initialized) ctx.init<RETURN_T, ROOT_T>(*this, c);
  return parse(name, p, ctx, c);
}

template <typename RETURN_T, typename ROOT_T>
container system::parse(std::string_view name, std::string_view text) const {
  using ret_type = script_stack_el_t<RETURN_T>;
  using root_type = final_stack_el_t<ROOT_T>;

  container scr;
  parse_context ctx;
  ctx.init<RETURN_T, ROOT_T>(*this, scr);
  scr.name = store_string(&scr, name);
  // Raw source stays local: it only feeds normalize/make_script_ast. The container never retains
  // it — `store_string` builds the compact `scr.source` token pool during the semantic pass.
  const auto script_block = std::string_view(text);

  // Path N: tavl lexes/structures/precedences the script (make_script_ast); normalize() turns its
  // AST into the same rpn block stream the semantic pass consumes. Replaces the old text parser +
  // shunting-yard path; convert_scope (scope-path splitting) is still used downstream.
  tavl::parser tp;
  configure_parser(tp);
  const auto tree = make_script_ast(tp, script_block);
  // Reserve before the walk so token text accumulates without reallocating mid-traversal.
  ctx.rpn_ctx.token_storage.reserve(script_block.size() + 4096);
  ctx.rpn_ctx.normalize(tree, script_block);
  auto output = ctx.rpn_ctx.output;
  output.emplace(output.begin(), rpn_conversion_ctx::block{ ctx.rpn_ctx.store_token(ctx.root_block_name), output.size()+1 });

  reserve_from_hint(&scr, output.size(), ctx.rpn_ctx.token_storage.size());

  set_function_type sft(&ctx, function_type::lvalue);

  auto script_cmds = command_block(std::span<rpn_conversion_ctx::block>(output), &ctx.rpn_ctx.token_storage);

  {
    set_expected_type set(&ctx, scope_type_name<ret_type>());
    dispatch_node(&ctx, &scr, script_cmds);
  }
  if (const auto cd = static_string_arg(script_cmds.find(custom_description_constant), custom_description_constant); !cd.empty() && !scr.block_descs.empty()) {
    scr.block_descs.back().custom_description = store_string(&scr, cd);
  }
  ctx.rpn_ctx.clear();

  while (ctx.pop_while_ignore()) {}

  if constexpr (!utils::is_void_v<root_type>) {
    if (ctx.scope_stack.size() != 1) raise_error(std::format("There is not closed scope of type '{}' on stack", ctx.stack_types[ctx.scope_stack.back()]));
    scope_exit(&ctx, &scr, 1);
  }

  if constexpr (!utils::is_void_v<ret_type>) {
    if (!ctx.is<ret_type>()) raise_error(std::format("Invalid return type '{}' expected '{}', stack size {}", ctx.stack_types.back(), scope_type_name<ret_type>(), ctx.stack_types.size()));
    push_basic_function(&ctx, &scr, basicf::pushreturn, 0);
  }

  if (ctx.stack_types.size() != 0) raise_error(std::format("Script is not properly ended, {} values on stack", ctx.stack_types.size()));

  finalize_resource_usage(ctx, scr);

  scr.build_description_index();
  compact_source_storage(&scr);

  return scr;
}

template <typename T>
bool system::parse_context::is_scope() const {
  using basic_T = final_stack_el_t<T>;
  return current_scope_type() == scope_type_name<basic_T>();
}

template <typename T>
bool system::parse_context::is() const {
  using basic_T = final_stack_el_t<T>;
  return top() == scope_type_name<basic_T>();
}

template <typename T>
void system::parse_context::push() {
  using basic_T = final_stack_el_t<T>;
  constexpr auto stn = scope_type_name<basic_T>();
  push(stn);
}

template <typename F, typename RT, bool is_iterator_func>
constexpr system::user_function_type system::get_user_function_type() {
  using ret_type = script_stack_el_t<std::conditional_t<is_iterator_func, RT, utils::function_result_type<F>>>;
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
