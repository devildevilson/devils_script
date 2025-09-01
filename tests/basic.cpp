#include <catch2/catch_test_macros.hpp>
#include "devils_script/system.h"

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE::DEVILS_SCRIPT_INNER_NAMESPACE;
#else 
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE;
#endif

static double g(const double v1, const double v2, const double v3) noexcept { return v1 + v2 + v3; }

TEST_CASE("Script basics", "[script]") {
  const std::string script1 = "5";
  const std::string script2 = "5 + 5";
  const std::string script3 = "35 * 2 + (-3) * (10 + 12) + (3 / 4) * max(5,6)";
  const std::string script4 = "{1,2,3,g(4,5,6),NAND={false, false},max={7,8,9}}";

  SECTION("script1") {
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

  SECTION("script2") {
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
  SECTION("script3") {
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
  SECTION("script4") {
    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<decltype(&g), &g>("g");
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

TEST_CASE("Script functions and functions call", "[functions]") {
  // (return type == first argument type == second argument type && arguments count == 2) is unlimited argument function
  const std::string script1 = "max(1,2)+max(1,2,3)";
  const std::string script2 = "f(1,2)+f(1,2,3)+f(1,2,3,4)+f(1,2,3,4,5)"; // function f
  const std::string script3 = "{f={1,2},f={1,2,3},f={1,2,3,4},f={1,2,3,4,5}}"; // eq with script2
  const std::string script4 = "true m true m true m true"; // operator m
  const std::string script5 = "{m={true,true,true,true}}";

  SECTION("script1") {
    ds::system sys;
    sys.init_math();
    sys.register_function<decltype(&f), &f>("ADD"); // it is required for arithmetic scripts
    const auto cont = sys.parse<double, void>(script1);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 5.0);
  }

  SECTION("script2") {
    using mf = ds::system::command_data::math_ftype;
    using at = ds::system::command_data::associativity;
    ds::system sys;
    sys.register_function<decltype(&f), &f>("ADD"); // it is required for arithmetic scripts
    sys.register_function<decltype(&f), &f>("f");
    sys.register_operator<decltype(&f), &f>("+", { 11, mf::binary, at::left });
    const auto cont = sys.parse<double, void>(script2);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 34.0);
  }

  SECTION("script3") {
    ds::system sys;
    sys.register_function<decltype(&f), &f>("ADD"); // it is required for arithmetic scripts
    sys.register_function<decltype(&f), &f>("f");
    const auto cont = sys.parse<double, void>(script3);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 34.0);
  }

  SECTION("script4") {
    using mf = ds::system::command_data::math_ftype;
    using at = ds::system::command_data::associativity;
    ds::system sys;
    sys.register_function<decltype(&m), &m>("AND"); // it is required for boolean scripts
    sys.register_operator<decltype(&m), &m>("m", { 3, mf::binary, at::left });
    const auto cont = sys.parse<bool, void>(script4);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SECTION("script5") {
    using mf = ds::system::command_data::math_ftype;
    using at = ds::system::command_data::associativity;
    ds::system sys;
    sys.register_function<decltype(&m), &m>("AND"); // it is required for boolean scripts
    sys.register_operator<decltype(&m), &m>("m", { 3, mf::binary, at::left });
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

TEST_CASE("Advanced example", "[advanced]") {
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

  sys.register_function<decltype(&func1), func1>("func1");
  sys.register_function<decltype(&func2), func2>("func2");
  sys.register_function<decltype(&func3), func3>("func3");
  sys.register_function<decltype(&func4), func4>("func4");
  sys.register_function<decltype(&func7), func7>("func7");
  sys.register_function<decltype(&to_scope3), to_scope3>("to_scope3");
  sys.register_function<decltype(&to_scope2), to_scope2>("to_scope2");
  sys.register_function<decltype(&func8), func8>("func8");
  sys.register_function<decltype(&func9), func9>("func9");

  SECTION("script1") {
    const auto cont = sys.parse<scope1, scope1>(script1);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SECTION("script2") {
    const auto cont = sys.parse<double, scope1>(script2);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 5.0);
  }

  SECTION("script3") {
    const auto cont = sys.parse<double, scope1>(script3);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 1.0);
  }

  SECTION("script4") {
    const auto cont = sys.parse<scope1, scope1>(script4);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SECTION("script5") {
    const auto cont = sys.parse<scope1, scope1>(script5);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SECTION("script6") {
    const auto cont = sys.parse<scope1, scope1>(script6);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SECTION("script7") {
    const auto cont = sys.parse<double, scope1>(script7);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10.0);
  }
}

// uses register_function_iter but probably can be added to register_function
static double func6(scope2, const std::function<double(scope3)>& fn) { return fn(scope3{}); }
static double func10(scope2, const std::function<double(scope2)>& fn1, const std::function<double(scope2)>& fn2) { return fn1(scope2{}) + fn2(scope2{}); }

TEST_CASE("Iterators example", "[iterators]") {
  const std::string script1 = "{ func6 = { value = 5 + 5 } }"; // returns 10
  const std::string script2 = "{ func6 = { value = { func2 = { 4,5 } } } }"; // returns 1
  const std::string script3 = "{ func10 = { value = { func7 * func7 }, count = { 1 + 1 } } }"; // returns 27

  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  sys.register_function<decltype(&func1), func1>("func1");
  sys.register_function<decltype(&func2), func2>("func2");
  sys.register_function<decltype(&func3), func3>("func3");
  sys.register_function<decltype(&func4), func4>("func4");
  sys.register_function<decltype(&func7), func7>("func7");
  sys.register_function<decltype(&to_scope3), to_scope3>("to_scope3");
  sys.register_function<decltype(&to_scope2), to_scope2>("to_scope2");
  sys.register_function<decltype(&func8), func8>("func8");
  sys.register_function<decltype(&func9), func9>("func9");

  sys.register_function_iter<decltype(&func6), func6>("func6", { "value" });
  sys.register_function_iter<decltype(&func10), func10>("func10", { "count", "value" });

  SECTION("script1") {
    const auto cont = sys.parse<double, scope2>(script1);
    ds::context ctx;
    ctx.set_arg(0, scope2{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10.0);
  }

  SECTION("script2") {
    const auto cont = sys.parse<double, scope2>(script2);
    ds::context ctx;
    ctx.set_arg(0, scope2{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 1.0);
  }

  SECTION("script3") {
    const auto cont = sys.parse<double, scope2>(script3);
    ds::context ctx;
    ctx.set_arg(0, scope2{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 27.0);
  }
}

TEST_CASE("Main lang statements", "[statements]") {
  const std::string script1 = "{ value_or = { false, 10, 20 }, value_or = { true, 10, 20 }, value_or(false, 10, 20) }"; // returns 50
  const std::string script2 = "{ 5.0 == 5.00000000001 }"; // returns true
  const std::string script3 = "{ select = { { condition = false, 10 }, { condition = true, 20 }, { 100 } } }"; // returns 20
  const std::string script4 = "{ sequence = { { condition = true, 5 }, { condition = true, 10 }, { condition = false, 15 } } }"; // returns 15
  //const std::string script5 = "{ switch = { value = this, { value = to_scope2, 5 }, { value = to_scope2 } } }";
  const std::string script6 = "chance < 0.5"; // (random value [0.0, 1.0] < 0.5) == random [true,false]
  // random select, much better then 'select = { { condition = chance < 0.5, ... }, ...'
  const std::string script7 = "{ random = { { weight = 1, 3 }, { weight = 2, 6 }, { weight = 3, 9 } } }";

  SECTION("value_or") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script1);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 50.0);
  }

  SECTION("equality") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<bool, void>(script2);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SECTION("select") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script3);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 20.0);
  }

  SECTION("sequence") {
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

  SECTION("chance") {
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

  SECTION("random") {
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
}

static double every_on_list(const ds::internal::thisctxlist &l, const std::function<double(scope2)>& fn) { 
  double val = 0.0;
  for (size_t i = 0; i < l.ctx->lists[l.idx].size(); ++i) {
    val += fn(l.ctx->lists[l.idx][i].get<scope2>());
  }
  return val;
}

TEST_CASE("Using arguments + save to context + lists", "[memory]") {
  const std::string script1 = "{ ctx_save = { number = 5 }, ctx:saved:number, ctx:saved:number }"; // returns 10
  const std::string script2 = "{ ctx:arg:first, ctx:arg:second }"; // returns 10
  const std::string script3 = "{ ctx_save = { obj = { func1 = abc } }, ctx:saved:obj = { func7 + func7 } }"; // returns 10 (save scope2 as obj)
  const std::string script4 = "{ this:func1:abc = { ctx_save_as = obj }, ctx:saved:obj = { func7 + func7 } }"; // returns 10 (save scope2 as obj)
  const std::string script5 = "{ ctx:list:baby_list = { add_to = outer, add_to = outer }, ctx:list:baby_list = { every_on_list = { value = { func7 } } } }"; // returns 10
  const std::string script6 = "{ ctx:list:baby_list = { add_to = outer, add_to = outer, every_on_list = { value = { func7 } } } }"; // returns 10 (same as above)

  SECTION("ctx_save numbers") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>(script1);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SECTION("ctx:arg") {
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

  SECTION("ctx_save objects") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<decltype(&func1), func1>("func1");
    sys.register_function<decltype(&func7), func7>("func7");
    const auto cont = sys.parse<double, scope1>(script3);
    ds::context ctx;
    const size_t root_index = cont.find_arg("root");
    ctx.set_arg(root_index, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SECTION("ctx_save_as objects") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<decltype(&func1), func1>("func1");
    sys.register_function<decltype(&func7), func7>("func7");
    const auto cont = sys.parse<double, scope1>(script4);
    ds::context ctx;
    const size_t root_index = cont.find_arg("root");
    ctx.set_arg(root_index, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SECTION("ctx:list objects") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<decltype(&func1), func1>("func1");
    sys.register_function<decltype(&func7), func7>("func7");
    sys.register_function_iter<decltype(&every_on_list), every_on_list>("every_on_list", { "value" });
    const auto cont = sys.parse<double, scope2>(script5);
    ds::context ctx;
    const size_t root_index = cont.find_arg("root");
    ctx.set_arg(root_index, scope2{}); // set root
    ctx.create_lists(&cont);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SECTION("ctx:list objects 2") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<decltype(&func1), func1>("func1");
    sys.register_function<decltype(&func7), func7>("func7");
    sys.register_function_iter<decltype(&every_on_list), every_on_list>("every_on_list", { "value" });
    const auto cont = sys.parse<double, scope2>(script6);
    ds::context ctx;
    const size_t root_index = cont.find_arg("root");
    ctx.set_arg(root_index, scope2{}); // set root
    ctx.create_lists(&cont);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }
}