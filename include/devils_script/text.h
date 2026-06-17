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
bool has_special_symbols(const std::string_view& str) noexcept;
bool has_common_symbols(const std::string_view& str) noexcept;

bool is_in_ignore_list(const std::string_view& str) noexcept;
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}