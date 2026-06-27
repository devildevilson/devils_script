#include "devils_script/context.h"

#include <cstring>
#include <iostream>
#include <stdexcept>

namespace devils_script {

context::context(const size_t stack_capacity, const size_t saved_capacity) noexcept
  : stack(stack_capacity), saved_stack(saved_capacity), args_stack(script_arguments_size),
    prng_state(0xdeadbab1ull), current_index(0), frame_base(0), arg_base(0), saved_base(0), list_base(0), userptr(nullptr), current_script(nullptr),
    trace([](const std::string& msg) { std::cout << msg << '\n'; }) {
  // Saved values and args are indexed directly by compiled code, not pushed sequentially.
  saved_stack._size = saved_stack._data.size();
  args_stack._size = args_stack._data.size();
}

context::context() noexcept : context(stack_size, local_vars_size) {}

void context::set_return(const std::string_view& type, const stack_element& el) { _return_value = any_stack(el.mem, type); }
std::string_view context::arg_type(const int64_t index) const { return args_stack.type(index); }
std::string_view context::saved_type(const int64_t index) const { return saved_stack.type(index); }
std::string_view context::return_type() const { return _return_value.type(); }
void context::clear() { current_index = 0; frame_base = 0; arg_base = 0; saved_base = 0; list_base = 0; stack.resize(0); }

context::stack_t::stack_t(const size_t max) noexcept : _size(0) { _data.resize(max); _types.resize(max); }

auto context::stack_t::get_view() const -> stack_element::view {
  if (_size == 0) return stack_element::view();
  return stack_element::view(_data[_size-1].mem, _types[_size-1]);
}

auto context::stack_t::get_view(const int64_t index) const -> stack_element::view {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  if (final_index >= int64_t(_size)) return stack_element::view();
  return stack_element::view(_data[final_index].mem, _types[final_index]);
}

void context::stack_t::erase() {
  _size = _size > 0 ? _size-1 : _size;
}

void context::stack_t::erase(const int64_t index) {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  if (final_index == int64_t(_size)-1) erase();
  else if (final_index < int64_t(_size)-1) {
    memmove(&_data[final_index], &_data[final_index+1], sizeof(char) * MAXIMUM_STACK_VAL_SIZE);
    memmove(&_types[final_index], &_types[final_index+1], sizeof(char) * MAXIMUM_STACK_VAL_SIZE);
    _size -= 1;
  }
}

void context::stack_t::resize(const size_t size) {
  if (size > _data.size()) throw std::runtime_error("Stack overflow");
  _size = size;
}

stack_element context::stack_t::element() const {
  return _size > 0 ? _data[_size - 1] : stack_element();
}

stack_element context::stack_t::element(const int64_t index) const {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  return final_index < int64_t(_size) ? _data[final_index] : stack_element();
}

std::string_view context::stack_t::type() const {
  return _size > 0 ? _types[_size - 1] : std::string_view();
}

std::string_view context::stack_t::type(const int64_t index) const {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  return final_index < int64_t(_size) ? _types[final_index] : std::string_view();
}

size_t context::stack_t::size() const { return _size; }

void context::stack_t::push(const std::string_view& type, const stack_element& el) {
  if (_size >= _data.size()) throw std::runtime_error("Stack overflow");
  _data[_size] = el;
  _types[_size] = type;
  _size += 1;
}

bool context::stack_t::invalid(const int64_t index) const {
  const int64_t final_index = index >= 0 ? index : int64_t(_size) + index;
  return final_index < int64_t(_size) ? _data[final_index].invalid() : true;
}

}
