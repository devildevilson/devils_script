#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <array>
#include <span>
#include <cctype>
#include <algorithm>
#include <locale>

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

namespace utils {
namespace string {

// returns sub strings count or SIZE_MAX is not enougth memory in arr
// if SIZE_MAX last str in arr is remaining part of the input
constexpr size_t split(const std::string_view &input, const std::string_view &token, std::span<std::string_view> &arr) {
  if (input.empty()) return 0;

  const size_t max_size = arr.empty() ? SIZE_MAX : arr.size();
  auto cur = input;
  auto prev = cur;
  size_t count = 0;

  do {
    const size_t pos = cur.find(token);
    const auto str = cur.substr(0, pos);
    prev = cur;
    //cur = (pos != std::string_view::npos) && (pos+token.size() <= cur.size()) ? cur.substr(pos+token.size()) : std::string_view();
    cur = pos != std::string_view::npos ? cur.substr(pos + token.size()) : std::string_view();
    if (!arr.empty()) arr[count] = str;
    count += 1;
  } while (!cur.empty() && count < max_size);

  count = count >= max_size && !cur.empty() ? SIZE_MAX : count;
  if (count == SIZE_MAX) arr[arr.size()-1] = prev;
  return count;
}

constexpr size_t split(const std::string_view& input, const std::string_view& token, std::string_view* arr, const size_t max_size) {
  auto ans = std::span(arr, max_size);
  return split(input, token, ans);
}

constexpr std::tuple<std::string_view, std::string_view> substr_split(const std::string_view &str, const std::string_view& substr) {
  const size_t index = str.find(substr);
  if (index == std::string_view::npos) return std::make_tuple(str, std::string_view());
  return std::make_tuple(str.substr(0, index), str.substr(index + substr.size()));
}

constexpr std::tuple<std::string_view, std::string_view> substr_split_alt(const std::string_view& str, const std::string_view* substrs, const size_t size) {
  size_t index = std::string_view::npos;
  std::string_view cursubstr;
  for (size_t i = 0; i < size; ++i) {
    const size_t pos = str.find(substrs[i]);
    if (pos < index) {
      index = pos;
      cursubstr = substrs[i];
    }
  }

  if (index == std::string_view::npos) return std::make_tuple(str, std::string_view());
  if (index == 0) return std::make_tuple(str.substr(0, cursubstr.size()), str.substr(cursubstr.size()));
  return std::make_tuple(str.substr(0, index), str.substr(index));
}

constexpr size_t split2(const std::string_view &input, const std::string_view &token, std::string_view *arr, const size_t max_arr) {
  size_t count = 0;
  size_t prev_pos = 0;
  size_t current_pos = 0;
  do {
    current_pos = input.find(token, prev_pos);

    const size_t substr_count = count + 1 == max_arr ? std::string_view::npos : current_pos - prev_pos;
    arr[count] = input.substr(prev_pos, substr_count);
    count += 1;
    prev_pos = current_pos + token.size();
  } while (current_pos < input.size() && count < max_arr);

  count = count == max_arr && current_pos != std::string_view::npos ? SIZE_MAX : count;

  return count;
}

constexpr std::string_view inside(const std::string_view &input, const std::string_view &right, const std::string_view &left) {
  const size_t start = input.find(right);
  const size_t end = input.rfind(left);
  if (start >= end) return std::string_view();
  if (end == std::string_view::npos) return std::string_view();

  return input.substr(start + right.size(), end - (start + right.size()));
}


constexpr std::string_view inside2(const std::string_view& input, const std::string_view& right, const std::string_view& left, std::vector<size_t> &stack) {
  size_t start = input.find(right);
  if (start == std::string_view::npos) return std::string_view();
  start += right.size();

  std::string_view ret;
  stack.push_back(start);
  while (start <= input.size() && !stack.empty()) {
    const size_t right_pos = input.find(right, start);
    const size_t left_pos = input.find(left, start);
    if (left_pos < right_pos) {
      start = left_pos == std::string_view::npos ? left_pos : left_pos+left.size();
      const size_t pos = stack.back();
      stack.pop_back();
      ret = input.substr(pos, left_pos - pos);
    } else {
      start = right_pos == std::string_view::npos ? right_pos : right_pos+right.size();
      stack.push_back(start);
    }
  }

  return ret;
}

constexpr std::string_view inside2(const std::string_view& input, const std::string_view& right, const std::string_view& left) {
  std::vector<size_t> stack;
  return inside2(input, right, left, stack);
}

constexpr bool is_whitespace(char c) {
  // Include your whitespaces here. The example contains the characters
  // documented by https://en.cppreference.com/w/cpp/string/wide/iswspace
  constexpr char matches[] = { ' ', '\n', '\r', '\f', '\v', '\t' };
  return std::any_of(std::begin(matches), std::end(matches), [c](char c0) { return c == c0; });
}

constexpr std::string_view trim(const std::string_view &input) {
  int64_t right = 0;
  int64_t left = input.size() - 1;

  for (; right < int64_t(input.size()) && is_whitespace(input[right]); ++right) {}
  for (; left >= right && is_whitespace(input[left]); --left) {}

  if (right > left) return std::string_view();
  return input.substr(right, left - right + 1);
}

constexpr bool is_digit(const char c) { return c <= '9' && c >= '0'; }
constexpr size_t stoi_impl(const std::string_view::iterator &beg, const std::string_view::iterator &end, size_t value = 0) {
  if (beg == end) return value;
  if (!is_digit(*beg)) return value;
  return stoi_impl(beg + 1, end, size_t(*beg - '0') + value * 10);
}

constexpr size_t stoi(const std::string_view &str) {
  return stoi_impl(str.begin(), str.end());
}

static_assert(stoi("10") == 10);
static_assert(stoi("346346363") == 346346363);

constexpr std::string_view slice(const std::string_view &input, const int64_t start = 0, const int64_t end = INT64_MAX) {
  const size_t fstart = start < 0 ? std::max(int64_t(input.size())+start, int64_t(0)) : start;
  const size_t fend = end < 0 ? std::max(int64_t(input.size())+end, int64_t(0)) : end;
  if (fstart >= fend) return std::string_view();
  return input.substr(fstart, fend-fstart);
}

template <typename T, size_t N = SIZE_MAX>
constexpr std::span<T, N> slice(const std::span<T, N> &input, const int64_t start = 0, const int64_t end = INT64_MAX) {
  const size_t fstart = start < 0 ? std::max(int64_t(input.size())+start, int64_t(0)) : std::min(size_t(start), input.size());
  const size_t fend = end < 0 ? std::max(int64_t(input.size())+end, int64_t(0)) : std::min(size_t(end), input.size());
  if (fstart >= fend) return std::span<T>();
  return std::span<T>(input.data()+fstart, fend - fstart);
}

template<typename charT>
struct locale_based_case_insensitive_equal {
  constexpr locale_based_case_insensitive_equal(const std::locale& loc) noexcept : loc_(loc) {}
  constexpr bool operator() (const charT ch1, const charT ch2) const noexcept {
    return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
  }
private:
  const std::locale& loc_;
};

// find substring (case insensitive)
template<typename T>
constexpr size_t find_ci(const T& str1, const T& str2, const std::locale& loc = std::locale()) {
  typename T::const_iterator it = std::search(str1.begin(), str1.end(), str2.begin(), str2.end(), locale_based_case_insensitive_equal<typename T::value_type>(loc));
  if (it != str1.end()) return it - str1.begin();
  return SIZE_MAX; // not found
}

constexpr size_t find_ci(const std::string_view& str1, const char* str2, const std::locale& loc = std::locale()) {
  return find_ci(str1, std::string_view(str2), loc);
}

constexpr bool parse_dice(const std::string_view& str, size_t& count, size_t& upper_bound) noexcept {
  std::array<std::string_view, 2> data;
  auto sp = std::span(data.data(), data.size());
  const size_t c = string::split(str, "d", sp);
  if (c == SIZE_MAX) return false;
  if (c == 1) {
    count = 1;
    upper_bound = string::stoi(data[0]);
    return true;
  }

  if (data[0] == "") {
    count = 1;
    upper_bound = string::stoi(data[1]);
    return true;
  }

  count = string::stoi(data[0]);
  upper_bound = string::stoi(data[1]);
  return true;
}

}
}
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}