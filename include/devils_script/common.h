#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <string>
#include <format>
#include "devils_script/type_traits.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

#define DEVILS_SCRIPT_BASIC_FUNCTIONS_LIST \
  X(jump) \
  X(root) \
  X(conversion) \
  X(compute_count) \
  X(effect_block) \
  X(string_block) \
  X(object_block) \
  X(string_subblock) \
  X(object_subblock) \
  X(AND) \
  X(OR) \
  X(NAND) \
  X(NOR) \
  X(ADD) \
  X(MUL) \
  X(andbin) \
  X(sum) \
  X(sumsetstack) \
  X(mulsetstack) \
  X(cmpeq2) \
  X(cmplesseqd2) \
  X(notfn) \
  X(unary_plus) \
  X(unary_minus) \
  X(andjump) \
  X(orjump) \
  X(condjump) \
  X(condjumpt) \
  X(condjump_get) \
  X(condjumpt_get) \
  X(pushbool) \
  X(pushvalue) \
  X(pushint) \
  X(pushstring) \
  X(pushroot) \
  X(pushthis) \
  X(pushprev) \
  X(pushreturn) \
  X(pusharg) \
  X(pushinvalid) \
  X(erase) \
  X(current) \
  X(chance) \
  X(argcontext) \
  X(context) \
  X(pushargvalue) \
  X(setargrvalue) \
  X(setarglvalue) \
  X(pushctxvalue) \
  X(savectxrvalue) \
  X(savectxlvalue) \
  X(pushlist) \




template <typename T>
using final_stack_el_t = std::conditional_t<
  utils::is_void_v<T>, utils::void_t,
  std::conditional_t<
    std::is_same_v<bool, T>, bool,
    std::conditional_t<
      std::is_integral_v<T>, int64_t,
      std::conditional_t<
        std::is_floating_point_v<T>, double,
  std::remove_cvref_t<T>
>>>>;

inline size_t compute_count1() { return 0; }
template <typename T>
inline size_t compute_count2(T) { return 0; }
template <typename F>
using compute_count_t = std::conditional_t<utils::is_function_v<utils::function_argument_type<F, 0>>, decltype(&compute_count1), decltype(&compute_count2<utils::function_argument_type<utils::function_argument_type<F, 1>, 0>>)>;

template<typename T>
concept has_valid_member_func = requires {
  { std::declval<T>().valid() } -> std::same_as<bool>;
};

template<typename T>
concept has_is_valid_member_func = requires {
  { std::declval<T>().is_valid() } -> std::same_as<bool>;
};

template<typename T>
concept has_not_valid_member_func = requires {
  { std::declval<T>().not_valid() } -> std::same_as<bool>;
};

template<typename T>
concept has_invalid_member_func = requires {
  { std::declval<T>().invalid() } -> std::same_as<bool>;
};

template<typename T>
concept has_operator_bool_func = requires {
  { std::declval<T>().operator bool() } -> std::same_as<bool>;
};

template <typename T> requires(has_valid_member_func<T>)
inline bool is_valid_detail(const T& arg) { return arg.valid(); }
template <typename T> requires(has_is_valid_member_func<T>)
inline bool is_valid_detail(const T& arg) { return arg.is_valid(); }
template <typename T> requires(has_not_valid_member_func<T>)
inline bool is_valid_detail(const T& arg) { return !arg.not_valid(); }
template <typename T> requires(has_invalid_member_func<T>)
inline bool is_valid_detail(const T& arg) { return !arg.invalid(); }
template <typename T> requires(has_operator_bool_func<T>)
inline bool is_valid_detail(const T& arg) { return bool(arg); }

template <typename T> requires (std::same_as<std::remove_cvref_t<T>, void> || std::same_as<std::remove_cvref_t<T>, utils::void_t>)
inline bool is_valid() { return true; }

template <typename T> requires (!std::same_as<std::remove_cvref_t<T>, void> && !std::same_as<std::remove_cvref_t<T>, utils::void_t>)
bool is_valid(const T& arg);

template <typename T>
using is_valid_t = decltype(&is_valid<T>);

struct context;
struct container;
inline int64_t default_command_f(int64_t, context*, const container*) { return 0; }
using function_t = decltype(&default_command_f);

inline void assert_msg_fn(context*, const container*, const std::string_view&, const size_t) {}
using assert_fn_t = decltype(&assert_msg_fn);

constexpr std::string_view custom_description_constant = "custom_description";

#define MAXIMUM_STACK_VAL_SIZE 16

template <typename T>
constexpr bool valid_stack_el_type_v = (sizeof(utils::void_or_t<std::remove_cvref_t<T>>) <= MAXIMUM_STACK_VAL_SIZE && std::is_trivially_destructible_v<utils::void_or_t<std::remove_cvref_t<T>>>);

bool type_is_ignore(const std::string_view& type) noexcept;
bool type_is_void(const std::string_view& type) noexcept;
bool type_is_bool(const std::string_view& type) noexcept;
bool type_is_integral(const std::string_view& type) noexcept;
bool type_is_floating_point(const std::string_view& type) noexcept;
bool type_is_fundamental(const std::string_view& type) noexcept;
bool type_is_string(const std::string_view& type) noexcept;
bool type_is_object(const std::string_view& type) noexcept;
bool type_is_element_view(const std::string_view& type) noexcept;
bool type_is_object_view(const std::string_view& type) noexcept;
bool type_is_any_stack(const std::string_view& type) noexcept;
bool type_is_any_object(const std::string_view& type) noexcept;
bool type_is_any_type_object(const std::string_view& type) noexcept;
bool type_is_any_type(const std::string_view& type) noexcept;

template <typename T>
constexpr std::string_view scope_type_name() {
  using T2 = std::remove_cvref_t<T>;
  if constexpr (utils::is_void_v<T2>) {
    return utils::type_name<void>();
  } else if constexpr (std::is_pointer_v<T2>) {
    using T3 = std::remove_cvref_t<std::remove_pointer_t<T2>>;
    return utils::type_name<T3*>();
  } else return utils::type_name<T2>();
}

// тут бы тип добавить рядом с памятью
struct alignas(MAXIMUM_STACK_VAL_SIZE) stack_element {
  struct view {
    const char* _mem;
    std::string_view _type;

    view() noexcept;
    view(const char* _mem, const std::string_view& _type) noexcept;
    bool valid() const;
    std::string_view type() const;

    template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, view>)
    bool is() const;
    template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, view>)
    auto rawget() const -> const final_stack_el_t<T>;
    template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, view>)
    auto get() const -> final_stack_el_t<T>;

    friend bool operator==(const view& v1, const view& v2);
    friend bool operator!=(const view& v1, const view& v2);
  };

  // соединить тип и данные в одной структуре?
  char mem[MAXIMUM_STACK_VAL_SIZE];

  template <typename T> requires(valid_stack_el_type_v<T>)
  auto rawget() const -> const final_stack_el_t<T>*;
  template <typename T> requires(valid_stack_el_type_v<T>)
  auto get() const -> final_stack_el_t<T>;
  template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, view>)
  void set(const T& val);

  void invalidate();
  bool invalid() const;
};

struct alignas(MAXIMUM_STACK_VAL_SIZE) any_stack {
  char _mem[MAXIMUM_STACK_VAL_SIZE];
  std::string_view _type;

  any_stack() noexcept;
  template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, stack_element::view> || std::is_same_v<std::remove_cvref_t<T>, any_stack>)
  any_stack(const T& val) noexcept;
  any_stack(const char* _mem, const std::string_view& _type) noexcept;

  void invalidate();
  bool invalid() const;
  std::string_view type() const;
  stack_element::view view() const;

  template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, stack_element::view> || std::is_same_v<std::remove_cvref_t<T>, any_stack>)
  bool is() const;
  template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, stack_element::view> || std::is_same_v<std::remove_cvref_t<T>, any_stack>)
  auto get() const -> final_stack_el_t<T>;
};

using element_view = stack_element::view;
struct object_view : public element_view {};
struct any_object : public any_stack {};

struct ignore_value {};
struct invalid_value {};

template <typename T>
constexpr bool is_el_view_v = std::is_same_v<element_view, std::remove_cvref_t<T>>;

template <typename T>
constexpr bool is_object_view_v = std::is_same_v<object_view, std::remove_cvref_t<T>>;

template <typename T>
constexpr bool is_any_stack_v = std::is_same_v<any_stack, std::remove_cvref_t<T>>;

template <typename T>
constexpr bool is_any_object_v = std::is_same_v<any_object, std::remove_cvref_t<T>>;

template <typename T>
constexpr bool is_typeless_v = is_el_view_v<T> || is_object_view_v<T> || is_any_stack_v<T> || is_any_object_v<T>;

template <typename T>
constexpr bool valid_stack_type_v = valid_stack_el_type_v<T> || is_typeless_v<std::remove_cvref_t<T>>;

template<typename T>
concept valid_stack_type = requires { valid_stack_type_v<T>; };

template <typename T>
constexpr bool is_valid_argument_type_v = valid_stack_type_v<T> || (utils::is_optional_v<T> && !utils::is_void_v<utils::optional_value_t<T>>) || std::is_same_v<element_view, std::remove_cvref_t<T>> || std::is_same_v<object_view, std::remove_cvref_t<T>>;

template <typename T>
constexpr bool is_valid_return_type_v = valid_stack_type_v<T> || std::is_same_v<any_stack, std::remove_cvref_t<T>> || std::is_same_v<any_object, std::remove_cvref_t<T>> || utils::is_void_v<T>;

template <typename T>
//constexpr bool is_not_fundamental = ((!std::is_pointer_v<T> && !std::is_fundamental_v<T>) || (std::is_pointer_v<T> && !std::is_fundamental_v<std::remove_pointer_t<T>>)) && !utils::is_void_v<T>;
constexpr bool is_not_fundamental = !std::is_fundamental_v<T> && !std::is_same_v<std::string_view, T>;

template <typename F>
constexpr bool is_not_member_function = utils::is_void_v<utils::function_member_of<F>>;

template <typename F>
constexpr bool is_member_function = !utils::is_void_v<utils::function_member_of<F>>;

template <typename F>
constexpr bool scope_is_required = !is_not_member_function<F> || (is_not_member_function<F> && is_not_fundamental<utils::function_argument_type<F, 0>>);

template <typename F>
constexpr bool valid_function_return_type = utils::is_void_v<utils::function_result_type<F>> || valid_stack_type_v<utils::function_result_type<F>> || std::is_same_v<any_stack, std::remove_cvref_t<utils::function_result_type<F>>>;

template<typename F>
concept valid_function_type = requires { utils::is_function_v<F> && (utils::is_void_v<utils::function_result_type<F>> || valid_function_return_type<F>); };

template <typename F>
constexpr bool is_predicate_function_v = utils::is_function_v<F> && std::is_same_v<std::remove_cvref_t<utils::function_result_type<F>>, bool> && utils::function_arguments_count<F> == 1;

template <typename F>
constexpr bool is_numeric_function_v = utils::is_function_v<F> && std::is_fundamental_v<std::remove_cvref_t<utils::function_result_type<F>>> && !std::is_same_v<std::remove_cvref_t<utils::function_result_type<F>>, bool> && utils::function_arguments_count<F> == 1;

template <typename F>
constexpr bool is_effect_function_v = utils::is_function_v<F> && utils::is_void_v<std::remove_cvref_t<utils::function_result_type<F>>> && utils::function_arguments_count<F> == 1;

// std::optional
template <typename F>
constexpr bool is_valid_argument_function_v = is_predicate_function_v<F> || is_numeric_function_v<F> || is_effect_function_v<F>;

template <typename F>
using member_of_or_first = std::remove_cvref_t<std::conditional_t<is_member_function<F>, utils::function_member_of<F>, utils::function_argument_type<F, 0>>>;

template <typename T>
using valid_scope_t_or_void = std::conditional_t<
  utils::is_void_v<std::remove_cvref_t<T>>, utils::void_t,
    std::conditional_t<
    is_typeless_v<std::remove_cvref_t<T>>, utils::void_t,
      std::conditional_t<
      std::is_fundamental_v<std::remove_cvref_t<T>>, utils::void_t,
        std::conditional_t<
        std::is_enum_v<std::remove_cvref_t<T>>, utils::void_t,
          std::conditional_t<
          std::is_same_v<std::string_view, std::remove_cvref_t<T>>, utils::void_t,
            std::conditional_t<
            std::is_same_v<ignore_value, std::remove_cvref_t<T>>, utils::void_t,
              std::conditional_t<
              valid_stack_el_type_v<std::remove_cvref_t<T>>, std::remove_cvref_t<T>, std::remove_cvref_t<T>*
  >>>>>>>;

template <typename F>
using scope_t = valid_scope_t_or_void<member_of_or_first<F>>;

template <typename F, typename HT>
using args_t = std::conditional_t<
  !utils::is_void_v<utils::function_member_of<F>>, utils::function_arguments_tuple_t<F>,
  std::conditional_t<
    utils::is_void_v<HT>, utils::function_arguments_tuple_t<F>,
    utils::remove_ith_tuple_t<0, utils::function_arguments_tuple_t<F>>
  >>;

template <typename T, typename Tuple>
inline void on_effect1(void*, const std::string_view&, const T&, const Tuple&) {}
template <typename Tuple>
inline void on_effect2(void*, const std::string_view&, const Tuple&) {}

template <typename T, typename Tuple>
using on_effect1_t = void(*)(void*, const std::string_view&, const T&, const Tuple&);

template <typename Tuple>
using on_effect2_t = void(*)(void*, const std::string_view&, const Tuple&);

template <typename F, typename HT>
using on_effect_t = std::conditional_t<
  utils::is_void_v<HT>, 
    std::conditional_t<
    utils::is_void_v<utils::function_result_type<F>>, 
      on_effect2_t<utils::function_arguments_tuple_t<F>>,
      on_effect1_t<std::remove_cvref_t<utils::void_or_t<utils::function_result_type<F>>>, utils::function_arguments_tuple_t<F>>
    >,
    std::conditional_t<
    !utils::is_void_v<utils::function_member_of<F>>,
      std::conditional_t<
      utils::is_void_v<utils::function_result_type<F>>,
        on_effect2_t<utils::function_arguments_tuple_t<F>>,
        on_effect1_t<std::remove_cvref_t<utils::void_or_t<utils::function_result_type<F>>>, utils::function_arguments_tuple_t<F>>
      >,
      std::conditional_t<
      utils::is_void_v<utils::function_result_type<F>>,
        on_effect2_t<utils::tuple_cat_t<std::tuple<HT>, utils::function_arguments_tuple_t<F>>>,
        on_effect1_t<std::remove_cvref_t<utils::void_or_t<utils::function_result_type<F>>>, utils::tuple_cat_t<std::tuple<HT>, utils::function_arguments_tuple_t<F>>>
      >
    >
  >;

bool is_valid_function_name(const std::string_view &name) noexcept;

enum class basicf {
  none,
#define X(name) name,
  DEVILS_SCRIPT_BASIC_FUNCTIONS_LIST
#undef X

  invalid
};

std::string_view to_string(const basicf val) noexcept;
basicf find_basicf(const std::string_view &str) noexcept;

template <typename T1, typename T2> requires(is_typeless_v<T1> && is_typeless_v<T2>)
bool operator==(const T1& s1, const T2& s2) noexcept;
template <typename T1, typename T2> requires(is_typeless_v<T1> && is_typeless_v<T2>)
bool operator!=(const T1& s1, const T2& s2) noexcept;

// instead of std function
template <typename RT, typename IN>
class subblock {
public:
  subblock(context* ctx, const container* scr, const size_t start, const size_t end) noexcept;
  RT operator() (const IN& in) const;
  bool valid() const noexcept;
private:
  context* ctx;
  const container* scr;
  size_t start;
  size_t end;
};

namespace detail {
  template <typename>
  struct subblock_traits {
    static constexpr bool is_subblock = false;
    using return_type = utils::void_t;
    using input_type = utils::void_t;
  };

  template <typename RT, typename IN>
  struct subblock_traits<subblock<RT, IN>> {
    static constexpr bool is_subblock = true;
    using return_type = RT;
    using input_type = IN;
  };
}

template <typename T>
constexpr bool is_subblock_v = detail::subblock_traits<T>::is_subblock;

template <typename T>
using subblock_return_type = typename detail::subblock_traits<T>::return_type;

template <typename T>
using subblock_input_type = typename detail::subblock_traits<T>::input_type;








template <typename T> requires (!std::same_as<std::remove_cvref_t<T>, void> && !std::same_as<std::remove_cvref_t<T>, utils::void_t>)
bool is_valid(const T& arg) {
  if constexpr (utils::is_void_v<T>) return true;
  else if constexpr (std::is_fundamental_v<T>) return true;
  else if constexpr (is_typeless_v<T>) return true;
  else if constexpr (std::is_same_v<std::string_view, T>) return true;
  else if constexpr (std::is_pointer_v<T>) return arg != nullptr;
  else return is_valid_detail(arg);
}

template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, element_view>)
bool stack_element::view::is() const {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_el_view_v<basic_T>) {
    return true;
  } else if constexpr (std::is_pointer_v<basic_T>) {
    using no_ptr_t = std::remove_cvref_t<std::remove_pointer_t<basic_T>>;
    return _type == utils::type_name<no_ptr_t*>() || _type == utils::type_name<const no_ptr_t*>();
  } else return _type == utils::type_name<basic_T>();
}

template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, element_view>)
auto stack_element::view::rawget() const -> const final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_el_view_v<basic_T>) {
    return this;
  } else return *reinterpret_cast<const basic_T*>(&_mem[0]);
}

template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, element_view>)
auto stack_element::view::get() const -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if (!is<basic_T>()) throw std::runtime_error(std::format("Stack value view contains '{}' type, but '{}' is requested", _type, utils::type_id<basic_T>()));
  if constexpr (is_el_view_v<basic_T>) {
    return *this;
  } else return rawget<basic_T>();
}

template <typename T> requires(valid_stack_el_type_v<T>)
auto stack_element::rawget() const -> const final_stack_el_t<T>* {
  using basic_T = final_stack_el_t<T>;
  return reinterpret_cast<const basic_T*>(&mem[0]);
}

template <typename T> requires(valid_stack_el_type_v<T>)
auto stack_element::get() const -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  return *rawget<basic_T>();
}

template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, element_view>)
void stack_element::set(const T& val) {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_el_view_v<basic_T>) {
    memcpy(mem, val._mem, MAXIMUM_STACK_VAL_SIZE);
  } else *reinterpret_cast<basic_T*>(&mem[0]) = val;
}

template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, stack_element::view> || std::is_same_v<std::remove_cvref_t<T>, any_stack>)
any_stack::any_stack(const T& val) noexcept : _type(utils::type_name<final_stack_el_t<T>>()) {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_typeless_v<basic_T>) {
    _type = val.type();
    memcpy(_mem, val._mem, MAXIMUM_STACK_VAL_SIZE);
  } else *reinterpret_cast<basic_T*>(_mem[0]) = val;
}

template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, stack_element::view> || std::is_same_v<std::remove_cvref_t<T>, any_stack>)
bool any_stack::is() const {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_typeless_v<basic_T>) {
    return true;
  } else if constexpr (std::is_pointer_v<basic_T>) {
    using no_ptr_t = std::remove_cvref_t<std::remove_pointer_t<basic_T>>;
    return _type == utils::type_name<no_ptr_t*>() || _type == utils::type_name<const no_ptr_t*>();
  } else return _type == utils::type_name<basic_T>();
}

template <typename T> requires(valid_stack_el_type_v<T> || std::is_same_v<std::remove_cvref_t<T>, stack_element::view> || std::is_same_v<std::remove_cvref_t<T>, any_stack>)
auto any_stack::get() const -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if (!is<basic_T>()) throw std::runtime_error(std::format("Stack value view contains '{}' type, but '{}' is requested", _type, utils::type_id<basic_T>()));
  if constexpr (is_typeless_v<basic_T>) {
    return stack_element::view(_mem, _type);
  } else return *reinterpret_cast<const basic_T*>(&_mem[0]);
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
