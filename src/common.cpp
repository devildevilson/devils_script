#include "devils_script/common.h"

#include <charconv>

namespace devils_script {

namespace {
unsigned integer_literal_base(const std::string_view text) noexcept {
  if (text.size() <= 2 || text[0] != '0') return 10;
  switch (text[1]) {
    case 'x': case 'X': return 16;
    case 'o': case 'O': return 8;
    case 'b': case 'B': return 2;
    default: return 10;
  }
}
}

int32_t nullable_guard_mode(const std::string_view expected_type) noexcept {
  if (type_is_bool(expected_type)) return 1;
  if (type_is_integral(expected_type)) return 3;
  if (type_is_fundamental(expected_type)) return 2;
  return 0;
}

integer_literal parse_integer_literal(const std::string_view text, script_int_t& value) noexcept {
  const unsigned base = integer_literal_base(text);
  const auto digits = base == 10 ? text : text.substr(2);
  if (digits.empty() || (base != 10 && digits.front() == '-')) return integer_literal::none;
  const auto last = digits.data() + digits.size();
  script_int_t parsed = 0;
  const auto [ptr, ec] = std::from_chars(digits.data(), last, parsed, int(base));
  if (ptr != last) return integer_literal::none;
  if (ec == std::errc::result_out_of_range) return integer_literal::out_of_range;
  if (ec != std::errc()) return integer_literal::none;
  value = parsed;
  return integer_literal::valid;
}

std::string integer_literal_out_of_range_message(const std::string_view text) {
  const auto range = std::format("integer literal '{}' is out of range for int{}", text, sizeof(script_int_t) * 8);
  if (integer_literal_base(text) != 10) return range;
  return std::format("{}; write '{}.0' for a floating-point value", range, text);
}

size_t compute_count1() { return 0; }

int64_t default_command_f(int64_t, context*, const script_container*) { return 0; }

void assert_msg_fn(context*, const script_container*, const std::string_view&, const size_t) {}

}
