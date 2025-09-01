#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <functional>
#include <string_view>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <span>
#include <list>
#include <tuple>
#include <optional>

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

namespace utils {
  enum class function_class {
    type,
    pointer,
    pointer_noexcept,
    std_function,
    member,
    member_noexcept,
    member_const,
    member_const_noexcept,
    count
  };

  struct void_t {};
  template <typename T>
  constexpr bool is_void_v = std::is_void_v<std::remove_cvref_t<T>> || std::is_same_v<void_t, std::remove_cvref_t<T>>;
  template <typename T>
  using void_or_t = std::conditional_t<std::is_void_v<T>, utils::void_t, T>;

  namespace detail {

  template<typename> 
  struct function_traits_v2 { 
    using member_of = void_t;
    using result_type = void_t;
    using argument_types = std::tuple<void_t>;
    using arguments_tuple_t = void_t;
    static constexpr bool is_function = false;
    static constexpr size_t argument_count = 0;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args> 
  struct function_traits_v2<R(Args...)> { 
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = true;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<R(Args...) const> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = true;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<R(Args...) noexcept> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = true;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<R(Args...) volatile> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = true;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<R(Args...) const noexcept> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = true;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<R(Args...) const volatile> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = true;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<R(Args...) const volatile noexcept> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = true;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<std::function<R(Args...)>> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = true;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<std::function<R(Args...) const>> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = true;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<std::function<R(Args...) noexcept>> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = true;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<std::function<R(Args...) volatile>> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = true;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<std::function<R(Args...) const noexcept>> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = true;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<std::function<R(Args...) const volatile>> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = true;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<std::function<R(Args...) const volatile noexcept>> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = false;
    static constexpr bool is_std_function = true;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<R(*)(Args...)> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename... Args>
  struct function_traits_v2<R(*)(Args...) noexcept> {
    using member_of = void_t;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename C, typename... Args>
  struct function_traits_v2<R(C::*)(Args...)> {
    using member_of = C;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename C, typename... Args>
  struct function_traits_v2<R(C::*)(Args...) const> {
    using member_of = C;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename C, typename... Args>
  struct function_traits_v2<R(C::*)(Args...) noexcept> {
    using member_of = C;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename C, typename... Args>
  struct function_traits_v2<R(C::*)(Args...) volatile> {
    using member_of = C;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename C, typename... Args>
  struct function_traits_v2<R(C::*)(Args...) const noexcept> {
    using member_of = C;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = false;
    static constexpr bool is_noexcept = true;
  };

  template<typename R, typename C, typename... Args>
  struct function_traits_v2<R(C::*)(Args...) const volatile> {
    using member_of = C;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = false;
  };

  template<typename R, typename C, typename... Args>
  struct function_traits_v2<R(C::*)(Args...) const volatile noexcept> {
    using member_of = C;
    using result_type = R;
    using argument_types = std::tuple<Args..., void_t>;
    using arguments_tuple_t = std::tuple<std::remove_cvref_t<Args>...>;
    static constexpr bool is_function = true;
    static constexpr size_t argument_count = std::tuple_size_v<argument_types>-1;
    static constexpr bool is_function_type = false;
    static constexpr bool is_function_pointer = true;
    static constexpr bool is_std_function = false;
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = true;
    static constexpr bool is_noexcept = true;
  };

  /// Simple type introspection without RTTI.
  template <typename T>
  constexpr std::string_view get_type_name() {
#if defined(_MSC_VER)
    constexpr std::string_view start_char_seq = "get_type_name<";
    constexpr std::string_view end_char_seq = ">(void)";
    constexpr std::string_view function_type_pattern = ")(";
    constexpr std::string_view sig = __FUNCSIG__;
    constexpr size_t sig_size = sig.size()+1;
    constexpr size_t str_seq_name_start = sig.find(start_char_seq) + start_char_seq.size();
    constexpr size_t end_of_char_str = sig.rfind(start_char_seq);
    constexpr size_t count = sig_size - str_seq_name_start - end_char_seq.size() - 1; // отстается символ '>' в конце
    constexpr std::string_view substr = sig.substr(str_seq_name_start, count);
    if constexpr (substr.find(function_type_pattern) == std::string_view::npos) {
      constexpr std::string_view class_char_seq = "class ";
      constexpr std::string_view struct_char_seq = "struct ";
      const size_t class_seq_start = substr.find(class_char_seq);
      const size_t struct_seq_start = substr.find(struct_char_seq);
      if constexpr (class_seq_start == 0) return substr.substr(class_char_seq.size());
      if constexpr (struct_seq_start == 0) return substr.substr(struct_char_seq.size());;
    }
    return substr;
#elif defined(__clang__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
    constexpr std::string_view start_char_seq = "T = ";
    constexpr size_t sig_size = sig.size()+1;
    constexpr size_t str_seq_name_start = sig.find(start_char_seq) + start_char_seq.size();
    constexpr size_t end_of_char_str = 2;
    constexpr size_t count = sig_size - str_seq_name_start - end_of_char_str;
    return sig.substr(str_seq_name_start, count);
#elif defined(__GNUC__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
    constexpr std::string_view start_char_seq = "T = ";
    constexpr size_t sig_size = sig.size()+1;
    constexpr size_t str_seq_name_start = sig.find(start_char_seq) + start_char_seq.size();
    constexpr size_t end_of_char_str = sig_size - sig.find(';');
    constexpr size_t count = sig_size - str_seq_name_start - end_of_char_str;
    return sig.substr(str_seq_name_start, count);
#else
#error Compiler not supported for demangling
#endif
  }

  constexpr uint8_t to_u8(const char c) { return uint8_t(c); }

  constexpr uint64_t U8TO64_LE(const char* data) {
    return  uint64_t(to_u8(data[0]))        | (uint64_t(to_u8(data[1])) << 8)  | (uint64_t(to_u8(data[2])) << 16) |
          (uint64_t(to_u8(data[3])) << 24) | (uint64_t(to_u8(data[4])) << 32) | (uint64_t(to_u8(data[5])) << 40) |
          (uint64_t(to_u8(data[6])) << 48) | (uint64_t(to_u8(data[7])) << 56);
  }

  constexpr uint64_t U8TO64_LE(const uint8_t* data) {
    return  uint64_t(data[0])        | (uint64_t(data[1]) << 8)  | (uint64_t(data[2]) << 16) |
          (uint64_t(data[3]) << 24) | (uint64_t(data[4]) << 32) | (uint64_t(data[5]) << 40) |
          (uint64_t(data[6]) << 48) | (uint64_t(data[7]) << 56);
  }

  constexpr uint64_t to_u64(const char c) { return uint64_t(to_u8(c)); }

  constexpr uint64_t murmur_hash64A(const std::string_view& in_str, const uint64_t seed) {
    constexpr uint64_t m = 0xc6a4a7935bd1e995LLU;
    constexpr int r = 47;
    const size_t len = in_str.size();
    const size_t end = len - (len % sizeof(uint64_t));

    uint64_t h = seed ^ (len * m);

    for (size_t i = 0; i < end; i += 8) {
      uint64_t k = U8TO64_LE(&in_str[i]);
      k *= m;
      k ^= k >> r;
      k *= m;

      h ^= k;
      h *= m;
    }

    const auto key_end = in_str.substr(end);
    const size_t left = len & 7;
    switch (left) {
      case 7: h ^= to_u64(key_end[6]) << 48; [[fallthrough]];
      case 6: h ^= to_u64(key_end[5]) << 40; [[fallthrough]];
      case 5: h ^= to_u64(key_end[4]) << 32; [[fallthrough]];
      case 4: h ^= to_u64(key_end[3]) << 24; [[fallthrough]];
      case 3: h ^= to_u64(key_end[2]) << 16; [[fallthrough]];
      case 2: h ^= to_u64(key_end[1]) << 8;  [[fallthrough]];
      case 1: h ^= to_u64(key_end[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
  }

  template<typename>   constexpr bool is_optional_impl_v = false;
  template<typename T> constexpr bool is_optional_impl_v<std::optional<T>> = true;

  template<typename> struct optional_traits { using value_type = void_t; };
  template<typename T> struct optional_traits<std::optional<T>> { using value_type = T; };

  template<typename>   constexpr bool is_function_impl_v = false;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<R(Args...)> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<std::function<R(Args...)>> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<std::function<R(Args...) noexcept>> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<std::function<R(Args...) volatile>> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<std::function<R(Args...) const>> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<std::function<R(Args...) const volatile>> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<std::function<R(Args...) const noexcept>> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<std::function<R(Args...) volatile noexcept>> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<std::function<R(Args...) volatile const noexcept>> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<R(*)(Args...)> = true;
  template<typename R, typename... Args> constexpr bool is_function_impl_v<R(*)(Args...) noexcept> = true;
  template<typename R, typename C, typename... Args> constexpr bool is_function_impl_v<R(C::*)(Args...)> = true;
  template<typename R, typename C, typename... Args> constexpr bool is_function_impl_v<R(C::*)(Args...) noexcept> = true;
  template<typename R, typename C, typename... Args> constexpr bool is_function_impl_v<R(C::*)(Args...) volatile> = true;
  template<typename R, typename C, typename... Args> constexpr bool is_function_impl_v<R(C::*)(Args...) const> = true;
  template<typename R, typename C, typename... Args> constexpr bool is_function_impl_v<R(C::*)(Args...) const noexcept> = true;
  template<typename R, typename C, typename... Args> constexpr bool is_function_impl_v<R(C::*)(Args...) const volatile> = true;
  template<typename R, typename C, typename... Args> constexpr bool is_function_impl_v<R(C::*)(Args...) volatile noexcept> = true;
  template<typename R, typename C, typename... Args> constexpr bool is_function_impl_v<R(C::*)(Args...) const volatile noexcept> = true;

  template<typename F, std::size_t... S>
  constexpr void static_for(F&& function, std::index_sequence<S...>) {
    int unpack[] = { 0,
      (void(function(std::integral_constant<std::size_t, S>{})), 0)...
    };
    (void)unpack;
  }

  template<size_t I, typename T>
  struct remove_ith_type { using type = utils::void_t; };

  template<typename T, typename... Ts>
  struct remove_ith_type<0, std::tuple<T, Ts...>> {
    using type = std::tuple<Ts...>;
  };

  template<size_t I, typename T, typename... Ts>
  struct remove_ith_type<I, std::tuple<T, Ts...>> {
    using type = decltype(
      std::tuple_cat(
        std::declval<std::tuple<T>>(),
        std::declval<typename remove_ith_type<I - 1, std::tuple<Ts...>>::type>()
      )
    );
  };

  template<typename... Types>
  struct tuple_cat_type {
    using type = decltype(std::tuple_cat(std::declval<Types>()...));
  };
}

template <typename T>
constexpr T min(const T& a, const T& b) noexcept { return a < b ? a : b; }

template<typename T>
constexpr size_t function_arguments_count = detail::function_traits_v2<T>::argument_count;

template<typename T, size_t N>
using function_argument_type = typename std::tuple_element<min(N, function_arguments_count<T>), typename detail::function_traits_v2<T>::argument_types>::type;

template<typename T>
using function_result_type = typename detail::function_traits_v2<T>::result_type;

template<typename T>
using function_member_of = typename detail::function_traits_v2<T>::member_of;

template<typename T>
using function_argument_types = typename detail::function_traits_v2<T>::argument_types;

template<typename T>
using function_arguments_tuple_t = typename detail::function_traits_v2<T>::arguments_tuple_t;

template<typename T>
constexpr bool is_function_v = detail::function_traits_v2<T>::is_function;

template<typename T>
constexpr bool is_std_function_v = detail::function_traits_v2<T>::is_std_function;

template <typename Container>
struct is_container : std::false_type {};
template <typename... Ts>
struct is_container<std::array<Ts...>> : std::true_type {};
template <typename... Ts>
struct is_container<std::span<Ts...>> : std::true_type {};
template <typename T, size_t N>
struct is_container<std::array<T, N>> : std::true_type {};
template <typename T, size_t N>
struct is_container<std::span<T, N>> : std::true_type {};
template <typename... Ts>
struct is_container<std::vector<Ts...>> : std::true_type {};
template <typename... Ts>
struct is_container<std::list<Ts...>> : std::true_type {};
template <typename... Ts>
struct is_container<std::unordered_set<Ts...>> : std::true_type {};
template <typename... Ts>
struct is_container<std::set<Ts...>> : std::true_type {};

template <typename... Ts>
constexpr bool is_container_v = is_container<Ts...>::value;

template <typename Container>
struct is_map : std::false_type {};
template <typename... Ts>
struct is_map<std::unordered_map<Ts...>> : std::true_type {};
template <typename... Ts>
struct is_map<std::map<Ts...>> : std::true_type {};

template <typename... Ts>
constexpr bool is_map_v = is_map<Ts...>::value;

template <typename T>
constexpr bool is_optional_v = detail::is_optional_impl_v<std::remove_cvref_t<T>>;

template <typename T>
using optional_value_t = typename detail::optional_traits<T>::value_type;

template <size_t I, typename T>
using remove_ith_tuple_t = typename detail::remove_ith_type<I, T>::type;

template <typename... Ts>
using tuple_cat_t = typename detail::tuple_cat_type<Ts...>::type;

template <typename T>
constexpr std::string_view type_name() { return detail::get_type_name<T>(); }

constexpr uint64_t default_murmur_seed = 14695981039346656037ull;
constexpr uint64_t murmur_hash64A(const std::string_view &in_str, const uint64_t seed) {
  return detail::murmur_hash64A(in_str, seed);
}

template <typename T>
constexpr size_t type_id() {
  using type = std::remove_reference_t<std::remove_cv_t<T>>; // std::remove_pointer_t
  const auto name = detail::get_type_name<type>();
  return murmur_hash64A(name, default_murmur_seed);
}

template <typename F, size_t N>
constexpr std::string arg_type_name() {
  if constexpr (N < utils::function_arguments_count<F>) {
    using arg_type = function_argument_type<F, N>;
    return ", " + std::string(type_name<arg_type>()) + arg_type_name<F, N + 1>();
  } else return "";
}

template <typename F, size_t N>
constexpr std::string first_arg_type_name() {
  if constexpr (N < utils::function_arguments_count<F>) {
    using arg_type = function_argument_type<F, N>;
    return std::string(type_name<arg_type>()) + arg_type_name<F, N + 1>();
  } else return "";
}

template <typename F>
constexpr std::string make_function_sig_string() {
  using ret_type = function_result_type<F>;
  return std::string(type_name<ret_type>()) + " (" + first_arg_type_name<F, 0>() + ")";
}

template<std::size_t iterations, typename F>
constexpr void static_for(F&& function) {
  detail::static_for(std::forward<F>(function), std::make_index_sequence<iterations>());
}
}
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}