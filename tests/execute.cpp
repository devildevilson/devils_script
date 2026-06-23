#include <doctest/doctest.h>
#include <unordered_map>
#include <string>
#include "devils_script/system.h"

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE::DEVILS_SCRIPT_INNER_NAMESPACE;
#else
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE;
#endif

namespace {

template <typename T>
struct handle {
  T* ptr;
  size_t type;
  T& operator*() const { return *ptr; }
  bool valid() const { return ptr != nullptr; }
};

struct person {
  uint16_t age;
  int charisma;
  void add_charisma(int c) { charisma += c; }
};

static uint16_t person_age(handle<person> p) { return (*p).age; }
static handle<person> person_self(handle<person> p) { return p; }

// A distinct object type, used to test root-scope type mismatch between caller and sub-script.
struct city {
  int population;
};
static int city_pop(handle<city> c) { return (*c).population; }

// A registry of compiled sub-scripts wired into the system's script resolver.
struct registry {
  std::unordered_map<std::string, const ds::script_container*> table;
  void add(std::string name, const ds::container& c) { table.emplace(std::move(name), &c); }
  void install(ds::system& sys) {
    sys.set_script_resolver([this](std::string_view name) -> const ds::script_container* {
      const auto it = table.find(std::string(name));
      return it == table.end() ? nullptr : it->second;
    });
  }
};

}

TEST_CASE("execute: script-in-script calls") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&person_age>("age");
  sys.register_function<&person::add_charisma, handle<person>>("add_charisma");

  // Root-less sub-script: returns the sum of two named double arguments.
  const auto addup = sys.parse<double, void>("addup", "ctx:arg:base + ctx:arg:bonus");
  // Rooted sub-script: returns the person's age plus a named bonus.
  const auto agebonus = sys.parse<double, handle<person>>("agebonus", "age + ctx:arg:bonus");
  // Rooted effect sub-script: bumps the person's charisma by a named amount.
  const auto boost = sys.parse<void, handle<person>>("boost", "add_charisma = ctx:arg:amount");

  registry reg;
  reg.add("addup", addup);
  reg.add("agebonus", agebonus);
  reg.add("boost", boost);
  reg.install(sys);

  person p{ 40, 7 };
  handle<person> ph{ &p, 0 };

  SUBCASE("root-less call returns value") {
    const auto cont = sys.parse<double, void>("caller", "execute = { addup, base = 10, bonus = 5 }");
    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(15.0));
  }

  SUBCASE("rooted call passes current scope implicitly") {
    const auto cont = sys.parse<double, handle<person>>("caller", "execute = { agebonus, bonus = 2 }");
    ds::context ctx;
    ctx.clear();
    ctx.set_arg(0, ph);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(42.0));
  }

  SUBCASE("rooted effect mutates through the sub-script") {
    const auto cont = sys.parse<void, handle<person>>("caller", "execute = { boost, amount = 3 }");
    ds::context ctx;
    ctx.clear();
    ctx.set_arg(0, ph);
    cont.process(&ctx);
    CHECK(p.charisma == 10);
  }

  SUBCASE("execute result composes in a block") {
    const auto cont = sys.parse<double, void>("caller", "{ 100, execute = { addup, base = 1, bonus = 2 } }");
    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(103.0));
  }

  SUBCASE("two executes in one script keep frames isolated") {
    const auto cont = sys.parse<double, void>(
      "caller",
      "{ execute = { addup, base = 1, bonus = 2 }, execute = { addup, base = 10, bonus = 20 } }");
    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(33.0));
  }

  SUBCASE("nested execute stacks frame_base correctly") {
    // `nested` itself calls `addup`, so this exercises a sub-script running inside a sub-script
    // (two stacked frame_base offsets) without copying the parent stack.
    const auto nested = sys.parse<double, void>("nested", "execute = { addup, base = 100, bonus = ctx:arg:b }");
    registry reg2;
    reg2.add("addup", addup);
    reg2.add("nested", nested);
    reg2.install(sys);

    const auto cont = sys.parse<double, void>("caller", "execute = { nested, b = 5 }");
    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(105.0));
  }

  SUBCASE("sub using switch resolves packed stack indices under frame_base") {
    // `switch` emits cmpeq2 (packed two stack indices). Running it as a sub-script verifies those
    // absolute indices are offset by frame_base instead of relying on a zero-based parent stack.
    const auto switched = sys.parse<double, void>(
      "switched", "{ switch = { value = 2, { value = 1, 10 }, { value = 2, ctx:arg:bump } } }");
    registry reg2;
    reg2.add("switched", switched);
    reg2.install(sys);

    const auto cont = sys.parse<double, void>("caller", "execute = { switched, bump = 20 }");
    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(20.0));
  }
}

TEST_CASE("execute: no-scope, no-arg, and prev invariants") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&person_age>("age");
  sys.register_function<&person_self>("self");

  // A scope-less, argument-less sub-script: needs no environment at all.
  const auto const_script = sys.parse<double, void>("const_script", "5 + 5");
  // A rooted sub whose `prev` (inside a nested object scope) refers back to its OWN root scope.
  const auto prevsub = sys.parse<double, handle<person>>("prevsub", "self = { prev:age }");

  registry reg;
  reg.add("const_script", const_script);
  reg.add("prevsub", prevsub);
  reg.install(sys);

  person p{ 40, 7 };
  handle<person> ph{ &p, 0 };

  SUBCASE("no-arg root-less sub runs with no scope (braced form)") {
    const auto cont = sys.parse<double, void>("caller", "execute = { const_script }");
    ds::context ctx;
    ctx.clear();                 // bare context: no scope, no args
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(10.0));
  }

  SUBCASE("no-arg root-less sub runs with no scope (bare-name form)") {
    const auto cont = sys.parse<double, void>("caller", "execute = const_script");
    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(10.0));
  }

  SUBCASE("'this' in a scope-less script is a parse error") {
    CHECK_THROWS(sys.parse<double, void>("x", "this"));
  }

  SUBCASE("'prev' in a scope-less script is a parse error") {
    CHECK_THROWS(sys.parse<double, void>("x", "prev"));
  }

  SUBCASE("'prev' that exceeds the script's own scopes is a parse error (cannot reach a caller)") {
    // Only the root scope exists, so `prev` has nothing outer to walk to — it must fail at compile
    // time rather than ever reaching into a parent frame at runtime.
    CHECK_THROWS(sys.parse<double, handle<person>>("x", "prev:age"));
  }

  SUBCASE("sub-script 'prev' resolves its own frame under frame_base") {
    const auto cont = sys.parse<double, handle<person>>("caller", "execute = { prevsub }");
    ds::context ctx;
    ctx.clear();
    ctx.set_arg(0, ph);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(40.0));
  }
}

TEST_CASE("execute: heavy stack arithmetic under frame_base") {
  // `random` is the most stack-index-heavy builtin: it builds a running prefix-sum across slots
  // (sumsetstack), scales the roll into a slot (mulsetstack), compares (cmplesseqd2) and erases
  // temporaries high-index-first. Every one of those is an ABSOLUTE main-stack index, so running a
  // `random` sub-script proves they are rebased by frame_base instead of assuming a zero-based stack.
  // We make selection deterministic by putting all weight on the first branch (cumulative weight is
  // 1 there, so roll*total <= 1 always picks it) — no dependence on prng_state.
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&person_age>("age");

  // Root-less weighted random: always returns its first branch's value (a named arg).
  const auto randweighted = sys.parse<double, void>(
    "randweighted", "{ random = { { weight = 1, ctx:arg:hi }, { weight = 0, 999.0 } } }");
  // Rooted weighted random: always returns the person's age (read through the implicit root scope).
  // `age + 0.0` promotes the uint16 result to double so both branches share one type.
  const auto randrooted = sys.parse<double, handle<person>>(
    "randrooted", "{ random = { { weight = 1, age + 0.0 }, { weight = 0, 999.0 } } }");

  registry reg;
  reg.add("randweighted", randweighted);
  reg.add("randrooted", randrooted);
  reg.install(sys);

  person p{ 40, 7 };
  handle<person> ph{ &p, 0 };

  SUBCASE("random sub-script resolves prefix-sum/scale/compare/erase indices under frame_base") {
    // The leading 1000 sits on the parent stack, so the sub runs at frame_base = 1: every random
    // stack op must add that offset or it would read/clobber the parent's 1000.
    const auto cont = sys.parse<double, void>("caller", "{ 1000, execute = { randweighted, hi = 7 } }");
    ds::context ctx;
    ctx.clear();
    ctx.prng_state = 125;     // irrelevant to the outcome; set only to prove determinism
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(1007.0));
  }

  SUBCASE("rooted random sub-script reads root via pushthis under frame_base") {
    // frame_base here is 2 (the 1000, plus the implicitly-pushed root copy) — exercises pushthis and
    // the packed random ops stacked on the same offset.
    const auto cont = sys.parse<double, handle<person>>("caller", "{ 1000, execute = { randrooted } }");
    ds::context ctx;
    ctx.clear();
    ctx.set_arg(0, ph);
    ctx.prng_state = 555;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(1040.0));
  }

  SUBCASE("random sub-script result matches the same script run at top level") {
    // Differential check: identical body at frame_base 0 vs frame_base 1 must agree for any roll.
    const auto direct = sys.parse<double, void>(
      "direct", "{ random = { { weight = 1, 3 }, { weight = 2, 6 }, { weight = 3, 9 } } }");
    registry reg2;
    reg2.add("direct", direct);
    reg2.install(sys);
    const auto cont = sys.parse<double, void>("caller", "{ 0, execute = { direct } }");

    ds::context base;
    base.clear();
    base.prng_state = 7777;
    direct.process(&base);

    ds::context off;
    off.clear();
    off.prng_state = 7777;
    cont.process(&off);

    REQUIRE(base.is_return<double>());
    REQUIRE(off.is_return<double>());
    CHECK(off.get_return<double>() == doctest::Approx(base.get_return<double>()));
  }
}

TEST_CASE("execute: parse-time errors") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&person_age>("age");
  sys.register_function<&city_pop>("city_pop");

  const auto addup = sys.parse<double, void>("addup", "ctx:arg:base + ctx:arg:bonus");
  const auto agebonus = sys.parse<double, handle<person>>("agebonus", "age + ctx:arg:bonus");
  // A sub-script rooted on a DIFFERENT object type than the caller will provide.
  const auto citypop = sys.parse<double, handle<city>>("citypop", "city_pop");

  registry reg;
  reg.add("addup", addup);
  reg.add("agebonus", agebonus);
  reg.add("citypop", citypop);
  reg.install(sys);

  SUBCASE("unknown script") {
    CHECK_THROWS(sys.parse<double, void>("caller", "execute = { nope, x = 1 }"));
  }
  SUBCASE("missing argument") {
    CHECK_THROWS(sys.parse<double, void>("caller", "execute = { addup, base = 1 }"));
  }
  SUBCASE("unknown (extra) argument") {
    CHECK_THROWS(sys.parse<double, void>("caller", "execute = { addup, base = 1, bonus = 2, extra = 3 }"));
  }
  SUBCASE("rooted sub-script without an active scope") {
    CHECK_THROWS(sys.parse<double, void>("caller", "execute = { agebonus, bonus = 1 }"));
  }

  // --- argument-type checking: a sub-script that does not receive the right types must reject ---
  SUBCASE("non-convertible argument type (string given for a string-vs-double clash)") {
    // addup's args are doubles; a string literal is not numerically convertible -> parse error.
    CHECK_THROWS(sys.parse<double, void>("caller", "execute = { addup, base = \"oops\", bonus = 5 }"));
  }
  SUBCASE("numeric arguments still convert (int literal feeds a double arg)") {
    // The complement of the above: bool/int/double DO convert, so this must compile and run.
    const auto cont = sys.parse<double, void>("caller", "execute = { addup, base = 1, bonus = 2 }");
    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(3.0));
  }

  // --- root-scope matching: the sub's required root type must match what the caller provides ---
  SUBCASE("root scope type mismatch between caller and sub-script") {
    // Caller's current scope is handle<person>, but `citypop` is rooted on handle<city>.
    CHECK_THROWS(sys.parse<double, handle<person>>("caller", "execute = { citypop }"));
  }
  SUBCASE("matching root scope type compiles") {
    CHECK_NOTHROW(sys.parse<double, handle<city>>("caller", "execute = { citypop }"));
  }

  // --- return-value matching: the sub's return type must satisfy the caller's context ---
  SUBCASE("value-returning sub used in an effect (void) context") {
    CHECK_THROWS(sys.parse<void, void>("caller", "execute = { addup, base = 1, bonus = 2 }"));
  }
  SUBCASE("return type incompatible with the caller's expected object type") {
    // addup returns double; the caller's script must yield handle<person> -> incompatible.
    CHECK_THROWS(sys.parse<handle<person>, void>("caller", "execute = { addup, base = 1, bonus = 2 }"));
  }
}
