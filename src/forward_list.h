#ifndef DEVILS_ENGINE_SCRIPT_FORWARD_LIST_H
#define DEVILS_ENGINE_SCRIPT_FORWARD_LIST_H

#include <utility>

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    template <typename T>
    struct forward_list {
      using pointer = forward_list<T>*;

      struct iterator {
        pointer cur;

        bool operator==(const iterator &b) const { return cur == b.cur; }
        bool operator!=(const iterator &b) const { return !operator==(b); }

        iterator & operator++() {
          cur = cur->next;
          return *this;
        }

        iterator operator++(int) {
          auto ret = *this;
          cur = cur->next;
          return ret;
        }

        T & operator*() { return cur->data; }
        const T & operator*() const { return cur->data; }
        T* operator->() { return &cur->data; }
        const T* operator->() const { return &cur->data; }
      };

      struct list_view {
        iterator start;

        iterator begin() const { return start; }
        iterator end() const { return iterator{nullptr}; }
      };

      T data;
      forward_list<T>* next;

      forward_list() noexcept : next(nullptr) {}
      explicit forward_list(T&& data) noexcept : data(std::forward<T>(data)), next(nullptr) {}
//       template <typename... Args>
//       forward_list(Args&&... args) : data(std::forward<Args>(args)...), next(nullptr) {}
      forward_list(const forward_list &copy) noexcept = default;
      forward_list(forward_list &&move) noexcept = default;
      ~forward_list() noexcept = default;

      forward_list & operator=(const forward_list &copy) noexcept = default;
      forward_list & operator=(forward_list &&move) noexcept = default;

      list_view view() { return list_view{iterator{this}}; }
    };

    template <typename T>
    struct is_forward_list_t : public std::false_type {
      using underlying_type = void;
    };

    template <typename T>
    struct is_forward_list_t<forward_list<T>> : public std::true_type {
      using underlying_type = T;
    };

    template <typename T>
    struct is_forward_list_t<const forward_list<T>> : public std::true_type {
      using underlying_type = T;
    };

    template <typename T>
    struct is_forward_list_t<forward_list<T>&> : public std::true_type {
      using underlying_type = T;
    };

    template <typename T>
    struct is_forward_list_t<const forward_list<T>&> : public std::true_type {
      using underlying_type = T;
    };

    template <typename T>
    struct is_forward_list_t<forward_list<T>*> : public std::true_type {
      using underlying_type = T;
    };

    template <typename T>
    struct is_forward_list_t<const forward_list<T>*> : public std::true_type {
      using underlying_type = T;
    };

    template <typename T>
    constexpr bool is_forward_list_v = is_forward_list_t<T>::value;

    template <typename T>
    using forward_list_type = typename is_forward_list_t<T>::underlying_type;
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
