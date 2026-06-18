#include <doctest/doctest.h>
#include "devils_script/system.h"

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE::DEVILS_SCRIPT_INNER_NAMESPACE;
#else 
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE;
#endif

static double g(const double v1, const double v2, const double v3) noexcept { return v1 + v2 + v3; }

TEST_CASE("Script basics") {
  const std::string script1 = "5";
  const std::string script2 = "5 + 5";
  const std::string script3 = "35 * 2 + (-3) * (10 + 12) + (3 / 4) * max(5,6)";
  const std::string script4 = "{1,2,3,g(4,5,6),NAND={false, false},max={7,8,9}}";

  SUBCASE("script1") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    const auto cont = sys.parse<double, void>(script1);

    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);

    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 5.0);
  }

  SUBCASE("script2") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    const auto cont = sys.parse<double, void>(script2);

    ds::context ctx;
    ctx.clear();
    cont.process(&ctx);

    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10.0);
  }

  ds::container cont1;
  SUBCASE("script3") {
    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      cont1 = sys.parse<double, void>(script3);
    }

    {
      ds::context ctx;
      ctx.clear();
      cont1.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      REQUIRE(ctx.get_return<double>() == 8.5);
    }
  }

  ds::container cont2;
  SUBCASE("script4") {
    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<&g>("g");
      cont2 = sys.parse<double, void>(script4);
    }

    {
      ds::context ctx;
      ctx.clear();
      cont2.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      REQUIRE(ctx.get_return<double>() == 31.0);
    }
  }
}

// can be unlimited function
static double f(const double v1, const double v2) noexcept { return v1 + v2; }
static bool m(const bool v1, const bool v2) noexcept { return v1 && v2; }

TEST_CASE("Script functions and functions call") {
  // (return type == first argument type == second argument type && arguments count == 2) is unlimited argument function
  const std::string script1 = "max(1,2)+max(1,2,3)";
  const std::string script2 = "f(1,2)+f(1,2,3)+f(1,2,3,4)+f(1,2,3,4,5)"; // function f
  const std::string script3 = "{f={1,2},f={1,2,3},f={1,2,3,4},f={1,2,3,4,5}}"; // eq with script2
  const std::string script4 = "true m true m true m true"; // operator m
  const std::string script5 = "{m={true,true,true,true}}";

  SUBCASE("script1") {
    ds::system sys;
    sys.init_math();
    sys.register_function<&f>("ADD"); // it is required for arithmetic scripts
    const auto cont = sys.parse<double, void>(script1);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 5.0);
  }

  SUBCASE("script2") {
    using mf = ds::system::command_data::math_ftype;
    using at = ds::system::command_data::associativity;
    ds::system sys;
    sys.register_function<&f>("ADD"); // it is required for arithmetic scripts
    sys.register_function<&f>("f");
    sys.register_operator<&f>("+", { 11, mf::binary, at::left });
    const auto cont = sys.parse<double, void>(script2);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 34.0);
  }

  SUBCASE("script3") {
    ds::system sys;
    sys.register_function<&f>("ADD"); // it is required for arithmetic scripts
    sys.register_function<&f>("f");
    const auto cont = sys.parse<double, void>(script3);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 34.0);
  }

  SUBCASE("script4") {
    using mf = ds::system::command_data::math_ftype;
    using at = ds::system::command_data::associativity;
    ds::system sys;
    sys.register_function<&m>("AND"); // it is required for boolean scripts
    sys.register_operator<&m>("m", { 3, mf::binary, at::left });
    const auto cont = sys.parse<bool, void>(script4);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SUBCASE("script5") {
    using mf = ds::system::command_data::math_ftype;
    using at = ds::system::command_data::associativity;
    ds::system sys;
    sys.register_function<&m>("AND"); // it is required for boolean scripts
    sys.register_operator<&m>("m", { 3, mf::binary, at::left });
    const auto cont = sys.parse<bool, void>(script5);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }
}

struct scope1 { bool valid() const { return true; } };
struct scope2 { bool is_valid() const { return true; } };
struct scope3 { operator bool() const { return true; } };
static scope2 func1(scope1, std::string_view) { return scope2{}; }
static double func2(scope3, double, double) { return 1; }
static double func3(double a, double b, double c) { return a + b + c; }
static double func4(const std::string_view& str) { return 1; }
static std::string_view func5() { return "rvalue"; }
static double func7(scope2) { return 5; }
static scope3 to_scope3(scope2) { return scope3{}; }
static scope2 to_scope2(scope1) { return scope2{}; }
static scope3 func8(scope2, std::string_view) { return scope3{}; }
static double func9(scope1, double) { return 10; }

struct object_ref {
  int64_t id;
  bool valid() const { return id != 0; }
};

static object_ref liege(object_ref cur) { return object_ref{ cur.id + 10 }; }
static object_ref even_child(object_ref) { return object_ref{ 2 }; }
static object_ref first_child(object_ref cur) { return object_ref{ cur.id + 100 }; }
static object_ref nemesis(object_ref cur) { return object_ref{ cur.id + 1000 }; }
static bool is_married_to(object_ref cur, object_ref other) { return cur.id == 1 && other.id == 1111; }
static bool object_id_is(object_ref cur, int64_t id) { return cur.id == id; }
static bool object_arg_id_is(std::string_view, object_ref cur, int64_t id) { return cur.id == id; }
static scope2 wrong_object(object_ref) { return scope2{}; }
static void object_effect(object_ref) {}
static double runtime_num() { return 3.0; }
static int g_short_circuit_calls = 0;
static bool runtime_true() { return true; }
static bool runtime_false() { return false; }
static bool counted_true() { g_short_circuit_calls += 1; return true; }
static bool counted_false() { g_short_circuit_calls += 1; return false; }
static double runtime_five() { return 5.0; }

struct effect_stats {
  int calls = 0;
  std::string_view name;
  double ret = 0.0;
  double arg0 = 0.0;
  double arg1 = 0.0;
  int64_t scope_id = 0;
};

static double effect_sum(double a, double b) { return a + b; }
static void on_effect_sum(void* ptr, const std::string_view& name, const double& ret, const std::tuple<double, double>& args) {
  auto* stats = static_cast<effect_stats*>(ptr);
  stats->calls += 1;
  stats->name = name;
  stats->ret = ret;
  stats->arg0 = std::get<0>(args);
  stats->arg1 = std::get<1>(args);
}

static void effect_touch(double) {}
static void on_effect_touch(void* ptr, const std::string_view& name, const std::tuple<double>& args) {
  auto* stats = static_cast<effect_stats*>(ptr);
  stats->calls += 1;
  stats->name = name;
  stats->arg0 = std::get<0>(args);
}

static double effect_object_score(object_ref scope, double bonus) { return double(scope.id) + bonus; }
static void on_effect_object_score(void* ptr, const std::string_view& name, const double& ret, const std::tuple<object_ref, double>& args) {
  auto* stats = static_cast<effect_stats*>(ptr);
  stats->calls += 1;
  stats->name = name;
  stats->ret = ret;
  stats->scope_id = std::get<0>(args).id;
  stats->arg0 = std::get<1>(args);
}

TEST_CASE("on_effect callbacks") {
  SUBCASE("callback receives function name return value and arguments") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&effect_sum, &on_effect_sum>("effect_sum");

    const auto cont = sys.parse<double, void>("effect_sum = { 2, 3 }");
    effect_stats stats;
    ds::context ctx;
    ctx.userptr = &stats;
    cont.process(&ctx);

    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 5.0);
    CHECK(stats.calls == 1);
    CHECK(stats.name == "effect_sum");
    CHECK(stats.ret == 5.0);
    CHECK(stats.arg0 == 2.0);
    CHECK(stats.arg1 == 3.0);
  }

  SUBCASE("void function callback receives function name and arguments") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&effect_touch, &on_effect_touch>("effect_touch");

    const auto cont = sys.parse<void, void>("effect_touch = 7");
    effect_stats stats;
    ds::context ctx;
    ctx.userptr = &stats;
    cont.process(&ctx);

    CHECK(ctx.return_type().empty());
    CHECK(stats.calls == 1);
    CHECK(stats.name == "effect_touch");
    CHECK(stats.arg0 == 7.0);
  }

  SUBCASE("scoped callback receives scope and script arguments") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&effect_object_score, object_ref, &on_effect_object_score>("effect_object_score");

    const auto cont = sys.parse<double, object_ref>("effect_object_score = 5");
    effect_stats stats;
    ds::context ctx;
    ctx.userptr = &stats;
    ctx.set_arg(0, object_ref{ 11 });
    cont.process(&ctx);

    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 16.0);
    CHECK(stats.calls == 1);
    CHECK(stats.name == "effect_object_score");
    CHECK(stats.ret == 16.0);
    CHECK(stats.scope_id == 11);
    CHECK(stats.arg0 == 5.0);
  }
}

TEST_CASE("Advanced example") {
  const std::string script1 = "this"; // returns this
  const std::string script2 = "this:func1:abc = { func7 }";  // returns 5
  const std::string script3 = "to_scope2 = { to_scope3 = { func2 = { 4,5 } } }"; // returns 1
  const std::string script4 = "to_scope2 = { to_scope3 = { prev = { prev } } }"; // returns this
  const std::string script5 = "to_scope2.to_scope3.prev.prev"; // returns this
  const std::string script6 = "to_scope2:func8:abc = { { prev.prev } }"; // returns this
  const std::string script7 = "to_scope2:func8:abc.prev.prev = { func9 = { 1 } }"; // returns 10

  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  sys.register_function<&func1>("func1");
  sys.register_function<&func2>("func2");
  sys.register_function<&func3>("func3");
  sys.register_function<&func4>("func4");
  sys.register_function<&func7>("func7");
  sys.register_function<&to_scope3>("to_scope3");
  sys.register_function<&to_scope2>("to_scope2");
  sys.register_function<&func8>("func8");
  sys.register_function<&func9>("func9");

  SUBCASE("script1") {
    const auto cont = sys.parse<scope1, scope1>(script1);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SUBCASE("script2") {
    const auto cont = sys.parse<double, scope1>(script2);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 5.0);
  }

  SUBCASE("script3") {
    const auto cont = sys.parse<double, scope1>(script3);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 1.0);
  }

  SUBCASE("script4") {
    const auto cont = sys.parse<scope1, scope1>(script4);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SUBCASE("script5") {
    const auto cont = sys.parse<scope1, scope1>(script5);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SUBCASE("script6") {
    const auto cont = sys.parse<scope1, scope1>(script6);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SUBCASE("script7") {
    const auto cont = sys.parse<double, scope1>(script7);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10.0);
  }
}

TEST_CASE("Object rvalue scripts") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&liege>("liege");
  sys.register_function<&first_child>("first_child");
  sys.register_function<&nemesis>("nemesis");
  sys.register_function<&is_married_to>("is_married_to");
  sys.register_function<&object_id_is>("object_id_is");
  sys.register_function<&object_arg_id_is>("object_arg_id_is");
  sys.register_function<&wrong_object>("wrong_object");

  SUBCASE("object chain can be used as rvalue argument") {
    const auto cont = sys.parse<bool, object_ref>("is_married_to = liege.first_child.nemesis");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SUBCASE("object block skips false conditioned subblock and returns fallback object") {
    const auto cont = sys.parse<object_ref, object_ref>("{ { condition = false, liege }, this }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<object_ref>());
    REQUIRE(ctx.get_return<object_ref>().id == 1);
  }

  SUBCASE("object block result can be consumed by object-scoped function") {
    const auto cont = sys.parse<bool, object_ref>("object_arg_id_is = { marker, { { condition = false, liege }, this }, 1 }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SUBCASE("wrong object type is rejected in object context") {
    CHECK_THROWS(sys.parse<object_ref, object_ref>("wrong_object"));
  }
}

// uses register_function_iter but probably can be added to register_function
static double func6(scope2, const std::function<double(scope3)>& fn) { return fn(scope3{}); }
static double func10(scope2, const std::function<double(scope2)>& fn1, const std::function<double(scope2)>& fn2) { return fn1(scope2{}) + fn2(scope2{}); }
static double child_id(object_ref child) { return double(child.id); }
static bool child_is_even(object_ref child) { return child.id % 2 == 0; }
static double child_count_one(object_ref) { return 1.0; }
static double child_count_two(object_ref) { return 2.0; }
static int g_any_child_value_calls = 0;
static bool counted_child_is_even(object_ref child) { g_any_child_value_calls += 1; return child.id % 2 == 0; }
static bool any_child(
  object_ref root,
  ds::script_function<bool(object_ref)> value,
  ds::script_function<bool(object_ref)> filter,
  ds::script_function<double(object_ref)> count
) {
  if (!root.valid() || !value) return false;
  const size_t required = count ? size_t(count(root)) : 1;
  size_t successes = 0;
  const object_ref children[] = { object_ref{ 1 }, object_ref{ 2 }, object_ref{ 3 }, object_ref{ 4 } };
  for (const auto child : children) {
    if (filter && !filter(child)) continue;
    successes += size_t(value(child));
    if (successes >= required) return true;
  }
  return false;
}

TEST_CASE("Iterators example") {
  const std::string script1 = "{ func6 = { value = 5 + 5 } }"; // returns 10
  const std::string script2 = "{ func6 = { value = { func2 = { 4,5 } } } }"; // returns 1
  const std::string script3 = "{ func10 = { value = { func7 * func7 }, count = { 1 + 1 } } }"; // returns 27

  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  sys.register_function<&func1>("func1");
  sys.register_function<&func2>("func2");
  sys.register_function<&func3>("func3");
  sys.register_function<&func4>("func4");
  sys.register_function<&func7>("func7");
  sys.register_function<&to_scope3>("to_scope3");
  sys.register_function<&to_scope2>("to_scope2");
  sys.register_function<&func8>("func8");
  sys.register_function<&func9>("func9");

  sys.register_function_iter<&func6>("func6", { "value" });
  sys.register_function_iter<&func10>("func10", { "count", "value" });

  SUBCASE("script1") {
    const auto cont = sys.parse<double, scope2>(script1);
    ds::context ctx;
    ctx.set_arg(0, scope2{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10.0);
  }

  SUBCASE("script2") {
    const auto cont = sys.parse<double, scope2>(script2);
    ds::context ctx;
    ctx.set_arg(0, scope2{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 1.0);
  }

  SUBCASE("script3") {
    const auto cont = sys.parse<double, scope2>(script3);
    ds::context ctx;
    ctx.set_arg(0, scope2{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 27.0);
  }

  SUBCASE("script_function callback type supports value filter and count") {
    static_assert(sizeof(ds::script_function<bool(object_ref)>) <= sizeof(std::function<bool(object_ref)>));

    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&child_id>("child_id");
    sys.register_function<&child_is_even>("child_is_even");
    sys.register_function<&child_count_one>("child_count_one");
    sys.register_function<&child_count_two>("child_count_two");
    sys.register_function<&counted_child_is_even>("counted_child_is_even");
    sys.register_function_iter<&any_child>("any_child", { "value", "filter", "count" });

    {
      const auto cont = sys.parse<bool, object_ref>("{ any_child = { value = { child_id >= 3 }, filter = child_is_even, count = child_count_one } }");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
    }

    {
      const auto cont = sys.parse<bool, object_ref>("{ any_child = { value = { child_id >= 3 }, filter = child_is_even, count = child_count_two } }");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == false);
    }

    {
      const auto cont = sys.parse<bool, object_ref>("{ any_child = { value = { child_id >= 4 } } }");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
    }

    {
      g_any_child_value_calls = 0;
      const auto cont = sys.parse<bool, object_ref>("{ any_child = { value = counted_child_is_even, count = child_count_one } }");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
      CHECK(g_any_child_value_calls == 2);
    }
  }
}

TEST_CASE("Main lang statements") {
  const std::string script1 = "{ value_or = { false, 10, 20 }, value_or = { true, 10, 20 }, value_or(false, 10, 20) }"; // returns 50
  const std::string script2 = "{ 5.0 == 5.00000000001 }"; // returns true
  const std::string script3 = "{ select = { { condition = false, 10 }, { condition = true, 20 }, { 100 } } }"; // returns 20
  const std::string script4 = "{ sequence = { { condition = true, 5 }, { condition = true, 10 }, { condition = false, 15 } } }"; // returns 15
  //const std::string script5 = "{ switch = { value = this, { value = to_scope2, 5 }, { value = to_scope2 } } }";
  const std::string script6 = "chance < 0.5"; // (random value [0.0, 1.0] < 0.5) == random [true,false]
  // random select, much better then 'select = { { condition = chance < 0.5, ... }, ...'
  const std::string script7 = "{ random = { { weight = 1, 3 }, { weight = 2, 6 }, { weight = 3, 9 } } }";

  SUBCASE("value_or") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script1);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 50.0);
  }

  SUBCASE("equality") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<bool, void>(script2);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SUBCASE("select") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script3);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 20.0);
  }

  SUBCASE("sequence") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script4);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 15.0);
  }

  // switch???

  SUBCASE("chance") {
    { // context seed 1
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<bool, void>(script6);
      ds::context ctx;
      ctx.prng_state = 1;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      REQUIRE(ctx.get_return<bool>() == true);
    }

    { // different context seed 352
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<bool, void>(script6);
      ds::context ctx;
      ctx.prng_state = 352;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      REQUIRE(ctx.get_return<bool>() == false);
    }

    { // seed from system
      ds::system::options o;
      o.seed = 36263;
      ds::system sys(o);
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<bool, void>(script6);
      ds::context ctx;
      ctx.prng_state = 1;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      REQUIRE(ctx.get_return<bool>() == false);
    }
  }

  SUBCASE("random") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script7);
    ds::context ctx;
    ctx.prng_state = 125;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 6);
  }

  SUBCASE("AND and OR short-circuit runtime calls") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&runtime_true>("runtime_true");
    sys.register_function<&runtime_false>("runtime_false");
    sys.register_function<&counted_true>("counted_true");
    sys.register_function<&counted_false>("counted_false");

    {
      g_short_circuit_calls = 0;
      const auto cont = sys.parse<bool, void>("{ runtime_false, counted_true }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == false);
      CHECK(g_short_circuit_calls == 0);
    }

    {
      g_short_circuit_calls = 0;
      const auto cont = sys.parse<bool, void>("{ OR = { runtime_true, counted_false } }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
      CHECK(g_short_circuit_calls == 0);
    }

    {
      g_short_circuit_calls = 0;
      const auto cont = sys.parse<bool, void>("{ runtime_true, counted_true }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
      CHECK(g_short_circuit_calls == 1);
    }
  }

  SUBCASE("chance and random are deterministic for equal seeds") {
    {
      ds::system::options opts;
      opts.seed = 12345;
      ds::system sys(opts);
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<bool, void>("chance < 0.5");

      ds::context ctx1;
      ctx1.prng_state = 777;
      cont.process(&ctx1);

      ds::context ctx2;
      ctx2.prng_state = 777;
      cont.process(&ctx2);

      REQUIRE(ctx1.is_return<bool>());
      REQUIRE(ctx2.is_return<bool>());
      CHECK(ctx1.get_return<bool>() == ctx2.get_return<bool>());
    }

    {
      ds::system::options opts;
      opts.seed = 12345;
      ds::system sys(opts);
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<double, void>("{ random = { { weight = 1, 3 }, { weight = 2, 6 }, { weight = 3, 9 } } }");

      ds::context ctx1;
      ctx1.prng_state = 555;
      cont.process(&ctx1);

      ds::context ctx2;
      ctx2.prng_state = 555;
      cont.process(&ctx2);

      REQUIRE(ctx1.is_return<double>());
      REQUIRE(ctx2.is_return<double>());
      CHECK(ctx1.get_return<double>() == ctx2.get_return<double>());
    }
  }

  SUBCASE("mixed block and expression syntax is rejected") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    CHECK_THROWS(sys.parse<double, void>("ADD = { 5 + AND = { true, 1 } + 10 }"));
    CHECK_THROWS(sys.parse<double, void>("5 + AND = { true, 1 }"));
  }
}

TEST_CASE("switch statement") {
  SUBCASE("number values") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("{ switch = { value = 2, { value = 1, 10 }, { value = 2, 20 }, { value = 3, 30 } } }");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 20.0);
  }

  SUBCASE("string values") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("{ switch = { value = red, { value = blue, 10 }, { value = red, 20 } } }");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 20.0);
  }

  SUBCASE("object values") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&liege>("liege");
    sys.register_function<&object_id_is>("object_id_is");
    const auto cont = sys.parse<double, object_ref>("{ switch = { value = this, { value = liege, 10 }, { value = this, 20 } } }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 20.0);
  }

  SUBCASE("case value type must match top value type") {
    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      CHECK_THROWS(sys.parse<double, void>("{ switch = { value = 1, { value = red, 10 } } }"));
    }

    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<&wrong_object>("wrong_object");
      CHECK_THROWS(sys.parse<double, object_ref>("{ switch = { value = this, { value = wrong_object, 10 } } }"));
    }
  }
}

TEST_CASE("Type checking and valid argument checks") {
  SUBCASE("parse-time type checks reject wrong contexts") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func9>("func9");
    sys.register_function<&object_id_is>("object_id_is");
    sys.register_function<&wrong_object>("wrong_object");

    CHECK_THROWS(sys.parse<double, void>("unknown_function"));
    CHECK_THROWS(sys.parse<double, scope2>("func9 = { 1 }"));
    CHECK_THROWS(sys.parse<double, scope1>("func9 = { 1, 2 }"));
    CHECK_THROWS(sys.parse<bool, object_ref>("object_id_is = { red }"));
    CHECK_THROWS(sys.parse<object_ref, object_ref>("wrong_object"));
  }

  SUBCASE("runtime valid checks reject invalid scopes") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_id_is>("object_id_is");

    const auto cont = sys.parse<bool, object_ref>("object_id_is = { 1 }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 0 });
    CHECK_THROWS(cont.process(&ctx));
  }

  SUBCASE("return values keep declared type") {
    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<std::string_view, void>("red");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<std::string_view>());
      CHECK(ctx.get_return<std::string_view>() == "red");
    }

    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<&liege>("liege");
      const auto cont = sys.parse<object_ref, object_ref>("liege");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<object_ref>());
      CHECK(ctx.get_return<object_ref>().id == 11);
    }

    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<&object_effect>("object_effect");
      const auto cont = sys.parse<void, object_ref>("object_effect");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      CHECK_NOTHROW(cont.process(&ctx));
      CHECK(ctx.return_type().empty());
    }
  }
}

TEST_CASE("Script description evaluation") {
  using kind = ds::container::description_node_kind;
  using state = ds::container::description_value_state;

  SUBCASE("folded value is visible") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    const auto cont = sys.parse<double, void>("5 + 5");
    ds::context ctx;
    bool found_value = false;

    cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      if (entry.kind == kind::block && entry.state == state::value && entry.value.is<double>()) {
        found_value = found_value || entry.value.get<double>() == 10.0;
      }
    });

    CHECK(found_value);
  }

  SUBCASE("invalid object scope does not stop description traversal") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_id_is>("object_id_is");

    const auto cont = sys.parse<bool, object_ref>("object_id_is = { 11 }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 0 });

    size_t nodes = 0;
    bool found_unavailable = false;
    CHECK_NOTHROW(cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      nodes += 1;
      found_unavailable = found_unavailable || entry.state == state::unavailable;
    }));

    CHECK(nodes > 0);
    CHECK(found_unavailable);
  }

  SUBCASE("iterator children are placeholders") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function_iter<&func6>("func6", { "value" });

    const auto cont = sys.parse<double, scope2>("{ func6 = { value = 5 + 5 } }");
    CHECK(cont.description_cmd_index_offsets.size() == cont.cmds.size() + 1);
    CHECK(cont.description_cmd_index_nodes.size() > 0);

    ds::context ctx;
    ctx.set_arg(0, scope2{});

    bool iterator_value = false;
    bool child_placeholder = false;
    cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      if (entry.name == "func6" && entry.kind == kind::iterator && entry.state == state::value && entry.value.is<double>())
        iterator_value = entry.value.get<double>() == 10.0;
      if (entry.kind == kind::argument && entry.state == state::placeholder)
        child_placeholder = true;
    });

    CHECK(iterator_value);
    CHECK(child_placeholder);
  }

  SUBCASE("node kinds cover common script forms") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_id_is>("object_id_is");
    sys.register_function<&runtime_num>("runtime_num");

    {
      const auto cont = sys.parse<double, void>("runtime_num + runtime_num");
      ds::context ctx;

      bool saw_function = false;
      bool saw_operator = false;
      cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
        saw_function = saw_function || (entry.name == "runtime_num" && entry.kind == kind::function && entry.state == state::value && entry.value.is<double>());
        saw_operator = saw_operator || (entry.name == "+" && entry.kind == kind::operator_t);
      });

      CHECK(saw_function);
      CHECK(saw_operator);
    }

    {
      const auto cont = sys.parse<std::string_view, void>("red");
      ds::context ctx;
      bool saw_literal = false;
      cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
        saw_literal = saw_literal || (entry.name == "red" && entry.kind == kind::literal && entry.state == state::value && entry.value.is<std::string_view>() && entry.value.get<std::string_view>() == "red");
      });
      CHECK(saw_literal);
    }
  }

  SUBCASE("effect nodes are described but not evaluated") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_effect>("object_effect");

    const auto cont = sys.parse<void, object_ref>("object_effect");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });

    bool saw_effect = false;
    CHECK_NOTHROW(cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      saw_effect = saw_effect || (entry.name == "object_effect" && entry.kind == kind::effect && entry.state == state::unavailable);
    }));
    CHECK(saw_effect);
  }
}

static double every_on_list(const ds::internal::thisctxlist &l, const std::function<double(scope2)>& fn) {
  double val = 0.0;
  for (size_t i = 0; i < l.ctx->lists[l.idx].size(); ++i) {
    val += fn(l.ctx->lists[l.idx][i].get<scope2>());
  }
  return val;
}

TEST_CASE("Using arguments + save to context + lists") {
  const std::string script1 = "{ ctx_save = { number = 5 }, ctx:saved:number, ctx:saved:number }"; // returns 10
  const std::string script2 = "{ ctx:arg:first, ctx:arg:second }"; // returns 10
  const std::string script3 = "{ ctx_save = { obj = { func1 = abc } }, ctx:saved:obj = { func7 + func7 } }"; // returns 10 (save scope2 as obj)
  const std::string script4 = "{ this:func1:abc = { ctx_save_as = obj }, ctx:saved:obj = { func7 + func7 } }"; // returns 10 (save scope2 as obj)
  const std::string script5 = "{ ctx:list:baby_list = { add_to = outer, add_to = outer }, ctx:list:baby_list = { every_on_list = { value = { func7 } } } }"; // returns 10
  const std::string script6 = "{ ctx:list:baby_list = { add_to = outer, add_to = outer, every_on_list = { value = { func7 } } } }"; // returns 10 (same as above)
  const std::string script7 = "{ ctx_set = { first = 7 }, ctx:arg:first }"; // writes arg rvalue and reads it back
  const std::string script8 = "{ this:func1:abc = { ctx_set_as = obj }, ctx:arg:obj = { func7 + func7 } }"; // writes arg lvalue/scope

  SUBCASE("ctx_save numbers") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script1);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SUBCASE("ctx:arg") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script2);
    ds::context ctx;
    const size_t first_index = cont.find_arg("first");
    ctx.set_arg(first_index, 5.0);
    const size_t second_index = cont.find_arg("second");
    ctx.set_arg(second_index, 5.0);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SUBCASE("ctx_save objects") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func1>("func1");
    sys.register_function<&func7>("func7");
    const auto cont = sys.parse<double, scope1>(script3);
    ds::context ctx;
    const size_t root_index = cont.find_arg("root");
    ctx.set_arg(root_index, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SUBCASE("ctx_save_as objects") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func1>("func1");
    sys.register_function<&func7>("func7");
    const auto cont = sys.parse<double, scope1>(script4);
    ds::context ctx;
    const size_t root_index = cont.find_arg("root");
    ctx.set_arg(root_index, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SUBCASE("ctx:list objects") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func1>("func1");
    sys.register_function<&func7>("func7");
    sys.register_function_iter<&every_on_list>("every_on_list", { "value" });
    const auto cont = sys.parse<double, scope2>(script5);
    ds::context ctx;
    const size_t root_index = cont.find_arg("root");
    ctx.set_arg(root_index, scope2{}); // set root
    ctx.create_lists(&cont);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SUBCASE("ctx:list objects 2") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func1>("func1");
    sys.register_function<&func7>("func7");
    sys.register_function_iter<&every_on_list>("every_on_list", { "value" });
    const auto cont = sys.parse<double, scope2>(script6);
    ds::context ctx;
    const size_t root_index = cont.find_arg("root");
    ctx.set_arg(root_index, scope2{}); // set root
    ctx.create_lists(&cont);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SUBCASE("ctx:list pipeline filter and count") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&liege>("liege");
    sys.register_function<&even_child>("even_child");
    sys.register_function<&child_is_even>("child_is_even");

    const auto cont = sys.parse<double, object_ref>(
      "{ ctx:list:children = { add_to = outer, add_to = outer.liege, add_to = outer.even_child }, ctx:list:children = { filter = child_is_even, count } }"
    );
    ds::context ctx;
    ctx.set_arg(cont.find_arg("root"), object_ref{ 1 });
    ctx.create_lists(&cont);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 1.0);
  }

  SUBCASE("ctx:list pipeline map and first with default") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&liege>("liege");
    sys.register_function<&even_child>("even_child");
    sys.register_function<&child_id>("child_id");

    const auto cont = sys.parse<object_ref, object_ref>(
      "{ ctx:list:children = { add_to = outer, add_to = outer.even_child, map = liege, first = { child_id >= 12 }, default = this } }"
    );
    ds::context ctx;
    ctx.set_arg(cont.find_arg("root"), object_ref{ 1 });
    ctx.create_lists(&cont);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<object_ref>());
    CHECK(ctx.get_return<object_ref>().id == 12);
  }

  SUBCASE("ctx:list pipeline first default is required and used") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&liege>("liege");
    sys.register_function<&child_is_even>("child_is_even");

    CHECK_THROWS(sys.parse<object_ref, object_ref>("{ ctx:list:children = { add_to = outer, first = child_is_even } }"));

    const auto cont = sys.parse<object_ref, object_ref>(
      "{ ctx:list:children = { add_to = outer, add_to = outer.liege, first = child_is_even, default = this } }"
    );
    ds::context ctx;
    ctx.set_arg(cont.find_arg("root"), object_ref{ 1 });
    ctx.create_lists(&cont);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<object_ref>());
    CHECK(ctx.get_return<object_ref>().id == 1);
  }

  SUBCASE("ctx:list pipeline numeric reducers with defaults") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&even_child>("even_child");
    sys.register_function<&child_id>("child_id");

    {
      const auto cont = sys.parse<double, object_ref>(
        "{ ctx:list:children = { add_to = outer, add_to = outer.even_child }, ctx:list:children = { sum = child_id } }"
      );
      ds::context ctx;
      ctx.set_arg(cont.find_arg("root"), object_ref{ 1 });
      ctx.create_lists(&cont);
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 3.0);
    }

    {
      CHECK_THROWS(sys.parse<double, object_ref>("{ ctx:list:children = { min = child_id } }"));
      const auto cont = sys.parse<double, object_ref>("{ ctx:list:children = { add_to = outer, clear, min = child_id, default = 42 } }");
      ds::context ctx;
      ctx.set_arg(cont.find_arg("root"), object_ref{ 1 });
      ctx.create_lists(&cont);
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 42.0);
    }

    {
      const auto cont = sys.parse<double, object_ref>(
        "{ ctx:list:children = { add_to = outer, add_to = outer.even_child }, ctx:list:children = { average = child_id, default = 0 } }"
      );
      ds::context ctx;
      ctx.set_arg(cont.find_arg("root"), object_ref{ 2 });
      ctx.create_lists(&cont);
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 2.0);
    }
  }

  SUBCASE("ctx_set args") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script7);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 7);

    const size_t first_index = cont.find_arg("first");
    REQUIRE(first_index < ds::context::script_arguments_size);
    CHECK(ctx.get_arg<double>(first_index) == 7);
  }

  SUBCASE("ctx_set_as object args") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func1>("func1");
    sys.register_function<&func7>("func7");
    const auto cont = sys.parse<double, scope1>(script8);
    ds::context ctx;
    ctx.set_arg(cont.find_arg("root"), scope1{});
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 10);
    CHECK(ctx.is_arg<scope2>(cont.find_arg("obj")));
  }

  SUBCASE("scalar args cannot be used as lvalue scope") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func7>("func7");
    CHECK_THROWS(sys.parse<double, void>("{ ctx_set = { first = 7 }, ctx:arg:first = { func7 } }"));
  }
}
