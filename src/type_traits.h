#ifndef DEVILS_SCRIPT_TYPE_TRAITS_H
#define DEVILS_SCRIPT_TYPE_TRAITS_H

#include <string_view>
#include <type_traits>
#include <functional>
#include <optional>

#if __cplusplus >= 202002L
#  include <span>
#else
#  define TCB_SPAN_NAMESPACE_NAME std
#  include "tcb/span.hpp"
#endif

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
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

    namespace detail {
      // primary template.
      template<typename T>
      struct function_traits : public function_traits<decltype(&T::operator())> {};

      // partial specialization for function type
      template<typename R, typename... Args>
      struct function_traits<R(Args...)> {
        using member_of = void;
        using result_type = R;
        using argument_types = std::tuple<Args..., void>;
        using arguments_tuple_t = std::tuple<Args...>;
        static constexpr function_class class_e = function_class::type;
        static constexpr size_t argument_count = std::tuple_size_v<argument_types> - 1;
      };

      // partial specialization for function pointer
      template<typename R, typename... Args>
      struct function_traits<R (*)(Args...)> {
        using member_of = void;
        using result_type = R;
        using argument_types = std::tuple<Args..., void>;
        using arguments_tuple_t = std::tuple<Args...>;
        static constexpr function_class class_e = function_class::pointer;
        static constexpr size_t argument_count = std::tuple_size_v<argument_types> - 1;
      };

      template<typename R, typename... Args>
      struct function_traits<R (*)(Args...) noexcept> {
        using member_of = void;
        using result_type = R;
        using argument_types = std::tuple<Args..., void>;
        using arguments_tuple_t = std::tuple<Args...>;
        static constexpr function_class class_e = function_class::pointer_noexcept;
        static constexpr size_t argument_count = std::tuple_size_v<argument_types> - 1;
      };

      // partial specialization for std::function
      template<typename R, typename... Args>
      struct function_traits<std::function<R(Args...)>> {
        using member_of = void;
        using result_type = R;
        using argument_types = std::tuple<Args..., void>;
        using arguments_tuple_t = std::tuple<Args...>;
        static constexpr function_class class_e = function_class::std_function;
        static constexpr size_t argument_count = std::tuple_size_v<argument_types> - 1;
      };

      // partial specialization for pointer-to-member-function (i.e., operator()'s)
      template<typename T, typename R, typename... Args>
      struct function_traits<R (T::*)(Args...)> {
        using member_of = T;
        using result_type = R;
        using argument_types = std::tuple<Args..., void>;
        using arguments_tuple_t = std::tuple<Args...>;
        static constexpr function_class class_e = function_class::member;
        static constexpr size_t argument_count = std::tuple_size_v<argument_types> - 1;
      };

      // operator() noexcept
      template<typename T, typename R, typename... Args>
      struct function_traits<R (T::*)(Args...) noexcept> {
        using member_of = T;
        using result_type = R;
        using argument_types = std::tuple<Args..., void>;
        using arguments_tuple_t = std::tuple<Args...>;
        static constexpr function_class class_e = function_class::member_noexcept;
        static constexpr size_t argument_count = std::tuple_size_v<argument_types> - 1;
      };

      // operator() const
      template<typename T, typename R, typename... Args>
      struct function_traits<R (T::*)(Args...) const> {
        using member_of = T;
        using result_type = R;
        using argument_types = std::tuple<Args..., void>;
        using arguments_tuple_t = std::tuple<Args...>;
        static constexpr function_class class_e = function_class::member_const;
        static constexpr size_t argument_count = std::tuple_size_v<argument_types> - 1;
      };

      // operator() const noexcept
      template<typename T, typename R, typename... Args>
      struct function_traits<R (T::*)(Args...) const noexcept> {
        using member_of = T;
        using result_type = R;
        using argument_types = std::tuple<Args..., void>;
        using arguments_tuple_t = std::tuple<Args...>;
        static constexpr function_class class_e = function_class::member_const_noexcept;
        static constexpr size_t argument_count = std::tuple_size_v<argument_types> - 1;
      };

      /// Simple type introspection without RTTI.
      template <typename T>
      constexpr std::string_view get_type_name() {
        //std::cout << __PRETTY_FUNCTION__ << "\n";
        //std::cout << __FUNCSIG__ << "\n";
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
        const int left = len & 7;
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

      // до с++20 придется использовать что то такое, в каком нибудь самом идиотском сценарии кто то может решить что возвращать std::false_type хорошая идея
      // гоните таких людей ссаными тряпками
#define HAS_MEM_FUNC(name, func)                                    \
  template<typename C, typename... Args>                            \
  class has_##name {                                                \
  private:                                                          \
    template<typename T>                                            \
    static constexpr auto check(T*)                                 \
    -> decltype( std::declval<T>().func( std::declval<Args>()... ) ); \
    template<typename>                                              \
    static constexpr std::false_type check(...);                    \
    using type = decltype(check<C>(0));                             \
  public:                                                           \
    static constexpr bool value = !std::is_same_v<type, std::false_type>; \
  };                                                                \
  template <typename T, typename... Args>                           \
  constexpr bool has_##name##_v = has_##name <T, Args...>::value;   \


#define HAS_MEM_FUNC_ARGS(name, func, ...)                          \
  template<typename C, typename... Args>                            \
  class has_##name {                                                \
  private:                                                          \
    template<typename T>                                            \
    static constexpr auto check(T*)                                 \
    -> decltype( std::declval<T>().func( std::declval<Args>()... ) ); \
    template<typename>                                              \
    static constexpr std::false_type check(...);                    \
    using type = decltype(check<C>(0));                             \
  public:                                                           \
    static constexpr bool value = !std::is_same_v<type, std::false_type>; \
  };                                                                \
  template <typename T>                                             \
  constexpr bool has_##name##_v = has_##name <T , ##__VA_ARGS__>::value;   \


#define HAS_BASIC_MEM_FUNC(func) HAS_MEM_FUNC(func, func)
#define HAS_BASIC_MEM_FUNC_ARGS(func, ...) HAS_MEM_FUNC_ARGS(func, func, ##__VA_ARGS__)

      HAS_MEM_FUNC_ARGS(operator_bool, operator bool )
      HAS_MEM_FUNC_ARGS(operator_dereference, operator*)
      HAS_MEM_FUNC_ARGS(operator_pointer_access, operator->)
      HAS_BASIC_MEM_FUNC_ARGS(valid)
      HAS_BASIC_MEM_FUNC_ARGS(invalid)

      template <typename T, typename F>
      constexpr bool is_member() {
        using base_type = typename detail::function_traits<F>::member_of;
        using clear_type = std::remove_cv_t< std::remove_reference_t< std::remove_pointer_t< T > > >;
        if constexpr (std::is_void_v<base_type> || std::is_void_v<clear_type>) return false;
        else if constexpr (std::is_pointer_v<T>) {
          if constexpr (std::is_same_v<base_type, clear_type>) return true;
          if constexpr (std::is_base_of_v<base_type, clear_type>) return true;
        } else {
          if constexpr (has_operator_dereference_v<T>) {
            using type = decltype(T().operator*());
            using clear_type = std::remove_cv_t< std::remove_reference_t< std::remove_pointer_t< type > > >;
            if constexpr (std::is_same_v<base_type, clear_type>) return true;
            if constexpr (std::is_base_of_v<base_type, clear_type>) return true;
          } else if constexpr (has_operator_pointer_access_v<T>) {
            using type = decltype(T().operator->());
            using clear_type = std::remove_cv_t< std::remove_reference_t< std::remove_pointer_t< type > > >;
            if constexpr (std::is_same_v<base_type, clear_type>) return true;
            if constexpr (std::is_base_of_v<base_type, clear_type>) return true;
          } else {
            if constexpr (std::is_same_v<base_type, clear_type>) return true;
            if constexpr (std::is_base_of_v<base_type, clear_type>) return true;
          }
        }
        return false;
      }

      template <typename T>
      struct is_optional : public std::false_type {
        using underlying_type = void;
      };
      template <typename T>
      struct is_optional<std::optional<T>> : public std::true_type {
        using underlying_type = T;
      };

      template <typename T>
      struct is_span : public std::false_type {
        using underlying_type = void;
      };
      template <typename T, size_t U>
      struct is_span<std::span<T, U>> : public std::true_type {
        using underlying_type = T;
      };
    }

    template <char ... C>
    struct string_literal {
      static constexpr const size_t size = sizeof...(C)+1;
      static constexpr const char value[size] = {C..., '\0'};
      constexpr operator const char* () const { return value; }
    };
    //template <char ... C> constexpr const char string_literal<C...>::size;
    template <char ... C> constexpr const char string_literal<C...>::value[string_literal<C...>::size];

//     template <size_t N, size_t... I>
//     constexpr void copy_chars_impl(const char* str, char* dest, std::index_sequence<I...>) {
//       dest[I] = str[I]...;
//     }
//
//     template <size_t N, typename Indices = std::make_index_sequence<N>>
//     constexpr void copy_chars(const char* str, char* dest) {
//
//     }

//     template <size_t N>
//     struct string_literal2 {
//       char value[N];
//       constexpr string_literal2(const char (&str)[N]) { for (size_t i = 0; i < N; ++i) value[i] = str[i]; }
//       constexpr operator const char* () const { return value; }
//     };

    //constexpr string_literal2 lit("abc");

    template<typename T, size_t N>
    using function_argument_type = typename std::tuple_element<N, typename detail::function_traits<T>::argument_types>::type;

    template<typename T>
    using function_result_type = typename detail::function_traits<T>::result_type;

    template<typename T>
    using function_member_of = typename detail::function_traits<T>::member_of;

    template<typename T>
    using function_arguments_tuple_t = typename detail::function_traits<T>::arguments_tuple_t;

    template<typename T>
    constexpr enum function_class function_class_e = detail::function_traits<T>::class_e;

    template<typename T>
    constexpr size_t get_function_argument_count() { return detail::function_traits<T>::argument_count; }

    template<typename T>
    constexpr std::string_view type_name() {
      using type = std::remove_reference_t<std::remove_cv_t<T>>;
      return detail::get_type_name<type>();
    }

    template <typename F>
    using final_output_type = std::remove_cv_t< std::remove_reference_t< function_result_type<F> > >;

    template <typename F, size_t N>
    using final_arg_type = std::remove_cv_t< std::remove_reference_t< function_argument_type<F, N> > >;

    template <typename T>
    using clear_type_t = std::remove_cv_t< std::remove_reference_t< std::remove_pointer_t< T > > >;

    template <typename T, typename F>
    constexpr bool is_member_v = detail::is_member<T, F>();

    template <typename T>
    constexpr bool is_optional_v = detail::is_optional<T>::value;

    template <typename T>
    using optional_type = typename detail::is_optional<T>::underlying_type;

    template <typename T>
    constexpr bool is_span_v = detail::is_span<T>::value;

    template <typename T>
    using span_type = typename detail::is_span<T>::underlying_type;


    constexpr uint64_t default_murmur_seed = 14695981039346656037ull;
    constexpr uint64_t murmur_hash64A(const std::string_view &in_str, const uint64_t seed) {
      return detail::murmur_hash64A(in_str, seed);
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

template <typename CharT, CharT... Cs>
constexpr devils_script::string_literal<Cs...> operator"" _create() { return {}; }

#endif
