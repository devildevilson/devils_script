#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "devils_script/common.h"
#include "devils_script/context.h"
#include "devils_script/basic_functions.h"

// Template runtime thunks for registered C++ callbacks.
//
// Registration stores erased command function pointers in the compiled container. The
// templates in this file recover the original C++ function signature, pull arguments and
// scope values from the VM stack, validate scopes, invoke the user callback, and push the
// normalized script result back to the stack.
//
// Iterator callbacks are implemented by wrapping compiled sub-command ranges in lambdas or
// `script_function` views. Those wrappers share the caller's `context`, so iterator bodies
// must restore the stack to the expected shape before returning.

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

namespace detail {
template <typename T>
struct is_script_function : std::false_type {};

template <typename R, typename Arg>
struct is_script_function<script_function<R(Arg)>> : std::true_type {};

template <typename T>
constexpr bool is_script_function_v = is_script_function<std::remove_cvref_t<T>>::value;

template <size_t OFF, size_t COUNT, auto f, typename HT, is_valid_t<HT> vt, size_t... I>
int64_t invoke_mathfunc(int64_t val, context* ctx, const container* scr, std::index_sequence<I...>);

template <size_t OFF, size_t COUNT, auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff, size_t... I>
int64_t invoke_userfunc(context* ctx, const container* scr, std::index_sequence<I...>);

template <size_t OFF, size_t COUNT, auto f, typename HT, on_effect_t<decltype(f), HT> eff, size_t... I>
int64_t invoke_userfunc_scope(context* ctx, const container* scr, HT& scope, std::index_sequence<I...>);

template <size_t OFF, size_t COUNT, auto f, typename HT, is_valid_t<HT> vt, size_t... I>
int64_t invoke_mathfunc_unsafe(int64_t val, context* ctx, const container* scr, std::index_sequence<I...>);

template <size_t OFF, size_t COUNT, auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff, size_t... I>
int64_t invoke_userfunc_unsafe(context* ctx, const container* scr, std::index_sequence<I...>);

template <size_t OFF, size_t COUNT, auto f, typename HT, on_effect_t<decltype(f), HT> eff, size_t... I>
int64_t invoke_userfunc_scope_unsafe(context* ctx, const container* scr, HT& scope, std::index_sequence<I...>);

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t invoke_mathfunc_noargs(int64_t val, context* ctx, const container* scr);

template <auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff>
int64_t invoke_userfunc_noargs(context* ctx, const container* scr);

template <auto f, typename HT, on_effect_t<decltype(f), HT> eff>
int64_t invoke_userfunc_scope_noargs(context* ctx, const container* scr, HT& scope);

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t invoke_mathfunc_unsafe_noargs(int64_t val, context* ctx, const container* scr);

template <auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff>
int64_t invoke_userfunc_unsafe_noargs(context* ctx, const container* scr);

template <auto f, typename HT, on_effect_t<decltype(f), HT> eff>
int64_t invoke_userfunc_scope_unsafe_noargs(context* ctx, const container* scr, HT& scope);
}

template <auto f, typename HT = void, is_valid_t<HT> vt = nullptr>
int64_t mathfunc(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT = void, is_valid_t<HT> vt = nullptr, on_effect_t<decltype(f), HT> eff = nullptr>
int64_t userfunc(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT, is_valid_t<HT> vt = nullptr>
int64_t useriter(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_effect(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT, is_valid_t<HT> vt, bool has_count>
int64_t useriter_condition(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_numeric(int64_t val, context* ctx, const container* scr);

template <typename FROM, typename TO>
int64_t convert(int64_t, context*, const container*);

template <auto f, typename HT = void, is_valid_t<HT> vt = nullptr>
int64_t mathfunc_unsafe(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT = void, is_valid_t<HT> vt = nullptr, on_effect_t<decltype(f), HT> eff = nullptr>
int64_t userfunc_unsafe(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT, is_valid_t<HT> vt = nullptr>
int64_t useriter_unsafe(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_effect_unsafe(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT, is_valid_t<HT> vt, bool has_count>
int64_t useriter_condition_unsafe(int64_t val, context* ctx, const container* scr);

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_numeric_unsafe(int64_t val, context* ctx, const container* scr);

template <typename FROM, typename TO>
int64_t convert_unsafe(int64_t, context*, const container*);

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t compute_count_and_mul(int64_t, context*, const container*);








namespace detail {
template <typename F, size_t ID>
using el_t = final_stack_el_t<utils::function_argument_type<F, ID>>;

template <typename T> requires (std::is_enum_v<std::remove_cvref_t<T>>)
auto stack_get(context* ctx, const int64_t index) -> std::remove_cvref_t<T> {
  using cur_t = std::remove_cvref_t<T>;
  return static_cast<cur_t>(ctx->stack.safe_get<int64_t>(index));
}

template <typename T> requires (utils::is_optional_v<T>)
auto stack_get(context* ctx, const int64_t index) -> std::remove_cvref_t<T> {
  using cur_t = final_stack_el_t<utils::optional_value_t<T>>;
  if (ctx->stack.invalid(index)) return std::nullopt;
  if constexpr (std::is_enum_v<cur_t>) {
    return std::make_optional(stack_get<cur_t>(ctx, index));
  } else return T(ctx->stack.safe_get<cur_t>(index));
}

template <typename T>
auto stack_get(context* ctx, const int64_t index) -> final_stack_el_t<T> {
  using cur_t = final_stack_el_t<T>;
  return T(ctx->stack.safe_get<cur_t>(index));
}

template <typename T> requires (std::is_enum_v<std::remove_cvref_t<T>>)
auto stack_get_unsafe(context* ctx, const int64_t index) -> std::remove_cvref_t<T> {
  using cur_t = std::remove_cvref_t<T>;
  return static_cast<cur_t>(ctx->stack.get<int64_t>(index));
}

template <typename T> requires (utils::is_optional_v<T>)
auto stack_get_unsafe(context* ctx, const int64_t index) -> std::remove_cvref_t<T> {
  using cur_t = final_stack_el_t<utils::optional_value_t<T>>;
  if (ctx->stack.invalid(index)) return std::nullopt;
  if constexpr (std::is_enum_v<cur_t>) {
    return std::make_optional(stack_get_unsafe<cur_t>(ctx, index));
  }
  else return T(ctx->stack.get<cur_t>(index));
}

template <typename T>
auto stack_get_unsafe(context* ctx, const int64_t index) -> final_stack_el_t<T> {
  using cur_t = final_stack_el_t<T>;
  return T(ctx->stack.get<cur_t>(index));
}

template <typename T>
void stack_push_result(context* ctx, const T& value) {
  using value_t = std::remove_cvref_t<T>;
  if constexpr (std::is_enum_v<value_t>) {
    using underlying_t = std::underlying_type_t<value_t>;
    ctx->stack.push(static_cast<int64_t>(static_cast<underlying_t>(value)));
  } else {
    ctx->stack.push(value);
  }
}

template <typename T, is_valid_t<T> vt>
int64_t nullable_scope_guard(int64_t arg, context* ctx, const container*) {
  const auto [target, mode] = unpack2(arg);
  const auto value = ctx->stack.safe_get<T>();
  if (!std::invoke(vt, value)) {
    ctx->stack.erase();
    if (mode == 1) ctx->stack.push(false);
    else if (mode == 2) ctx->stack.push(0.0);
    else {
      stack_element el;
      el.invalidate();
      ctx->stack.push(std::string_view(), el);
    }
    ctx->current_index = target - 1;
  }
  return 0;
}

template <size_t OFF, size_t COUNT, auto f, typename HT, is_valid_t<HT> vt, size_t... I>
int64_t invoke_mathfunc(int64_t val, context* ctx, const container*, std::index_sequence<I...>) {
  if constexpr (utils::is_void_v<HT>) {
    const auto ret = std::invoke(f, stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
    ctx->stack.resize(ctx->stack.size() - COUNT);
    stack_push_result(ctx, ret);
  } else {
    auto c = ctx->stack.safe_get<HT>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
    const auto ret = std::invoke(f, c, stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
    ctx->stack.resize(ctx->stack.size() - COUNT);
    stack_push_result(ctx, ret);
  }

  return -int64_t(COUNT) + 1;
}

template <size_t OFF, size_t COUNT, auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff, size_t... I>
int64_t invoke_userfunc(context* ctx, const container* scr, std::index_sequence<I...>) {
  using ret_val_t = std::remove_cvref_t<utils::function_result_type<decltype(f)>>;
  if constexpr (utils::is_void_v<ret_val_t> || std::is_same_v<ret_val_t, ignore_value>) {
    if constexpr (eff == nullptr) {
      std::invoke(f, stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
    } else {
      const auto& t = std::make_tuple(stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      std::apply(f, t);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, t);
    }

    ctx->stack.resize(ctx->stack.size() - COUNT);
    return -int64_t(COUNT);
  } else {
    if constexpr (eff == nullptr) {
      const auto ret = std::invoke(f, stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      ctx->stack.resize(ctx->stack.size() - COUNT);
      detail::stack_push_result(ctx, ret);
    } else {
      auto t = std::make_tuple(stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      const auto ret = std::apply(f, t);
      ctx->stack.resize(ctx->stack.size() - COUNT);
      detail::stack_push_result(ctx, ret);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, ret, t);
    }

    return -int64_t(COUNT) + 1;
  }
}

template <size_t OFF, size_t COUNT, auto f, typename HT, on_effect_t<decltype(f), HT> eff, size_t... I>
int64_t invoke_userfunc_scope(context* ctx, const container* scr, HT &scope, std::index_sequence<I...>) {
  using ret_val_t = std::remove_cvref_t<utils::function_result_type<decltype(f)>>;
  if constexpr (utils::is_void_v<ret_val_t> || std::is_same_v<ret_val_t, ignore_value>) {
    if constexpr (eff == nullptr) {
      std::invoke(f, scope, stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
    } else {
      const auto& t = std::make_tuple(scope, stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      std::apply(f, t);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, t);
    }

    ctx->stack.resize(ctx->stack.size() - COUNT);
    return -int64_t(COUNT);
  } else {
    if constexpr (eff == nullptr) {
      const auto ret = std::invoke(f, scope, stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      ctx->stack.resize(ctx->stack.size() - COUNT);
      detail::stack_push_result(ctx, ret);
    } else {
      const auto& t = std::make_tuple(scope, stack_get<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      const auto ret = std::apply(f, t);
      ctx->stack.resize(ctx->stack.size() - COUNT);
      detail::stack_push_result(ctx, ret);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, ret, t);
    }

    return -int64_t(COUNT) + 1;
  }
}

template <size_t OFF, size_t COUNT, auto f, typename HT, is_valid_t<HT> vt, size_t... I>
int64_t invoke_mathfunc_unsafe(int64_t val, context* ctx, const container*, std::index_sequence<I...>) {
  if constexpr (utils::is_void_v<HT>) {
    const auto ret = std::invoke(f, stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
    ctx->stack.resize(ctx->stack.size() - COUNT);
    stack_push_result(ctx, ret);
  } else {
    auto c = ctx->stack.get<HT>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
    const auto ret = std::invoke(f, c, stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
    ctx->stack.resize(ctx->stack.size() - COUNT);
    stack_push_result(ctx, ret);
  }

  return -int64_t(COUNT) + 1;
}

template <size_t OFF, size_t COUNT, auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff, size_t... I>
int64_t invoke_userfunc_unsafe(context* ctx, const container* scr, std::index_sequence<I...>) {
  using ret_val_t = std::remove_cvref_t<utils::function_result_type<decltype(f)>>;
  if constexpr (utils::is_void_v<ret_val_t> || std::is_same_v<ret_val_t, ignore_value>) {
    if constexpr (eff == nullptr) {
      std::invoke(f, stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
    } else {
      auto t = std::make_tuple(stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      std::apply(f, t);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, t);
    }

    return -int64_t(COUNT);
  } else {
    if constexpr (eff == nullptr) {
      const auto ret = std::invoke(f, stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      ctx->stack.resize(ctx->stack.size() - COUNT);
      detail::stack_push_result(ctx, ret);
    } else {
      auto t = std::make_tuple(stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      const auto ret = std::apply(f, t);
      ctx->stack.resize(ctx->stack.size() - COUNT);
      detail::stack_push_result(ctx, ret);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, ret, t);
    }

    return -int64_t(COUNT) + 1;
  }
}

template <size_t OFF, size_t COUNT, auto f, typename HT, on_effect_t<decltype(f), HT> eff, size_t... I>
int64_t invoke_userfunc_scope_unsafe(context* ctx, const container* scr, HT &scope, std::index_sequence<I...>) {
  using ret_val_t = std::remove_cvref_t<utils::function_result_type<decltype(f)>>;
  if constexpr (utils::is_void_v<ret_val_t> || std::is_same_v<ret_val_t, ignore_value>) {
    if constexpr (eff == nullptr) {
      std::invoke(f, scope, stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
    } else {
      const auto& t = std::make_tuple(scope, stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      std::apply(f, t);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, t);
    }

    return -int64_t(COUNT);
  } else {
    if constexpr (eff == nullptr) {
      const auto ret = std::invoke(f, scope, stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      ctx->stack.resize(ctx->stack.size() - COUNT);
      detail::stack_push_result(ctx, ret);
    } else {
      auto t = std::make_tuple(scope, stack_get_unsafe<el_t<decltype(f), I+OFF>>(ctx, -int64_t(COUNT - I))...);
      const auto ret = std::apply(f, t);
      ctx->stack.resize(ctx->stack.size() - COUNT);
      detail::stack_push_result(ctx, ret);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, ret, t);
    }

    return -int64_t(COUNT) + 1;
  }
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t invoke_mathfunc_noargs(int64_t val, context* ctx, const container*) {
  if constexpr (utils::is_void_v<HT>) {
    const auto ret = std::invoke(f);
    stack_push_result(ctx, ret);
  } else {
    auto c = ctx->stack.safe_get<HT>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
    const auto ret = std::invoke(f, c);
    stack_push_result(ctx, ret);
  }
  return 1;
}

template <auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff>
int64_t invoke_userfunc_noargs(context* ctx, const container* scr) {
  using ret_val_t = std::remove_cvref_t<utils::function_result_type<decltype(f)>>;
  if constexpr (utils::is_void_v<ret_val_t> || std::is_same_v<ret_val_t, ignore_value>) {
    if constexpr (eff == nullptr) {
      std::invoke(f);
    } else {
      std::invoke(f);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, std::tuple<>{});
    }

    return 0;
  } else {
    if constexpr (eff == nullptr) {
      const auto ret = std::invoke(f);
      detail::stack_push_result(ctx, ret);
    } else {
      const auto ret = std::invoke(f);
      detail::stack_push_result(ctx, ret);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, ret, std::tuple<>{});
    }

    return 1;
  }
}

template <auto f, typename HT, on_effect_t<decltype(f), HT> eff>
int64_t invoke_userfunc_scope_noargs(context* ctx, const container* scr, HT& scope) {
  using ret_val_t = std::remove_cvref_t<utils::function_result_type<decltype(f)>>;
  if constexpr (utils::is_void_v<ret_val_t> || std::is_same_v<ret_val_t, ignore_value>) {
    if constexpr (eff == nullptr) {
      std::invoke(f, scope);
    } else {
      std::invoke(f, scope);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, std::make_tuple(scope));
    }

    return 0;
  } else {
    if constexpr (eff == nullptr) {
      const auto ret = std::invoke(f, scope);
      detail::stack_push_result(ctx, ret);
    } else {
      const auto ret = std::invoke(f, scope);
      detail::stack_push_result(ctx, ret);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, ret, std::make_tuple(scope));
    }

    return 1;
  }
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t invoke_mathfunc_unsafe_noargs(int64_t val, context* ctx, const container*) {
  if constexpr (utils::is_void_v<HT>) {
    const auto ret = std::invoke(f);
    stack_push_result(ctx, ret);
  } else {
    auto c = ctx->stack.get<HT>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
    const auto ret = std::invoke(f, c);
    stack_push_result(ctx, ret);
  }

  return 1;
}

template <auto f, on_effect_t<decltype(f), scope_t<decltype(f)>> eff>
int64_t invoke_userfunc_unsafe_noargs(context* ctx, const container* scr) {
  using ret_val_t = std::remove_cvref_t<utils::function_result_type<decltype(f)>>;
  if constexpr (utils::is_void_v<ret_val_t> || std::is_same_v<ret_val_t, ignore_value>) {
    if constexpr (eff == nullptr) {
      std::invoke(f);
    } else {
      std::invoke(f);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, std::tuple<>{});
    }

    return 0;
  } else {
    if constexpr (eff == nullptr) {
      const auto ret = std::invoke(f);
      detail::stack_push_result(ctx, ret);
    } else {
      const auto ret = std::invoke(f);
      detail::stack_push_result(ctx, ret);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, ret, std::tuple<>{});
    }

    return 1;
  }
}

template <auto f, typename HT, on_effect_t<decltype(f), HT> eff>
int64_t invoke_userfunc_scope_unsafe_noargs(context* ctx, const container* scr, HT& scope) {
  using ret_val_t = std::remove_cvref_t<utils::function_result_type<decltype(f)>>;
  if constexpr (utils::is_void_v<ret_val_t> || std::is_same_v<ret_val_t, ignore_value>) {
    if constexpr (eff == nullptr) {
      std::invoke(f, scope);
    } else {
      std::invoke(f, scope);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, std::make_tuple(scope));
    }

    return 0;
  } else {
    if constexpr (eff == nullptr) {
      const auto ret = std::invoke(f, scope);
      detail::stack_push_result(ctx, ret);
    } else {
      const auto ret = std::invoke(f, scope);
      detail::stack_push_result(ctx, ret);
      const auto& name = scr->get_string(scr->descs[ctx->current_index].name);
      std::invoke(eff, ctx->userptr, name, ret, std::make_tuple(scope));
    }

    return 1;
  }
}
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t mathfunc(int64_t val, context* ctx, const container* scr) {
  constexpr bool is_not_member_func = is_not_member_function<decltype(f)>;
  constexpr bool requires_scope = !utils::is_void_v<HT>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  constexpr size_t sig_args_count = utils::function_arguments_count<decltype(f)>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  if constexpr (args_count == 0) {
    return detail::invoke_mathfunc_noargs<f, HT, vt>(val, ctx, scr);
  } else {
    return detail::invoke_mathfunc<first_argument_index, args_count, f, HT, vt>(val, ctx, scr, std::make_index_sequence<args_count>{});
  }
}

template <auto f, typename HT, is_valid_t<HT> vt, on_effect_t<decltype(f), HT> eff>
int64_t userfunc(int64_t val, context* ctx, const container* scr) {
  constexpr bool is_not_member_func = is_not_member_function<decltype(f)>;
  constexpr bool requires_scope = !utils::is_void_v<HT>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  constexpr size_t sig_args_count = utils::function_arguments_count<decltype(f)>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);

  if constexpr (args_count == 0) {
    if constexpr (utils::is_void_v<HT>) {
      return detail::invoke_userfunc_noargs<f, eff>(ctx, scr);
    } else {
      auto c = ctx->stack.safe_get<HT>(val);
      if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
      return detail::invoke_userfunc_scope_noargs<f, HT, eff>(ctx, scr, c);
    }
  } else {
    if constexpr (utils::is_void_v<HT>) {
      return detail::invoke_userfunc<0, sig_args_count, f, eff>(ctx, scr, std::make_index_sequence<sig_args_count>{});
    } else {
      auto c = ctx->stack.safe_get<HT>(val);
      if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
      return detail::invoke_userfunc_scope<first_argument_index, args_count, f, HT, eff>(ctx, scr, c, std::make_index_sequence<args_count>{});
    }
  }
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter(int64_t val, context* ctx, const container* scr) {
  constexpr bool is_not_member_func = is_not_member_function<decltype(f)>;
  using scope_type = HT;
  constexpr bool requires_scope = !utils::is_void_v<scope_type>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  constexpr size_t sig_args_count = utils::function_arguments_count<decltype(f)>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  using ret_type = final_stack_el_t<utils::function_result_type<decltype(f)>>;

  std::array<size_t, args_count> cmd_starts;
  std::array<size_t, args_count> cmd_ends;
  
  const size_t curins = ctx->current_index;
  const size_t start_jumpins = curins + 1;
  size_t curstart = start_jumpins + args_count;
  for (size_t i = 0; i < args_count; ++i) {
    const size_t index = start_jumpins + i;
    cmd_starts[i] = curstart;
    cmd_ends[i] = scr->cmds[index].arg;
    curstart = cmd_ends[i] == 0 ? curstart : cmd_ends[i];
  }

  auto args_tuple = utils::function_arguments_tuple_t<decltype(f)>{};
  utils::static_for<args_count>([&](auto index) {
    constexpr size_t cur_index = first_argument_index + index;
    using fn_t = std::remove_cvref_t<utils::function_argument_type<decltype(f), cur_index>>;
    using value_type = std::remove_cvref_t<utils::function_result_type<fn_t>>;
    using input_type = utils::function_argument_type<fn_t, 0>;

    static_assert(utils::is_function_v<fn_t>);

    if (cmd_ends[index] == 0) std::get<cur_index>(args_tuple) = nullptr;
    else if constexpr (detail::is_script_function_v<fn_t>) {
      std::get<cur_index>(args_tuple) = fn_t(ctx, scr, cmd_starts[index], cmd_ends[index]);
    } else {
      container_view v(scr, cmd_starts[index], cmd_ends[index]);
      std::get<cur_index>(args_tuple) = [v, ctx](input_type in) -> value_type {
        ctx->stack.push(in);
        v.process(ctx);
        if constexpr (!utils::is_void_v<value_type>) {
          return ctx->stack.safe_pop<value_type>();
        }
      };
    }
  });

  if constexpr (utils::is_void_v<scope_type>) {
    if constexpr (utils::is_void_v<ret_type> || std::is_same_v<ret_type, ignore_value>) {
      std::apply(f, args_tuple);
    } else {
      const auto ret = std::apply(f, args_tuple);
      detail::stack_push_result(ctx, ret);
    }
  } else if constexpr (is_not_member_func) {
    auto c = ctx->stack.safe_get<scope_type>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<scope_type>(), ctx->current_index));
    std::get<0>(args_tuple) = c;

    if constexpr (utils::is_void_v<ret_type> || std::is_same_v<ret_type, ignore_value>) {
      std::apply(f, args_tuple);
    } else {
      const auto ret = std::apply(f, args_tuple);
      detail::stack_push_result(ctx, ret);
    }
  } else if constexpr (is_member_function<decltype(f)>) {
    auto c = ctx->stack.safe_get<scope_type>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<scope_type>(), ctx->current_index));

    if constexpr (utils::is_void_v<ret_type> || std::is_same_v<ret_type, ignore_value>) {
      std::apply(f, std::tuple_cat(c, args_tuple));
    } else {
      const auto ret = std::apply(f, std::tuple_cat(c, args_tuple));
      detail::stack_push_result(ctx, ret);
    }
  } else throw std::runtime_error("Bad scope deduction");

  // Skip the iterator body command ranges after the callback consumes them.
  ctx->current_index = curstart - 1;
  
  return 1;
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_effect(int64_t val, context* ctx, const container* scr) {
  using first_el_t = std::remove_cvref_t<utils::function_argument_type<decltype(f), 0>>;
  using function_t = std::conditional_t<utils::is_function_v<first_el_t>, first_el_t, std::remove_cvref_t<utils::function_argument_type<decltype(f), 1>>>;
  using input_scope = utils::function_argument_type<function_t, 0>;

  const size_t curins = ctx->current_index;
  const size_t jumpins = curins + 1;
  const size_t start = curins + 2;
  const size_t end = scr->cmds[jumpins].arg;
  auto view = container_view(scr, start, end);

  const auto in_f = [&](input_scope s) -> bool {
    ctx->stack.push(s);
    view.process(ctx);
    ctx->stack.erase();

    return true;
  };

  if constexpr (!std::is_fundamental_v<first_el_t> && !utils::is_function_v<first_el_t>) {
    auto c = ctx->stack.safe_get<first_el_t>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<first_el_t>(), ctx->current_index));
    std::invoke(f, c, in_f);
  } else {
    std::invoke(f, in_f);
  }

  ctx->current_index = curins;

  return 1; // jump next
}

template <auto f, typename HT, is_valid_t<HT> vt, bool has_count>
int64_t useriter_condition(int64_t val, context* ctx, const container* scr) {
  using first_el_t = std::remove_cvref_t<utils::function_argument_type<decltype(f), 0>>;
  using function_t = std::conditional_t<utils::is_function_v<first_el_t>, first_el_t, std::remove_cvref_t<utils::function_argument_type<decltype(f), 1>>>;
  using input_scope = utils::function_argument_type<function_t, 0>;

  size_t size = 1;
  if constexpr (has_count) {
    size = size_t(ctx->stack.safe_pop<double>());
  }

  const size_t curins = ctx->current_index;
  const size_t jumpins = curins + 1;
  const size_t start = curins + 2;
  const size_t end = scr->cmds[jumpins].arg;
  auto view = container_view(scr, start, end);

  double sum = 0;
  size_t counter = 0;
  const auto in_f = [&] (input_scope s) -> bool {
    ctx->stack.push(s);
    const size_t cursize = ctx->stack.size();

    view.process(ctx);

    if (cursize+1 == ctx->stack.size()) throw std::runtime_error("Error?");

    const bool val = ctx->stack.safe_pop<bool>();
    ctx->stack.erase();
    counter += size_t(val);

    // Count-limited condition iterators stop once enough matching items were found.
    if (counter >= size) return false;
    return true;
  };

  if constexpr (!std::is_fundamental_v<first_el_t> && !utils::is_function_v<first_el_t>) {
    auto c = ctx->stack.safe_get<first_el_t>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<first_el_t>(), ctx->current_index));
    std::invoke(f, c, in_f);
  } else {
    std::invoke(f, in_f);
  }

  ctx->current_index = curins;
  ctx->stack.push(sum);

  return 1; // jump next
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_numeric(int64_t val, context* ctx, const container* scr) {
  using first_el_t = std::remove_cvref_t<utils::function_argument_type<decltype(f), 0>>;
  using function_t = std::conditional_t<utils::is_function_v<first_el_t>, first_el_t, std::remove_cvref_t<utils::function_argument_type<decltype(f), 1>>>;
  using input_scope = utils::function_argument_type<function_t, 0>;

  const size_t curins = ctx->current_index;
  const size_t jumpins = curins + 1;
  const size_t start = curins + 2;
  const size_t end = scr->cmds[jumpins].arg;
  auto view = container_view(scr, start, end);

  double sum = 0;
  const auto in_f = [&] (input_scope s) -> bool {
    ctx->stack.push(s);
    const size_t cursize = ctx->stack.size();

    view.process(ctx);

    if (cursize+1 == ctx->stack.size()) throw std::runtime_error("Error?");

    const double val = ctx->stack.safe_pop<double>();
    sum += val;
    ctx->stack.erase();
    return true;
  };

  if constexpr (!std::is_fundamental_v<first_el_t> && !utils::is_function_v<first_el_t>) {
    auto c = ctx->stack.safe_get<first_el_t>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<first_el_t>(), ctx->current_index));
    std::invoke(f, c, in_f);
  } else {
    std::invoke(f, in_f);
  }

  ctx->current_index = curins;
  ctx->stack.push(sum);

  return 1; // jump next
}

template <typename FROM, typename TO>
int64_t convert(int64_t, context* ctx, const container*) {
  auto v = ctx->stack.safe_pop<FROM>();
  ctx->stack.push(TO(v));
  return 0;
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t mathfunc_unsafe(int64_t val, context* ctx, const container* scr) {
  constexpr bool is_not_member_func = is_not_member_function<decltype(f)>;
  constexpr bool requires_scope = !utils::is_void_v<HT>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  constexpr size_t sig_args_count = utils::function_arguments_count<decltype(f)>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);

  if constexpr (args_count == 0) {
    return detail::invoke_mathfunc_unsafe_noargs<f, HT, vt>(val, ctx, scr);
  } else {
    return detail::invoke_mathfunc_unsafe<first_argument_index, args_count, f, HT, vt>(val, ctx, scr, std::make_index_sequence<args_count>{});
  }
}

template <auto f, typename HT, is_valid_t<HT> vt, on_effect_t<decltype(f), HT> eff>
int64_t userfunc_unsafe(int64_t val, context* ctx, const container* scr) {
  constexpr bool is_not_member_func = is_not_member_function<decltype(f)>;
  constexpr bool requires_scope = !utils::is_void_v<HT>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  constexpr size_t sig_args_count = utils::function_arguments_count<decltype(f)>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);

  if constexpr (args_count == 0) {
    if constexpr (utils::is_void_v<HT>) {
      return detail::invoke_userfunc_unsafe_noargs<f, eff>(ctx, scr);
    } else {
      auto c = ctx->stack.get<HT>(val);
      if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
      return detail::invoke_userfunc_scope_unsafe_noargs<f, HT, eff>(ctx, scr, c);
    }
  } else {
    if constexpr (utils::is_void_v<HT>) {
      return detail::invoke_userfunc_unsafe<0, sig_args_count, f, eff>(ctx, scr, std::make_index_sequence<sig_args_count>{});
    } else {
      auto c = ctx->stack.get<HT>(val);
      if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
      return detail::invoke_userfunc_scope_unsafe<first_argument_index, args_count, f, HT, eff>(ctx, scr, c, std::make_index_sequence<args_count>{});
    }
  }
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_unsafe(int64_t val, context* ctx, const container* scr) {
  constexpr bool is_not_member_func = is_not_member_function<decltype(f)>;
  using scope_type = HT;
  constexpr bool requires_scope = !utils::is_void_v<scope_type>;
  constexpr size_t first_argument_index = size_t(requires_scope && is_not_member_func);
  constexpr size_t sig_args_count = utils::function_arguments_count<decltype(f)>;
  constexpr size_t args_count = sig_args_count - size_t(requires_scope && is_not_member_func);
  using ret_type = final_stack_el_t<utils::function_result_type<decltype(f)>>;

  std::array<size_t, args_count> cmd_starts;
  std::array<size_t, args_count> cmd_ends;

  const size_t curins = ctx->current_index;
  const size_t start_jumpins = curins + 1;
  size_t curstart = start_jumpins + args_count;
  for (size_t i = 0; i < args_count; ++i) {
    const size_t index = start_jumpins + i;
    cmd_starts[i] = curstart;
    cmd_ends[i] = scr->cmds[index].arg;
    curstart = cmd_ends[i] == 0 ? curstart : cmd_ends[i];
  }

  auto args_tuple = utils::function_arguments_tuple_t<decltype(f)>{};
  utils::static_for<args_count>([&](auto index) {
    constexpr size_t cur_index = first_argument_index + index;
    using fn_t = std::remove_cvref_t<utils::function_argument_type<decltype(f), cur_index>>;
    using value_type = std::remove_cvref_t<utils::function_result_type<fn_t>>;
    using input_type = utils::function_argument_type<fn_t, 0>;

    if (cmd_ends[index] == 0) std::get<cur_index>(args_tuple) = nullptr;
    else if constexpr (detail::is_script_function_v<fn_t>) {
      std::get<cur_index>(args_tuple) = fn_t(ctx, scr, cmd_starts[index], cmd_ends[index]);
    } else {
      container_view v(scr, cmd_starts[index], cmd_ends[index]);
      std::get<cur_index>(args_tuple) = [v, ctx](input_type in) -> value_type {
        ctx->stack.push(in);
        v.process(ctx);
        if constexpr (!utils::is_void_v<value_type>) {
          return ctx->stack.pop<value_type>();
        }
      };
    }
  });

  if constexpr (utils::is_void_v<scope_type>) {
    if constexpr (utils::is_void_v<ret_type> || std::is_same_v<ret_type, ignore_value>) {
      std::apply(f, args_tuple);
    } else {
      const auto ret = std::apply(f, args_tuple);
      detail::stack_push_result(ctx, ret);
    }
  } else if constexpr (is_not_member_func) {
    auto c = ctx->stack.get<scope_type>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<scope_type>(), ctx->current_index));
    std::get<0>(args_tuple) = c;

    if constexpr (utils::is_void_v<ret_type> || std::is_same_v<ret_type, ignore_value>) {
      std::apply(f, args_tuple);
    } else {
      const auto ret = std::apply(f, args_tuple);
      detail::stack_push_result(ctx, ret);
    }
  } else if constexpr (is_member_function<decltype(f)>) {
    auto c = ctx->stack.get<scope_type>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<scope_type>(), ctx->current_index));

    if constexpr (utils::is_void_v<ret_type> || std::is_same_v<ret_type, ignore_value>) {
      std::apply(f, std::tuple_cat(c, args_tuple));
    } else {
      const auto ret = std::apply(f, std::tuple_cat(c, args_tuple));
      detail::stack_push_result(ctx, ret);
    }
  } else throw std::runtime_error("Bad scope deduction");

  // Skip the iterator body command ranges after the callback consumes them.
  ctx->current_index = curstart - 1;

  return 1;
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_effect_unsafe(int64_t val, context* ctx, const container* scr) {
  using first_el_t = std::remove_cvref_t<utils::function_argument_type<decltype(f), 0>>;
  using function_t = std::conditional_t<utils::is_function_v<first_el_t>, first_el_t, std::remove_cvref_t<utils::function_argument_type<decltype(f), 1>>>;
  using input_scope = utils::function_argument_type<function_t, 0>;

  const size_t curins = ctx->current_index;
  const size_t jumpins = curins + 1;
  const size_t start = curins + 2;
  const size_t end = scr->cmds[jumpins].arg;
  auto view = container_view(scr, start, end);

  const auto in_f = [&](input_scope s) -> bool {
    ctx->stack.push(s);
    view.process(ctx);
    ctx->stack.erase();

    return true;
  };

  if constexpr (!std::is_fundamental_v<first_el_t> && !utils::is_function_v<first_el_t>) {
    auto c = ctx->stack.get<first_el_t>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<first_el_t>(), ctx->current_index));
    std::invoke(f, c, in_f);
  }
  else {
    std::invoke(f, in_f);
  }

  ctx->current_index = curins;

  return 1; // jump next
}

template <auto f, typename HT, is_valid_t<HT> vt, bool has_count>
int64_t useriter_condition_unsafe(int64_t val, context* ctx, const container* scr) {
  using first_el_t = std::remove_cvref_t<utils::function_argument_type<decltype(f), 0>>;
  using function_t = std::conditional_t<utils::is_function_v<first_el_t>, first_el_t, std::remove_cvref_t<utils::function_argument_type<decltype(f), 1>>>;
  using input_scope = utils::function_argument_type<function_t, 0>;

  size_t size = 1;
  if constexpr (has_count) {
    size = size_t(ctx->stack.pop<double>());
  }

  const size_t curins = ctx->current_index;
  const size_t jumpins = curins + 1;
  const size_t start = curins + 2;
  const size_t end = scr->cmds[jumpins].arg;
  auto view = container_view(scr, start, end);

  double sum = 0;
  size_t counter = 0;
  const auto in_f = [&](input_scope s) -> bool {
    ctx->stack.push(s);
    const size_t cursize = ctx->stack.size();

    view.process(ctx);

    if (cursize + 1 == ctx->stack.size()) throw std::runtime_error("Error?");

    const bool val = ctx->stack.pop<bool>();
    ctx->stack.erase();
    counter += size_t(val);

    // Count-limited condition iterators stop once enough matching items were found.
    if (counter >= size) return false;
    return true;
  };

  if constexpr (!std::is_fundamental_v<first_el_t> && !utils::is_function_v<first_el_t>) {
    auto c = ctx->stack.get<first_el_t>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<first_el_t>(), ctx->current_index));
    std::invoke(f, c, in_f);
  } else {
    std::invoke(f, in_f);
  }

  ctx->current_index = curins;
  ctx->stack.push(sum);

  return 1; // jump next
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t useriter_numeric_unsafe(int64_t val, context* ctx, const container* scr) {
  using first_el_t = std::remove_cvref_t<utils::function_argument_type<decltype(f), 0>>;
  using function_t = std::conditional_t<utils::is_function_v<first_el_t>, first_el_t, std::remove_cvref_t<utils::function_argument_type<decltype(f), 1>>>;
  using input_scope = utils::function_argument_type<function_t, 0>;

  const size_t curins = ctx->current_index;
  const size_t jumpins = curins + 1;
  const size_t start = curins + 2;
  const size_t end = scr->cmds[jumpins].arg;
  auto view = container_view(scr, start, end);

  double sum = 0;
  const auto in_f = [&](input_scope s) -> bool {
    ctx->stack.push(s);
    const size_t cursize = ctx->stack.size();

    view.process(ctx);

    if (cursize + 1 == ctx->stack.size()) throw std::runtime_error("Error?");

    const double val = ctx->stack.pop<double>();
    sum += val;
    ctx->stack.erase();
    return true;
  };

  if constexpr (!std::is_fundamental_v<first_el_t> && !utils::is_function_v<first_el_t>) {
    auto c = ctx->stack.get<first_el_t>(val);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<first_el_t>(), ctx->current_index));
    std::invoke(f, c, in_f);
  } else {
    std::invoke(f, in_f);
  }

  ctx->current_index = curins;
  ctx->stack.push(sum);

  return 1; // jump next
}

template <typename FROM, typename TO>
int64_t convert_unsafe(int64_t, context* ctx, const container*) {
  auto v = ctx->stack.pop<FROM>();
  ctx->stack.push(TO(v));
  return 0;
}

template <auto f, typename HT, is_valid_t<HT> vt>
int64_t compute_count_and_mul(int64_t arg, context* ctx, const container* scr) {
  size_t count = 0;
  if constexpr (!utils::is_void_v<HT>) {
    auto c = ctx->stack.safe_get<HT>(arg);
    if (!std::invoke(vt, c)) throw std::runtime_error(std::format("Scope handle '{}' is invalid, instruction {}", utils::type_name<HT>(), ctx->current_index));
    count = std::invoke(f, c);
  } else {
    count = std::invoke(f);
  }

  const double v = ctx->stack.safe_pop<double>();
  ctx->stack.push(v * double(count));
  return 0;
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
