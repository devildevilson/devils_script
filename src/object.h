#ifndef DEVILS_ENGINE_SCRIPT_OBJECT_H
#define DEVILS_ENGINE_SCRIPT_OBJECT_H

#include <cstddef>
#include <string>
#include <cmath>
#include <cstring>
#include <limits>
#include "type_info.h"

// before c++20
#if __cplusplus >= 202002L
#  include <span>
#else
#  define TCB_SPAN_NAMESPACE_NAME std
#  include "tcb/span.hpp"
#endif

#define EPSILON 0.000001

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
#  define DEVILS_SCRIPT_FULL_NAMESPACE DEVILS_SCRIPT_OUTER_NAMESPACE::DEVILS_SCRIPT_INNER_NAMESPACE
#else
#  define DEVILS_SCRIPT_FULL_NAMESPACE DEVILS_SCRIPT_INNER_NAMESPACE
#endif

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    template <typename T>
    bool is_unresolved(T obj) {
      if constexpr (!std::is_fundamental_v<T>) {
        if constexpr (std::is_pointer_v<T>) return obj == nullptr;
        if constexpr (std::is_convertible_v<T, bool>) return !bool(obj);
        if constexpr (detail::has_valid_v<T>) {
          if constexpr (std::is_convertible_v<decltype(std::declval<T>().valid()), bool>) return !bool(obj.valid());
        }
        if constexpr (detail::has_invalid_v<T>) {
          if constexpr (std::is_convertible_v<decltype(std::declval<T>().invalid()), bool>) return bool(obj.invalid());
        }
      }
      return false;
    }

    template <typename T>
    constexpr static bool is_const_pointer_v = std::is_pointer_v<T> && std::is_const_v<std::remove_pointer_t<T>>;

    namespace detail {
      template<typename T>
      constexpr inline bool is(const size_t &type) noexcept {
        if constexpr (is_const_pointer_v<T>) {
          using non_const_ptr = std::remove_cv_t< std::remove_reference_t < std::remove_pointer_t<T> > > *;
          static_assert(std::is_pointer_v<non_const_ptr> && !std::is_const_v<non_const_ptr> && !std::is_const_v<std::remove_pointer_t<non_const_ptr>>);
          return type == type_id<T>() || type == type_id<non_const_ptr>();
        }
        return type == type_id<T>();
      }
      template<> constexpr inline bool is<void>(const size_t &type) noexcept   { return type == 0; }
      template<> constexpr inline bool is<bool>(const size_t &type) noexcept   { return type == type_id<bool>() || type == type_id<double>(); }
      template<> constexpr inline bool is<double>(const size_t &type) noexcept { return type == type_id<bool>() || type == type_id<double>(); }

      constexpr size_t align_to(size_t memory, size_t aligment) {
        return (memory + aligment - 1) / aligment * aligment;
      }
    }

    enum class init_ignore_value {};
    enum class init_unresolved_value {};

    struct object {
      static const size_t mem_size = detail::align_to(sizeof(size_t) + sizeof(void*), 8);

      size_t type;
      union {
        struct {
          void* data;
          double value;
        };

        struct {
          const void* const_data;
          size_t token;
        };

        struct {
          char mem[mem_size];
        };
      };

      template <typename T>
      inline static constexpr void check_type() {
        static_assert(sizeof(T) <= mem_size);
        static_assert(alignof(T) <= alignof(object));
        static_assert(std::is_trivially_destructible_v<T>, "Custom destructor is not supported");
        static_assert(std::is_copy_constructible_v<T>, "Type must be copyable");
        static_assert(!(std::is_pointer_v<T> && std::is_fundamental_v<std::remove_reference_t<std::remove_pointer_t<T>>>), "Do not store pointer to fundamental types, array views is supported");
        static_assert(!std::is_same_v<T, std::string_view*>, "Do not store pointer to string_view");
        static_assert(!std::is_same_v<T, const std::string_view*>, "Do not store pointer to string_view");
      }

      constexpr object() noexcept : type(0), data(nullptr), value(0) {}
      constexpr explicit object(const bool val) noexcept : type(type_id<bool>()), data(nullptr), value(val) { static_assert(type_id<bool>() != 0 && type_id<bool>() != SIZE_MAX); }
      constexpr explicit object(const double val) noexcept : type(type_id<double>()), data(nullptr), value(val) { static_assert(type_id<double>() != 0 && type_id<double>() != SIZE_MAX); }
      inline explicit object(const std::string_view val) noexcept : type(type_id<std::string_view>()), data(nullptr), value(0) {
        static_assert(type_id<std::string_view>() != 0 && type_id<std::string_view>() != SIZE_MAX);
        set_data(val);
      }
      inline explicit object(const std::string &val) noexcept : object(std::string_view(val)) {}

      template <typename T>
      explicit object(const std::span<T> val) noexcept : type(type_id<std::span<T>>()), data(nullptr), value(0) {
        static_assert(type_id<std::span<T>>() != 0 && type_id<std::span<T>>() != SIZE_MAX);
        set_data(val);
      }
      template <typename T, typename Alloc>
      explicit object(const std::vector<T, Alloc> &val) noexcept : object(std::span(val)) {}
      template <typename T, size_t N>
      explicit object(const std::array<T, N> &val) noexcept : object(std::span(val.data(), val.size())) {}

      template <typename T>
      object(T val) noexcept : type(type_id<T>()), data(nullptr), value(0) {
        static_assert(type_id<T>() != 0 && type_id<T>() != SIZE_MAX);
        check_type<T>();
        set_data(val);
      }

      constexpr explicit object(init_ignore_value) noexcept : type(SIZE_MAX), data(nullptr), value(-std::numeric_limits<double>::max()) {}
      constexpr explicit object(init_unresolved_value) noexcept : type(SIZE_MAX), data(nullptr), value(std::numeric_limits<double>::max()) {}

      ~object() noexcept = default;

      object(const object &copy) noexcept = default;
      object(object &move) noexcept = default;
      object & operator=(const object &copy) noexcept = default;
      object & operator=(object &move) noexcept = default;

      constexpr object & operator=(const bool val) noexcept { type = type_id<bool>(); data = nullptr; value = double(val); return *this; }
      constexpr object & operator=(const double val) noexcept { type = type_id<double>(); data = nullptr; value = val; return *this; }
      inline object & operator=(const std::string_view val) noexcept { type = type_id<std::string_view>(); data = nullptr; value = 0; set_data(val); return *this; }
      inline object & operator=(const std::string &val) noexcept { return operator=(std::string_view(val)); }

      template <typename T>
      object & operator=(const std::span<T> val) noexcept {
        static_assert(type_id<std::span<T>>() != 0 && type_id<std::span<T>>() != SIZE_MAX);
        type = type_id<std::span<T>>();
        data = nullptr;
        value = 0;
        set_data(val);
        return *this;
      }
      template <typename T, typename Alloc>
      object & operator=(const std::vector<T, Alloc> &val) noexcept { return operator=(std::span(val)); }
      template <typename T, size_t N>
      object & operator=(const std::array<T, N> &val) noexcept { return operator=(std::span(val.data(), val.size())); }

      template <typename T>
      object & operator=(T val) noexcept {
        static_assert(type_id<T>() != 0 && type_id<T>() != SIZE_MAX);
        type = type_id<T>();
        data = nullptr;
        value = 0;
        check_type<T>();
        set_data(val);
        return *this;
      }

      template <typename T>
      constexpr void set_data(T data) noexcept {
        auto ptr = &mem[0];
        new (ptr) T(data);
      }

      template<typename T>
      constexpr bool is() const noexcept { return detail::is<T>(type); }

      template<typename T>
      T get() const {
        // если требуется указатель на константную область памяти, но лежит указатель на не константную область
        // то мы все равно можем передать указатель, наверное нужно перекинуть проверку еще в object::is
        if constexpr (std::is_same_v<T, object>) {
          // ничего?
        } else if constexpr (is_const_pointer_v<T>) {
          using non_const_ptr = std::remove_cv_t< std::remove_reference_t < std::remove_pointer_t<T> > > *;
          static_assert(std::is_pointer_v<non_const_ptr> && !std::is_const_v<non_const_ptr> && !std::is_const_v<std::remove_pointer_t<non_const_ptr>>);
          if (!is<T>() && !is<non_const_ptr>()) {
            throw std::runtime_error(
              "Expected '" + std::string(type_name<T>()) + "' (" + std::to_string(type_id<T>()) + ") or '"
                           + std::string(type_name<non_const_ptr>()) + "' (" + std::to_string(type_id<non_const_ptr>()) + "), but another type stored (" + std::to_string(type) + ")"
            );
          }
        } else {
          if (!is<T>()) throw std::runtime_error("Expected '" + std::string(type_name<T>()) + "' (" + std::to_string(type_id<T>()) + "), but another type stored (" + std::to_string(type) + ")");
        }

        if constexpr (std::is_same_v<T, bool>) return !(std::abs(value) < EPSILON);
        else if constexpr (std::is_same_v<T, double>) return value;
        else if constexpr (std::is_same_v<T, object>) return *this;
        else {
          check_type<T>();
          auto ptr = &mem[0];
          return *reinterpret_cast<const T*>(ptr);
        }
        return T{};
      }

      constexpr bool lazy_type_compare(const object &another) const noexcept {
        const bool cur_num_or_bool = is<double>() || is<bool>();
        const bool ano_num_or_bool = another.is<double>() || another.is<bool>();
        if (cur_num_or_bool && ano_num_or_bool) return true;
        return type == another.type;
      }

      constexpr bool operator==(const object &another) const noexcept {
        if (is<double>() && another.is<double>()) {
          const double f = value;
          const double s = another.value;
          return std::abs(f-s) < EPSILON;
        }

        if (!lazy_type_compare(another)) return false;

        if (is<std::string_view>()) return get<std::string_view>() == another.get<std::string_view>();
        return memcmp(&mem[0], &another.mem[0], mem_size) == 0;
      }

      constexpr bool operator!=(const object &another) const noexcept { return !operator==(another); }

      constexpr size_t get_type() const noexcept { return type; }
      constexpr bool valid() const noexcept { return type != 0; }
      constexpr bool ignore() const noexcept     { return type == SIZE_MAX && data == nullptr && value == -std::numeric_limits<double>::max(); }
      constexpr bool unresolved() const noexcept { return type == SIZE_MAX && data == nullptr && value ==  std::numeric_limits<double>::max(); }
    };

    extern const object ignore_value;
    extern const object unresolved_value;

    template <typename T>
    constexpr bool is_valid_type_for_object_v =
      std::is_same_v<T, object> ||
      (sizeof(T) <= object::mem_size && alignof(T) <= alignof(object) &&
      std::is_trivially_destructible_v<T> && std::is_copy_constructible_v<T> &&
      !(std::is_pointer_v<T> && std::is_fundamental_v<std::remove_reference_t<std::remove_pointer_t<T>>>) &&
      !std::is_same_v<T, std::string_view*> &&
      !std::is_same_v<T, const std::string_view*>)
    ;

    namespace detail {
      template<> constexpr inline bool is<DEVILS_SCRIPT_FULL_NAMESPACE::object>(const size_t &) noexcept { return true; }
    }
  }
#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

namespace std {
  template<>
  struct hash<DEVILS_SCRIPT_FULL_NAMESPACE::object> {
    size_t operator() (const DEVILS_SCRIPT_FULL_NAMESPACE::object &obj) const {
      if (obj.is<std::string_view>()) return DEVILS_SCRIPT_FULL_NAMESPACE::murmur_hash64A(obj.get<std::string_view>(), DEVILS_SCRIPT_FULL_NAMESPACE::default_murmur_seed);
      const char* mem = reinterpret_cast<const char*>(&obj);
      return DEVILS_SCRIPT_FULL_NAMESPACE::murmur_hash64A(std::string_view(mem, sizeof(DEVILS_SCRIPT_FULL_NAMESPACE::object)), DEVILS_SCRIPT_FULL_NAMESPACE::default_murmur_seed);
    }
  };
}

#endif
