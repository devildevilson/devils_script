#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <string>
#include <vector>

#ifdef DEVILS_SCRIPT_USE_LUA
#include "sol/sol.hpp"
#endif

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

namespace text {
#ifdef DEVILS_SCRIPT_USE_LUA
std::string convert_lua(sol::object obj);
#endif

void remove_comment_blocks(std::string &script) noexcept;

bool is_bool(const std::string_view& str) noexcept;
bool as_bool(const std::string_view& str) noexcept;
bool is_number(const std::string_view& str, double& val) noexcept;
bool is_number(const std::string_view& str) noexcept;
double as_number(const std::string_view& str) noexcept;
bool is_block(const std::string_view& text) noexcept;
std::string_view remove_brackets(const std::string_view& block) noexcept;
bool is_string(const std::string_view& text) noexcept;

bool is_valid_lvalue(const std::string_view& str) noexcept;
bool is_valid_rvalue(const std::string_view& str) noexcept;
bool is_valid_function_name(const std::string_view& str) noexcept;
bool is_valid_operator_name(const std::string_view& str) noexcept;
bool is_special_operator(const std::string_view& str) noexcept;

bool is_in_ignore_list(const std::string_view& str) noexcept;

size_t split_tokens(const std::string_view& block, std::string_view* arr, const size_t max_size, std::vector<size_t>& stack);
std::tuple<std::string_view, std::string_view, std::string_view> parse_token(const std::string_view& token, const std::string_view* expr_tokens, const size_t max_size);
std::tuple<std::string_view, std::string_view, std::string_view, std::string_view> parse_token(const std::string_view& token, std::vector<size_t>& stack);
std::tuple<std::string_view, std::string_view, std::string_view> find_key(const std::string_view& script, const std::string_view& str, std::vector<size_t>& stack);
std::tuple<std::string_view, std::string_view, std::string_view> find_index(const std::string_view& script, const size_t index, std::vector<size_t>& stack);
std::string_view find_index_ignore_lvalue(const std::string_view& script, const size_t index, std::vector<size_t>& stack);
size_t count_block(const std::string_view& script, std::vector<size_t>& stack);
void traverse(const std::string_view& script, std::vector<size_t>& stack);
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}