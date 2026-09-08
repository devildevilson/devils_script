// The 32-bit configuration, exercised end to end.
//
// With DEVILS_SCRIPT_32BIT=1 the language's integer is int32_t and its floating-point value is
// float. Nothing about the numeric contract changes - integers stay exact and wrap, `/` still
// leaves the integers - only the width does, so the same properties are checked at the narrower
// boundaries. Built and run as its own executable because the main suite pins 64-bit values.
#include "devils_script/system.h"

#include <cstdio>
#include <functional>
#include <limits>
#include <string>

namespace ds = devils_script;

namespace {

int failures = 0;

void check(const bool ok, const std::string& what) {
  if (ok) return;
  std::printf("FAILED: %s\n", what.c_str());
  ++failures;
}

int32_t counted() { return 3; }
float halff() { return 0.5f; }

struct bag { int values[3]; };
bag the_bag{ { 2, 3, 4 } };
bag* a_bag() { return &the_bag; }
int64_t bag_first(const bag* b) { return b->values[0]; }

// Registered with the C++ types a consumer would naturally write. Both the callback result and the
// iterator result have to land on the language's own widths, not on `double`.
double each_value(bag* b, const std::function<double(int64_t)>& fn) {
  double sum = 0.0;
  for (const int v : b->values) sum += fn(v);
  return sum;
}

ds::system make() {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&counted>("counted");
  sys.register_function<&halff>("half");
  sys.register_function<&a_bag>("bag");
  sys.register_function<&bag_first>("first");
  sys.register_function_iter<&each_value>("each_value", { "value" });
  return sys;
}

template <typename R>
void expect(const ds::system& sys, const char* source, const R expected) {
  try {
    const auto script = sys.parse<R, void>("script", source);
    ds::context ctx;
    ctx.create_lists(&script);
    script.process(&ctx);
    const auto got = ctx.get_return<R>();
    if (got == expected) return;
    std::printf("FAILED: [%s] -> %s, expected %s\n", source,
                std::to_string(got).c_str(), std::to_string(expected).c_str());
    ++failures;
  } catch (const std::exception& e) {
    std::printf("FAILED: [%s] threw: %s\n", source, e.what());
    ++failures;
  }
}

void expect_throws(const ds::system& sys, const char* source) {
  try {
    const auto script = sys.parse<int32_t, void>("script", source);
    ds::context ctx;
    script.process(&ctx);
  } catch (const std::exception&) { return; }
  std::printf("FAILED: [%s] was accepted\n", source);
  ++failures;
}

}  // namespace

int main() {
  check(sizeof(ds::script_int_t) == 4, "script_int_t is 32 bits");
  check(sizeof(ds::script_float_t) == 4, "script_float_t is 32 bits");
  check(ds::type_is_integral(ds::utils::type_name<ds::script_int_t>()), "the narrow integer is the integral type");
  check(ds::type_is_floating_point(ds::utils::type_name<ds::script_float_t>()), "the narrow float is the floating type");

  // A C++ int64_t or double registered by a consumer still lands on the 32-bit stack types.
  check(std::is_same_v<ds::final_stack_el_t<int64_t>, int32_t>, "int64_t maps to the script integer");
  check(std::is_same_v<ds::final_stack_el_t<double>, float>, "double maps to the script float");

  const auto sys = make();
  constexpr int32_t min32 = std::numeric_limits<int32_t>::min();

  expect<int32_t>(sys, "2 + 3", 5);
  expect<int32_t>(sys, "counted * 7", 21);
  expect<int32_t>(sys, "2147483647 + 1", min32);          // wraps at the 32-bit boundary
  expect<int32_t>(sys, "-2147483647 - 1", min32);
  expect<int32_t>(sys, "counted + 2147483647", min32 + 2);
  expect<int32_t>(sys, "7 % 3", 1);
  expect<int32_t>(sys, "-7 % 3", -1);
  expect<int32_t>(sys, "(-2147483647 - 1) % -1", 0);
  expect_throws(sys, "counted % 0");

  // Integers past float precision stay exact: 16777217 is the first value a float cannot hold.
  expect<bool>(sys, "16777216 == 16777217", false);
  expect<bool>(sys, "16777217 > 16777216", true);
  expect<int32_t>(sys, "16777216 + 1", 16777217);

  // Division leaves the integers here too, and coming back is explicit.
  expect<float>(sys, "7 / 2", 3.5f);
  expect<float>(sys, "counted / 2", 1.5f);
  expect<int32_t>(sys, "to_int = { 7 / 2 }", 3);
  expect_throws(sys, "7 / 2");

  // Mixed operands convert the integer side, and comparisons do not narrow the float side.
  expect<float>(sys, "1.5 + 1", 2.5f);
  expect<float>(sys, "half + 1", 1.5f);
  expect<float>(sys, "2 * 3.5", 7.0f);
  expect<bool>(sys, "1.5 > 1", true);
  expect<bool>(sys, "1 == 1.0", true);

  // The rest of the language runs on the narrow types unchanged.
  expect<float>(sys, "max(3.0, 5.0)", 5.0f);
  expect<float>(sys, "sqrt(9.0)", 3.0f);
  expect<int32_t>(sys, "select = { { condition = false, 10 }, { condition = true, 20 }, { 100 } }", 20);
  expect<float>(sys, "{ ctx_save = { n = 5 }, ctx:saved:n + 1.0 }", 6.0f);
  expect<float>(sys, "{ ctx:list:xs = { add_to = 2.0, add_to = 3.0 }, ctx:list:xs = { sum = this } }", 5.0f);
  expect<float>(sys, "{ ctx_set = { a = 2.5 }, ctx:arg:a + 1 }", 3.5f);

  // Iterators: a `std::function<double(int64_t)>` callback compiles against the script float, and a
  // registered int64_t getter against the script integer.
  expect<int32_t>(sys, "bag = { first }", 2);
  expect<float>(sys, "bag = { each_value = { value = this * 2 } }", 18.0f);
  expect<float>(sys, "bag = { each_value = { value = this + 0.5 } }", 10.5f);

  if (failures != 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("32-bit configuration: all checks passed\n");
  return 0;
}
