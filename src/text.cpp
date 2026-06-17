#include "devils_script/text.h"

#include "devils_script/string-utils.hpp"
#include <charconv>
#include <format>
#include <stdexcept>
#include <iostream>

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

namespace text {
bool is_bool(const std::string_view& str) noexcept {
  return str == "true" || str == "false";
}

bool as_bool(const std::string_view& str) noexcept {
  return str == "true";
}

bool is_number(const std::string_view& str, double& val) noexcept {
  const auto& last = str.data() + str.size();
  const auto [ptr, ec] = std::from_chars(str.data(), last, val);
  return ec == std::errc() && ptr == last;
}

bool is_number(const std::string_view& str) noexcept {
  double val;
  return is_number(str, val);
}

double as_number(const std::string_view& str) noexcept {
  double val;
  const bool ret = is_number(str, val);
  if (!ret) return 0.0;
  return val;
}

bool is_block(const std::string_view& text) noexcept {
  const auto tmp = utils::string::trim(text);
  return tmp[0] == '{' && tmp.back() == '}';
}

std::string_view remove_brackets(const std::string_view& block) noexcept {
  const auto tmp = utils::string::trim(block);
  if (!is_block(tmp)) return block;
  return utils::string::trim(tmp.substr(1, tmp.size() - 2));
}

bool is_string(const std::string_view& text) noexcept {
  return !is_bool(text) && !is_number(text) && !is_block(text);
}

constexpr std::string_view engalpha = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz_";
constexpr std::string_view specials = "!@#$%^&*-+=<>?/;\\|`~";
constexpr std::string_view numbers = "1234567890";
constexpr std::string_view lvalue_chars = ".:";
constexpr std::string_view invalid = "{},";
constexpr std::string_view reserved_tokens[] = {
  //"unary_minus", "unary_plus", 
  "condition", "custom_description", "value", "weight", //"arg", "ctx", "count", "percent", "order_by",
  "__empty_lvalue", "__effect_block", "__string_block", "__object_block"
};
constexpr size_t reserved_tokens_size = sizeof(reserved_tokens) / sizeof(reserved_tokens[0]);
constexpr std::string_view tokens_ignore_list[] = {
  "condition", "custom_description", "value", "weight", //"arg", "ctx", "count", "percent", "order_by",
};
constexpr size_t tokens_ignore_list_size = sizeof(tokens_ignore_list) / sizeof(tokens_ignore_list[0]);
constexpr std::string_view reserved_operators[] = { "=", "?=", "//", "/*", "*/" };
constexpr size_t reserved_operators_size = sizeof(reserved_operators) / sizeof(reserved_operators[0]);

constexpr static bool is_common_english(const char c) {
  return std::any_of(engalpha.begin(), engalpha.end(), [c](const char ac) { return ac == c; });
}

constexpr static bool is_special(const char c) {
  return std::any_of(specials.begin(), specials.end(), [c](const char ac) { return ac == c; });
}

constexpr static bool is_number(const char c) {
  return std::any_of(numbers.begin(), numbers.end(), [c](const char ac) { return ac == c; });
}

constexpr static bool is_invalid_character(const char c) {
  return std::any_of(invalid.begin(), invalid.end(), [c](const char ac) { return ac == c; });
}

constexpr static bool is_lvalue_character(const char c) {
  return std::any_of(lvalue_chars.begin(), lvalue_chars.end(), [c](const char ac) { return ac == c; });
}

constexpr static bool is_reserved_word(const std::string_view &str) {
  return std::any_of(reserved_tokens, reserved_tokens+reserved_tokens_size, [&str](const std::string_view& reserv) { return reserv == str; });
}

constexpr static bool is_reserved_operator(const std::string_view& str) {
  return std::any_of(reserved_operators, reserved_operators+reserved_operators_size, [&str](const std::string_view& reserv) { return reserv == str; });
}

bool is_valid_lvalue(const std::string_view& str) noexcept {
  if (str.empty()) return false;
  if (isdigit(str[0])) return false;
  return std::all_of(str.begin(), str.end(), [](const char c) { 
    return is_common_english(c) || is_number(c) || is_lvalue_character(c);
  });
}

bool is_valid_rvalue(const std::string_view& str) noexcept {
  if (str.empty()) return false;
  return std::all_of(str.begin(), str.end(), [](const char c) { 
    return !is_invalid_character(c);
  });
}

bool is_valid_function_name(const std::string_view& str) noexcept {
  if (str.empty()) return false;
  if (isdigit(str[0])) return false;
  if (is_reserved_word(str)) return false;

  return std::all_of(str.begin(), str.end(), [](const char c) {
    return is_common_english(c) || is_number(c);
  });
}

bool is_valid_operator_name(const std::string_view& str) noexcept {
  if (str.empty()) return false;
  if (is_reserved_word(str)) return false;
  if (is_reserved_operator(str)) return false;
  return std::all_of(str.begin(), str.end(), [](const char c) {
    return is_common_english(c) || is_number(c) || is_special(c);
  });
}

bool is_special_operator(const std::string_view& str) noexcept {
  return std::all_of(str.begin(), str.end(), [](const char c) {
    return is_special(c);
  });
}

bool has_special_symbols(const std::string_view& str) noexcept {
  return std::any_of(str.begin(), str.end(), [](const char c) {
    return is_special(c);
  });
}

bool has_common_symbols(const std::string_view& str) noexcept {
  return std::any_of(str.begin(), str.end(), [](const char c) {
    return is_common_english(c) || is_number(c);
  });
}

bool is_in_ignore_list(const std::string_view& str) noexcept {
  return std::any_of(tokens_ignore_list, tokens_ignore_list + tokens_ignore_list_size, [&str](const std::string_view& reserv) { return reserv == str; });
}

}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}