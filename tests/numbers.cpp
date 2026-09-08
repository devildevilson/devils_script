// The numeric contract of the script language.
//
// A literal written without a fraction or an exponent is an integer and stays one: it is not read as
// a double because the surrounding block happens to be floating-point. Integer arithmetic is exact
// and wraps on overflow. The one operation that always leaves the integers behind is `/`.
#include <doctest/doctest.h>
#include "devils_script/system.h"

#include <limits>

namespace ds = devils_script;

namespace {

int64_t big() { return 9007199254740993LL; }
int64_t counted() { return 3; }
double half() { return 0.5; }

ds::system make() {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&big>("big");
  sys.register_function<&counted>("counted");
  sys.register_function<&half>("half");
  return sys;
}

template <typename R>
R eval(const ds::system& sys, const char* source) {
  const auto script = sys.parse<R, void>("script", source);
  ds::context ctx;
  script.process(&ctx);
  REQUIRE(ctx.is_return<R>());
  return ctx.get_return<R>();
}

}  // namespace

TEST_CASE("numbers: an integer literal stays an integer") {
  const auto sys = make();

  // Past 2^53 a double cannot tell these two apart. Before integer literals were honest, both the
  // constant folder and the emitted comparison read them as doubles and answered "equal".
  CHECK_FALSE(eval<bool>(sys, "9007199254740992 == 9007199254740993"));
  CHECK(eval<bool>(sys, "9007199254740993 > 9007199254740992"));
  CHECK(eval<bool>(sys, "9007199254740993 == big"));
  CHECK(eval<int64_t>(sys, "9007199254740992 + 1") == 9007199254740993LL);

  // The type follows the way the literal is written, not the block it sits in.
  const auto integral = sys.parse<int64_t, void>("script", "5");
  CHECK(integral.cmds.size() == 2);
  const auto in_double_block = sys.parse<double, void>("script", "2 + 3");
  ds::context ctx;
  in_double_block.process(&ctx);
  CHECK(ctx.get_return<double>() == 5.0);

  // An argument slot written from a literal takes the literal's type.
  const auto args = sys.parse<double, void>("script", "{ ctx_set = { n = 7 }, ctx:arg:n }");
  ds::context actx;
  args.process(&actx);
  CHECK(actx.get_arg<int64_t>(args.find_arg("n")) == 7);
}

TEST_CASE("numbers: integer arithmetic is exact and wraps") {
  const auto sys = make();
  constexpr int64_t max = std::numeric_limits<int64_t>::max();
  constexpr int64_t min = std::numeric_limits<int64_t>::min();

  CHECK(eval<int64_t>(sys, "2 * 3 + 10") == 16);
  CHECK(eval<int64_t>(sys, "-2 * 3") == -6);
  CHECK(eval<int64_t>(sys, "counted * 1000000000000") == 3000000000000LL);

  // Two's complement wrap, the same at compile time and at run time - the runtime opcodes go through
  // the unsigned type so the fold has a defined contract to reproduce.
  CHECK(eval<int64_t>(sys, "9223372036854775807 + 1") == min);
  CHECK(eval<int64_t>(sys, "-9223372036854775807 - 1") == min);
  CHECK(eval<int64_t>(sys, "counted * 9223372036854775807") == int64_t(uint64_t(3) * uint64_t(max)));

  // Remainder keeps C++ sign rules, and its two trapping cases are script errors, not crashes.
  CHECK(eval<int64_t>(sys, "7 % 3") == 1);
  CHECK(eval<int64_t>(sys, "-7 % 3") == -1);
  CHECK(eval<int64_t>(sys, "7 % -3") == 1);
  // INT64_MIN % -1 traps in hardware; the answer is 0 and it is produced, not crashed into.
  CHECK(eval<int64_t>(sys, "(-9223372036854775807 - 1) % -1") == 0);
  CHECK(eval<int64_t>(sys, "counted % -1") == 0);
  CHECK_THROWS(eval<int64_t>(sys, "counted % 0"));
}

TEST_CASE("numbers: division always leaves the integers") {
  const auto sys = make();

  // `/` has no integer overload at all, so both operands convert and the result is floating-point.
  CHECK(eval<double>(sys, "7 / 2") == 3.5);
  CHECK(eval<double>(sys, "10 / 4") == 2.5);
  CHECK(eval<double>(sys, "counted / 2") == 1.5);

  // Which means an integer-returning script cannot end in a division by accident.
  CHECK_THROWS(sys.parse<int64_t, void>("script", "7 / 2"));
  CHECK(eval<int64_t>(sys, "to_int = { 7 / 2 }") == 3);
  CHECK(eval<int64_t>(sys, "to_int = { -7 / 2 }") == -3);
  CHECK_THROWS(eval<int64_t>(sys, "to_int = { 1 / 0 }"));

  // Floating-point division keeps its own rules.
  CHECK(eval<double>(sys, "1.0 / 0") == std::numeric_limits<double>::infinity());
}

TEST_CASE("numbers: mixing integers and floating-point converts the integer side") {
  const auto sys = make();

  CHECK(eval<double>(sys, "1.0 + 5") == 6.0);
  CHECK(eval<double>(sys, "5 + 1.0") == 6.0);
  CHECK(eval<double>(sys, "2 * 3.5") == 7.0);
  CHECK(eval<double>(sys, "half + 1") == 1.5);
  CHECK(eval<double>(sys, "1 + half") == 1.5);

  // A comparison must not narrow the floating-point side to reach an integer overload: `1.5 > 1`
  // used to compile into `1 > 1`.
  CHECK(eval<bool>(sys, "1.5 > 1"));
  CHECK(eval<bool>(sys, "1 < 1.5"));
  CHECK(eval<bool>(sys, "half < 1"));
  CHECK_FALSE(eval<bool>(sys, "1.5 <= 1"));

  // Equality is compiled by hand and cannot convert its first operand after the fact, so mixed
  // comparisons get their own opcodes rather than failing to compile.
  CHECK(eval<bool>(sys, "1 == 1.0"));
  CHECK(eval<bool>(sys, "1.0 == 1"));
  CHECK(eval<bool>(sys, "half == 0.5"));
  CHECK(eval<bool>(sys, "1 != 1.5"));
  CHECK(eval<bool>(sys, "counted == 3.0"));
  CHECK(eval<bool>(sys, "3.0 == counted"));

  // A saved slot holding an integer reads fine where a floating-point value is expected.
  CHECK(eval<double>(sys, "{ ctx_save = { n = 5 }, ctx:saved:n + 1.0 }") == 6.0);
  CHECK(eval<int64_t>(sys, "{ ctx_save = { n = 5 }, ctx:saved:n + 1 }") == 6);
}

TEST_CASE("numbers: folded and unfolded arithmetic agree") {
  const auto sys = make();

  // `counted` and `half` are opaque to the folder, so wrapping a literal in them forces the same
  // expression down the ordinary codegen path. Both must produce the same value.
  const std::pair<const char*, const char*> pairs[] = {
    { "7 % 3",            "(counted + 4) % 3" },
    { "9223372036854775807 + 1", "9223372036854775807 + (counted - 2)" },
    { "2 * 3 + 10",       "(counted - 1) * 3 + 10" },
  };
  for (const auto& [folded, dynamic] : pairs) {
    CAPTURE(folded);
    CHECK(eval<int64_t>(sys, folded) == eval<int64_t>(sys, dynamic));
  }

  CHECK(eval<double>(sys, "7 / 2") == eval<double>(sys, "(counted + 4) / 2"));
  CHECK(eval<bool>(sys, "9007199254740992 == 9007199254740993") ==
        eval<bool>(sys, "9007199254740992 == big"));
}
