#pragma once

#include <cstdint>
#include <cstddef> 
#include <array> 
#include <string_view>
#include <string>
#include <cstring>
#include "devils_script/type_traits.h"
#include "devils_script/common.h"
#include "devils_script/container.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

#ifndef DEVILS_SCRIPT_DEFAULT_STACK_SIZE
#define DEVILS_SCRIPT_DEFAULT_STACK_SIZE 256
#endif

struct context {
  static constexpr size_t script_arguments_size = 8;
  static constexpr size_t stack_size = DEVILS_SCRIPT_DEFAULT_STACK_SIZE;
  static constexpr size_t local_vars_size = 16;

  template <size_t N>
  struct stack_t {
    size_t _size;
    std::array<stack_element, N> _data;
    std::array<std::string_view, N> _types;

    stack_t() noexcept;

    template <typename T> requires(valid_stack_type<T>)
    bool is() const;

    template <typename T> requires(valid_stack_type<T>)
    bool is(const int64_t index) const;

    template <typename T> requires(valid_stack_type<T>)
    auto get() const -> final_stack_el_t<T>;

    template <typename T> requires(valid_stack_type<T>)
    auto safe_get() const -> final_stack_el_t<T>;

    template <typename T> requires(valid_stack_type<T>)
    void push(const T& val);

    template <typename T> requires(valid_stack_type<T>)
    auto get(const int64_t index) const -> final_stack_el_t<T>;

    template <typename T> requires(valid_stack_type<T>)
    auto safe_get(const int64_t index) const -> final_stack_el_t<T>;

    template <typename T> requires(valid_stack_type<T>)
    void set(const int64_t index, const T& val);

    template <typename T> requires(valid_stack_type<T>)
    auto pop() -> final_stack_el_t<T>;

    template <typename T> requires(valid_stack_type<T>)
    auto safe_pop() -> final_stack_el_t<T>;

    auto get_view() const -> stack_element::view;
    auto get_view(const int64_t index) const -> stack_element::view;

    void erase();
    void erase(const int64_t index);
    void resize(const size_t size);
    stack_element element() const;
    stack_element element(const int64_t index) const;
    std::string_view type() const;
    std::string_view type(const int64_t index) const;
    size_t size() const;
    void push(const std::string_view &type, const stack_element &el);
    bool invalid(const int64_t index) const;
  private:
    template <typename T> requires(valid_stack_type<T>)
    auto rawget(const size_t index) const -> final_stack_el_t<T>*;
  };

  struct stack_t<stack_size> stack;
  struct stack_t<local_vars_size> saved_stack;
  struct stack_t<script_arguments_size> args_stack;

  uint64_t prng_state;
  size_t current_index;
  void* userptr;

  any_stack _return_value;

  std::vector<std::vector<stack_element>> lists;

  // prng_state - any non 0
  inline context() noexcept : prng_state(0xdeadbab1ull), current_index(0), userptr(nullptr) {
    // resize saved_stack because indices are controlled by script, no push/pop (?)
    saved_stack._size = saved_stack._data.size(); 
    // resize args_stack because indices are controlled by script, no push/pop (?)
    args_stack._size = args_stack._data.size();
  }

  template <typename T> requires(valid_stack_type<T>)
  bool is_arg(const size_t index) const;

  template <typename T> requires(valid_stack_type<T>)
  auto get_arg(const size_t index) const -> final_stack_el_t<T>;

  template <typename T> requires(valid_stack_type<T>)
  auto safe_get_arg(const size_t index) const -> final_stack_el_t<T>;

  template <typename T> requires(valid_stack_type<T>)
  void set_arg(const size_t index, const T& val);

  template <typename T> requires(valid_stack_type<T>)
  bool is_saved(const size_t index) const;

  template <typename T> requires(valid_stack_type<T>)
  auto get_saved(const size_t index) const -> final_stack_el_t<T>;

  template <typename T> requires(valid_stack_type<T>)
  auto safe_get_saved(const size_t index) const -> final_stack_el_t<T>;

  template <typename T> requires(valid_stack_type<T>)
  void set_saved(const size_t index, const T& val);

  template <typename T> requires(valid_stack_type<T>)
  bool is_return() const;

  template <typename T> requires(valid_stack_type<T>)
  auto get_return() const -> final_stack_el_t<T>;

  template <typename T> requires(valid_stack_type<T>)
  auto safe_get_return() const -> final_stack_el_t<T>;

  template <typename T> requires(valid_stack_type<T>)
  void set_return(const T& val);

  inline void set_return(const std::string_view& type, const stack_element& el) { _return_value = any_stack(el.mem, type); }
  inline std::string_view saved_type(const int64_t index) const { return saved_stack.type(index); }
  inline std::string_view return_type() const { return _return_value.type(); }
  inline void clear() { current_index = 0; stack.resize(0); /*saved_stack.resize(0);*/ }
  void create_lists(const container* scr);
};

template <size_t N>
context::stack_t<N>::stack_t() noexcept : _size(0) {}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
bool context::stack_t<N>::is() const {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_typeless_v<basic_T>) {
    return true;
  } else if constexpr (std::is_pointer_v<basic_T>) {
    using no_ptr_t = std::remove_cvref_t<std::remove_pointer_t<basic_T>>;
    return _size > 0 && (_types[_size - 1] == utils::type_name<no_ptr_t*>() || _types[_size - 1] == utils::type_name<const no_ptr_t*>());
  } else return _size > 0 && _types[_size - 1] == utils::type_name<basic_T>();
}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
bool context::stack_t<N>::is(const int64_t index) const {
  using basic_T = final_stack_el_t<T>;
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  if constexpr (is_typeless_v<basic_T>) {
    return final_index >= 0 && final_index < _size;
  } else if constexpr (std::is_pointer_v<basic_T>) {
    using no_ptr_t = std::remove_cvref_t<std::remove_pointer_t<basic_T>>;
    return final_index >= 0 && final_index < _size && (_types[final_index] == utils::type_name<no_ptr_t*>() || _types[final_index] == utils::type_name<const no_ptr_t*>());
  } else return final_index >= 0 && final_index < _size && _types[final_index] == utils::type_name<basic_T>();
}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
auto context::stack_t<N>::get() const -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_typeless_v<basic_T>) {
    return basic_T(_data[_size-1].mem, _types[_size-1]);
  } else return _data[_size-1].template get<basic_T>();
}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
auto context::stack_t<N>::safe_get() const -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if (!is<basic_T>()) throw std::runtime_error(std::format("Top of the stack has element with type '{}', but trying to get '{}'", type(), utils::type_name<basic_T>()));
  if constexpr (is_typeless_v<basic_T>) {
    return basic_T(_data[_size-1].mem, _types[_size-1]);
  } else return get<basic_T>();
}

// пушить без типа? врядли имеет большой смысл
template <size_t N>
template <typename T> requires(valid_stack_type<T>)
void context::stack_t<N>::push(const T& val) {
  using basic_T = final_stack_el_t<T>;
  if (_size >= _data.size()) throw std::runtime_error("Stack overflow");
  if constexpr (is_typeless_v<basic_T>) {
    //_data[_size].set(std::forward<basic_T>(val));
    memcpy(_data[_size].mem, val._mem, MAXIMUM_STACK_VAL_SIZE);
    _types[_size] = val.type();
  } else {
    _data[_size].set(val);
    _types[_size] = utils::type_name<basic_T>();
  }
  _size += 1;
}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
auto context::stack_t<N>::get(const int64_t index) const -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  if constexpr (is_typeless_v<basic_T>) {
    return basic_T(_data[final_index].mem, _types[final_index]);
  } else return _data[final_index].template get<basic_T>();
}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
auto context::stack_t<N>::safe_get(const int64_t index) const -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if (!is<basic_T>(index)) throw std::runtime_error(std::format("Stack element #{} has element with type '{}', but trying to get '{}'", index, type(index), utils::type_name<basic_T>()));
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  if constexpr (is_typeless_v<basic_T>) {
    return basic_T(_data[final_index].mem, _types[final_index]);
  } else return _data[final_index].template get<basic_T>();
}

// set без типа тоже имеет немного смысла
template <size_t N>
template <typename T> requires(valid_stack_type<T>)
void context::stack_t<N>::set(const int64_t index, const T& val) {
  using basic_T = final_stack_el_t<T>;
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  if (final_index >= _size) throw std::runtime_error("Stack overflow");
  if constexpr (is_typeless_v<basic_T>) {
    //_data[final_index].set(std::forward<basic_T>(val));
    memcpy(_data[final_index].mem, val._mem, MAXIMUM_STACK_VAL_SIZE);
    _types[final_index] = val.type();
  } else {
    _data[final_index].set(val);
    _types[final_index] = utils::type_name<basic_T>();
  }
}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
auto context::stack_t<N>::pop() -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if (_size == 0) throw std::runtime_error("Stack is empty");
  _size -= 1;
  if constexpr (is_typeless_v<basic_T>) {
    return basic_T(_data[_size].mem, _types[_size]);
  } else return _data[_size].template get<basic_T>();
}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
auto context::stack_t<N>::safe_pop() -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if (_size == 0) throw std::runtime_error("Stack is empty");
  if (!is<basic_T>()) throw std::runtime_error(std::format("Top of the stack has element with type '{}', but trying to get '{}'", type(), utils::type_name<basic_T>()));
  _size -= 1;
  if constexpr (is_typeless_v<basic_T>) {
    return basic_T(_data[_size].mem, _types[_size]);
  } else return _data[_size].template get<basic_T>();
}

template <size_t N>
auto context::stack_t<N>::get_view() const -> stack_element::view {
  if (_size == 0) return stack_element::view();
  return stack_element::view(_data[_size-1].mem, _types[_size-1]);
}

template <size_t N>
auto context::stack_t<N>::get_view(const int64_t index) const -> stack_element::view {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  if (final_index >= _size) return stack_element::view();
  return stack_element::view(_data[final_index].mem, _types[final_index]);
}

template <size_t N>
void context::stack_t<N>::erase() {
  _size = _size > 0 ? _size-1 : _size;
}

template <size_t N>
void context::stack_t<N>::erase(const int64_t index) {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  if (final_index == _size-1) erase();
  else if (final_index < _size-1) {
    memmove(&_data[final_index], &_data[final_index+1], sizeof(char) * MAXIMUM_STACK_VAL_SIZE);
    memmove(&_types[final_index], &_types[final_index+1], sizeof(char) * MAXIMUM_STACK_VAL_SIZE);
    _size -= 1;
  }
}

template <size_t N>
void context::stack_t<N>::resize(const size_t size) {
  if (size > _data.size()) throw std::runtime_error("Stack overflow");
  _size = size;
}

template <size_t N>
stack_element context::stack_t<N>::element() const {
  return _size > 0 ? _data[_size - 1] : stack_element();
}

template <size_t N>
stack_element context::stack_t<N>::element(const int64_t index) const {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  return final_index < _size ? _data[final_index] : stack_element();
}

template <size_t N>
std::string_view context::stack_t<N>::type() const {
  return _size > 0 ? _types[_size - 1] : std::string_view();
}

template <size_t N>
std::string_view context::stack_t<N>::type(const int64_t index) const {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  return final_index < _size ? _types[final_index] : std::string_view();
}

template <size_t N>
size_t context::stack_t<N>::size() const { return _size; }

template <size_t N>
void context::stack_t<N>::push(const std::string_view& type, const stack_element& el) {
  if (_size >= _data.size()) throw std::runtime_error("Stack overflow");
  _data[_size] = el;
  _types[_size] = type;
  _size += 1;
}

template <size_t N>
bool context::stack_t<N>::invalid(const int64_t index) const {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  return final_index < _size ? _data[final_index].invalid() : true;
}

template <size_t N>
template <typename T> requires(valid_stack_type<T>)
auto context::stack_t<N>::rawget(const size_t index) const -> final_stack_el_t<T>* {
  using basic_T = final_stack_el_t<T>;
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  return final_index < _size ? _data[final_index].template rawget<basic_T>() : nullptr;
}

template <typename T> requires(valid_stack_type<T>)
bool context::is_arg(const size_t index) const {
  return args_stack.is<T>(index);
}

template <typename T> requires(valid_stack_type<T>)
auto context::get_arg(const size_t index) const -> final_stack_el_t<T> {
  return args_stack.get<T>(index);
}

template <typename T> requires(valid_stack_type<T>)
auto context::safe_get_arg(const size_t index) const -> final_stack_el_t<T> {
  return args_stack.safe_get<T>(index);
}

template <typename T> requires(valid_stack_type<T>)
void context::set_arg(const size_t index, const T& val) {
  args_stack.set<T>(index, val);
}

template <typename T> requires(valid_stack_type<T>)
bool context::is_saved(const size_t index) const {
  return saved_stack.is<T>(index);
}

template <typename T> requires(valid_stack_type<T>)
auto context::get_saved(const size_t index) const -> final_stack_el_t<T> {
  return saved_stack.get<T>(index);
}

template <typename T> requires(valid_stack_type<T>)
auto context::safe_get_saved(const size_t index) const -> final_stack_el_t<T> {
  return saved_stack.safe_get<T>(index);
}

template <typename T> requires(valid_stack_type<T>)
void context::set_saved(const size_t index, const T& val) {
  saved_stack.set<T>(index, val);
}

template <typename T> requires(valid_stack_type<T>)
bool context::is_return() const {
  using basic_T = final_stack_el_t<T>;
  return return_type() == utils::type_name<basic_T>();
}

template <typename T> requires(valid_stack_type<T>)
auto context::get_return() const -> final_stack_el_t<T> {
  return _return_value.get<T>();
}

template <typename T> requires(valid_stack_type<T>)
auto context::safe_get_return() const -> final_stack_el_t<T> {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_typeless_v<basic_T>) {
    return basic_T(_return_value._mem, _return_value.type());
  } else {
    if (!is_return<basic_T>()) throw std::runtime_error(std::format("Could not get return value of type '{}', actual return value has type '{}'", utils::type_name<basic_T>(), _return_value.type()));
    return get_return<basic_T>();
  }
}

template <typename T> requires(valid_stack_type<T>)
void context::set_return(const T& val) {
  using basic_T = final_stack_el_t<T>;
  if constexpr (is_typeless_v<basic_T>) {
    _return_value = any_stack(val._mem, val.type());
  } else {
    _return_value = any_stack(val);
  }
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}