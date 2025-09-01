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
void remove_comment_blocks(std::string& script) noexcept {
  size_t pos_line_comment = std::string::npos;
  size_t pos_block_comment = std::string::npos;
  size_t curpos = 0;
  size_t endindex = 0;
  while (curpos < script.size()) {
    if (pos_line_comment != std::string::npos) {
      const size_t end_index = script.find("\n", curpos);
      auto beg = script.begin()+curpos;
      auto end = end_index != std::string::npos ? script.begin()+end_index : script.end();
      std::transform(beg, end, beg, [](const char& c) { return ' '; });
      curpos = end_index;
    } else if (pos_block_comment != std::string::npos) {
      const size_t end_index = script.find("*/", curpos);
      auto beg = script.begin()+curpos;
      auto end = end_index != std::string::npos ? script.begin()+end_index : script.end();
      std::transform(beg, end, beg, [](const char& c) { return ' '; });
      curpos = end_index;
    } else {
      pos_line_comment = script.find("//", curpos);
      pos_block_comment = script.find("/*", curpos);
      curpos = std::min(pos_line_comment, pos_block_comment);
    }
  }
}

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

bool is_in_ignore_list(const std::string_view& str) noexcept {
  return std::any_of(tokens_ignore_list, tokens_ignore_list + tokens_ignore_list_size, [&str](const std::string_view& reserv) { return reserv == str; });
}

#ifdef DEVILS_SCRIPT_USE_LUA
size_t compute_table_size(sol::table t) { size_t counter = 0; for (const auto& p : t) { ++counter; } return counter; }

std::string convert_lua(sol::object obj) {
  if (obj.is<bool>()) return obj.as<bool>() ? "true" : "false";
  if (obj.is<int64_t>()) return std::format("{}", obj.as<int64_t>());
  if (obj.is<double>()) return std::format("{}", obj.as<double>());
  if (obj.is<std::string_view>()) return std::string(obj.as<std::string_view>());
  if (!obj.is<sol::table>()) throw std::runtime_error("Invalid object type");

  std::string str;

  str += "{";
  size_t counter = 0;
  sol::table t = obj;
  for (const auto &pair : t) {
    auto lvalue = pair.first;
    auto rvalue = pair.second;

    while (lvalue.is<double>() && rvalue.is<sol::table>() && compute_table_size(rvalue.as<sol::table>()) == 1) {
      auto tbl = rvalue.as<sol::table>();
      for (const auto &pair : tbl) { lvalue = pair.first; rvalue = pair.second; }
    }

    if (counter != 0) str += ",";
    ++counter;

    if (lvalue.is<std::string_view>()) {
      str += lvalue.as<std::string_view>();
      str += "=";
    }

    str += convert_lua(rvalue);
  }
  str += "}";
  return str;
}
#endif

size_t split_tokens(const std::string_view& block, std::string_view* arr, const size_t max_size, std::vector<size_t>& stack) {
  /*if (!text::is_block(block)) {
    if (max_size > 0) { arr[0] = block; return 1; }
    return 0;
  }*/

  size_t counter = 0;
  const size_t stack_start = stack.size();
  //const auto str = utils::string::trim(text::remove_brackets(block));
  const auto str = block;
  size_t str_start = 0;
  for (size_t i = 0; i < str.size() && counter < max_size; ++i) {
    if (str[i] == ',' && stack.size() == stack_start) {
      const auto part = utils::string::trim(str.substr(str_start, i - str_start));
      if (part.empty()) throw std::runtime_error(std::format("Could not parse block '{}'", block));
      arr[counter] = part;
      counter += 1;
      str_start = i + 1;
    }

    if (str[i] == '{') stack.push_back(i);
    if (str[i] == '(') stack.push_back(i);

    if (str[i] == '}') {
      if (stack.empty()) throw std::runtime_error(std::format("Could not parse block '{}'", block));
      if (str[stack.back()] != '{') throw std::runtime_error(std::format("Could not parse block '{}'", block));
      stack.pop_back();
    }

    if (str[i] == ')') {
      if (stack.empty()) throw std::runtime_error(std::format("Could not parse block '{}'", block));
      if (str[stack.back()] != '(') throw std::runtime_error(std::format("Could not parse block '{}'", block));
      stack.pop_back();
    }
  }

  const auto last_part = utils::string::trim(str.substr(str_start));
  if (!last_part.empty()) {
    arr[counter] = last_part;
    counter += 1;
  }

  return counter;
}

std::tuple<std::string_view, std::string_view, std::string_view> parse_token(const std::string_view& token, const std::string_view* expr_tokens, const size_t max_size) {
  //constexpr std::string_view check_local_expr[] = { ">=", "<=", "==", "?=", "=" };
  //constexpr size_t check_local_expr_size = sizeof(check_local_expr) / sizeof(check_local_expr[0]);

  if (const size_t bracket_index = token.find('{'); bracket_index != std::string_view::npos) {
    const auto token_part = token.substr(0, bracket_index);
    const auto token_rvalue = token.substr(bracket_index);

    const auto [raw_lvalue, raw_remainder] = utils::string::substr_split_alt(token_part, expr_tokens, max_size);
    const auto lvalue = utils::string::trim(raw_lvalue);
    //const auto rvalue = utils::string::trim(raw_rvalue);
    //const auto remainder = utils::string::trim(token_part.substr(lvalue.size()));
    const auto remainder = utils::string::trim(raw_remainder);
    if (!lvalue.empty() && remainder != "?=" && remainder != "=") throw std::runtime_error(std::format("Invalid brackets element '{}'", token));
    //if (!rvalue.empty()) throw std::runtime_error(std::format("Invalid brackets element '{}'", token));
    return std::make_tuple(lvalue, remainder, utils::string::trim(token_rvalue));
  }

  const auto [raw_lvalue, raw_remainter] = utils::string::substr_split_alt(token, expr_tokens, max_size);
  const auto [divider, raw_rvalue] = utils::string::substr_split_alt(utils::string::trim(raw_remainter), expr_tokens, max_size);
  if (divider != "?=" && divider != "=") return std::make_tuple(std::string_view(), std::string_view(), utils::string::trim(token));
  return std::make_tuple(utils::string::trim(raw_lvalue), utils::string::trim(divider), utils::string::trim(raw_rvalue));
}

std::tuple<std::string_view, std::string_view, std::string_view, std::string_view> parse_token(const std::string_view &block, std::vector<size_t> &stack) {
  constexpr std::string_view dividers[] = { ",", "{", "(" };
  constexpr size_t dividers_size = sizeof(dividers) / sizeof(dividers[0]);

  constexpr std::string_view check_local_expr[] = { ">=", "<=", "==", "?=", "=" };
  constexpr size_t check_local_expr_size = sizeof(check_local_expr) / sizeof(check_local_expr[0]);

  const auto [token, strpart] = utils::string::substr_split_alt(block, dividers, dividers_size);
  if (token == "{") {
    const auto newblock = utils::string::inside2(utils::string::trim(block), "{", "}", stack);
    if (!stack.empty()) throw std::runtime_error(std::format("Could not parse script block '{}'", block));
    const size_t newblock_start = block.find(newblock);
    const auto found_block = block.substr(newblock_start-1, newblock.size()+2);
    if (!is_block(found_block)) throw std::runtime_error(std::format("Is it '{}' block?", found_block));

    const auto left_part = utils::string::trim(block.substr(0, block.find("{")));
    const auto [lvalue_raw, raw_remainter] = utils::string::substr_split_alt(left_part, check_local_expr, check_local_expr_size);
    const auto [expr_divider, rvalue_raw] = utils::string::substr_split_alt(utils::string::trim(raw_remainter), check_local_expr, check_local_expr_size);
    auto lvalue = utils::string::trim(lvalue_raw);
    auto rvalue = utils::string::trim(rvalue_raw);
    if (!expr_divider.empty() && expr_divider != "=" && expr_divider != "?=") throw std::runtime_error(std::format("Could not parse left value of script token '{}'", block));
    if (!lvalue.empty() && !is_valid_lvalue(lvalue)) throw std::runtime_error(std::format("Could not parse left value of script token '{}'", block));
    if (lvalue.empty() != expr_divider.empty()) throw std::runtime_error(std::format("Could not parse left value of script token '{}'", block));
    if (!rvalue.empty()) throw std::runtime_error(std::format("Could not parse left value of script token '{}'", block));

    auto remainder = utils::string::trim(block.substr(block.find(newblock)+newblock.size()+1));
    const size_t next_token_index = remainder.find(",");
    if (!remainder.empty() && next_token_index == std::string_view::npos) throw std::runtime_error(std::format("Could not parse block remainder '{}'", remainder));

    if (next_token_index != std::string_view::npos) remainder = remainder.substr(next_token_index+1);
    return std::make_tuple(lvalue, expr_divider, utils::string::trim(found_block), utils::string::trim(remainder));
  }

  const auto [lvalue_raw, raw_remainder] = utils::string::substr_split_alt(token, check_local_expr, check_local_expr_size);
  const auto [expr_divider, rvalue_raw] = utils::string::substr_split_alt(utils::string::trim(raw_remainder), check_local_expr, check_local_expr_size);
  if (expr_divider != "=" && expr_divider != "?=") return std::make_tuple(std::string_view(), std::string_view(), token, utils::string::trim(strpart));

  auto lvalue = utils::string::trim(lvalue_raw);
  auto rvalue = utils::string::trim(rvalue_raw);
  if (!is_valid_lvalue(lvalue)) throw std::runtime_error(std::format("Could not parse block '{}'", token));
  if (rvalue.empty()) throw std::runtime_error(std::format("Could not parse block '{}'", token));

  return std::make_tuple(lvalue, expr_divider, rvalue, utils::string::trim(strpart));
}

std::tuple<std::string_view, std::string_view, std::string_view> find_key(const std::string_view& script, const std::string_view &str, std::vector<size_t>& stack) {
  if (!is_block(script)) throw std::runtime_error(std::format("The string '{}' is not a valid script block", script));

  auto curstr = remove_brackets(script);
  while (!curstr.empty()) {
    const auto [lvalue, expr_divider, rvalue, remainder] = parse_token(curstr, stack);
    curstr = remainder;

    if (lvalue == str) return std::make_tuple(lvalue, expr_divider, rvalue);
  }

  return std::make_tuple(std::string_view(), std::string_view(), std::string_view());
}

std::tuple<std::string_view, std::string_view, std::string_view> find_index(const std::string_view& script, const size_t index, std::vector<size_t>& stack) {
  if (!is_block(script)) throw std::runtime_error(std::format("The string '{}' is not a valid script block", script));

  size_t counter = 0;
  auto curstr = remove_brackets(script);
  while (!curstr.empty()) {
    const auto [lvalue, expr_divider, rvalue, remainder] = parse_token(curstr, stack);
    curstr = remainder;
    if (index == counter) return std::make_tuple(lvalue, expr_divider, rvalue);
    counter += 1;
  }

  return std::make_tuple(std::string_view(), std::string_view(), std::string_view());
}

std::string_view find_index_ignore_lvalue(const std::string_view& script, const size_t index, std::vector<size_t>& stack) {
  if (!is_block(script)) throw std::runtime_error(std::format("The string '{}' is not a valid script block", script));

  size_t counter = 0;
  auto curstr = remove_brackets(script);
  while (!curstr.empty()) {
    const auto [lvalue, expr_divider, rvalue, remainder] = parse_token(curstr, stack);
    curstr = remainder;

    if (!lvalue.empty()) continue;
    if (index == counter) return rvalue;
    counter += 1;
  }

  return std::string_view();
}

size_t count_block(const std::string_view& script, std::vector<size_t>& stack) {
  if (!is_block(script)) throw std::runtime_error(std::format("The string '{}' is not a valid script block", script));

  auto curstr = remove_brackets(script);
  size_t counter = 0;
  while (!curstr.empty()) {
    const auto [lvalue, expr_divider, rvalue, remainder] = parse_token(curstr, stack);
    curstr = remainder;
    ++counter;
  }

  return counter;
}

void traverse(const std::string_view& script, std::vector<size_t>& stack) {
  if (!is_block(script)) {
    std::cout << script << "\n";
    return;
  }

  auto curstr = remove_brackets(script);
  while (!curstr.empty()) {
    const auto [lvalue, expr_divider, rvalue, remainder] = parse_token(curstr, stack);
    curstr = remainder;

    if (!is_block(rvalue)) {
      if (!lvalue.empty()) std::cout << lvalue << " " << expr_divider << " " << rvalue << "\n";
      else std::cout << rvalue << "\n";
    } else {
      if (!lvalue.empty()) std::cout << lvalue << " " << expr_divider << " {" << "\n";
      else std::cout << "{" << "\n";
      traverse(rvalue, stack);
      std::cout << "}" << "\n";
    }
  }
}
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}