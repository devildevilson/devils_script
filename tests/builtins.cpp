#include "test_helpers.h"

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
    const auto cont = sys.parse<double, scope2>("script", script1);
    ds::context ctx;
    ctx.set_arg(0, scope2{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10.0);
  }

  SUBCASE("script2") {
    const auto cont = sys.parse<double, scope2>("script", script2);
    ds::context ctx;
    ctx.set_arg(0, scope2{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 1.0);
  }

  SUBCASE("script3") {
    const auto cont = sys.parse<double, scope2>("script", script3);
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
      const auto cont = sys.parse<bool, object_ref>("script", "{ any_child = { value = { child_id >= 3 }, filter = child_is_even, count = child_count_one } }");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
    }

    {
      const auto cont = sys.parse<bool, object_ref>("script", "{ any_child = { value = { child_id >= 3 }, filter = child_is_even, count = child_count_two } }");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == false);
    }

    {
      const auto cont = sys.parse<bool, object_ref>("script", "{ any_child = { value = { child_id >= 4 } } }");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
    }

    {
      g_any_child_value_calls = 0;
      const auto cont = sys.parse<bool, object_ref>("script", "{ any_child = { value = counted_child_is_even, count = child_count_one } }");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
      CHECK(g_any_child_value_calls == 2);
    }
  }
}
// Exercises the `*_unsafe` opcode variants: with safety toggled off, the emitter
// selects the unsafe function pointers (sum_unsafe, mul_unsafe, andjump_unsafe,
// orjump_unsafe, condjump*_unsafe, cmpeq2_unsafe, sumsetstack_unsafe, ...).
// Results must match the safe-mode expectations.
TEST_CASE("unsafe-mode execution") {
  SUBCASE("ADD/NAND/max block (sum_unsafe + andjump_unsafe)") {
    ds::system sys;
    sys.toggle_safety();
    REQUIRE(sys.safety() == false);
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&g>("g");
    const auto cont = sys.parse<double, void>("script", "{1,2,3,g(4,5,6),NAND={false, false},max={7,8,9}}");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 31.0);
  }

  SUBCASE("arithmetic expression") {
    ds::system sys;
    sys.toggle_safety();
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", "35 * 2 + (-3) * (10 + 12) + (3 / 4) * max(5,6)");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 8.5);
  }

  SUBCASE("AND/OR short-circuit (andjump_unsafe + orjump_unsafe)") {
    ds::system sys;
    sys.toggle_safety();
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&runtime_true>("runtime_true");
    sys.register_function<&runtime_false>("runtime_false");
    sys.register_function<&counted_true>("counted_true");
    sys.register_function<&counted_false>("counted_false");

    {
      g_short_circuit_calls = 0;
      const auto cont = sys.parse<bool, void>("script", "{ runtime_false, counted_true }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == false);
      CHECK(g_short_circuit_calls == 0); // short-circuited: counted_true not called
    }

    {
      g_short_circuit_calls = 0;
      const auto cont = sys.parse<bool, void>("script", "{ OR = { runtime_true, counted_false } }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
      CHECK(g_short_circuit_calls == 0);
    }

    {
      g_short_circuit_calls = 0;
      const auto cont = sys.parse<bool, void>("script", "{ runtime_true, counted_true }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
      CHECK(g_short_circuit_calls == 1);
    }
  }

  SUBCASE("select + sequence (condjump_unsafe)") {
    ds::system sys;
    sys.toggle_safety();
    sys.init_basic_functions();
    sys.init_math();

    {
      const auto cont = sys.parse<double, void>("script", "{ select = { { condition = false, 10 }, { condition = true, 20 }, { 100 } } }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 20.0);
    }

    {
      const auto cont = sys.parse<double, void>("script", "{ sequence = { { condition = true, 5 }, { condition = true, 10 }, { condition = false, 15 } } }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 15.0);
    }
  }

  SUBCASE("switch (cmpeq2_unsafe)") {
    ds::system sys;
    sys.toggle_safety();
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", "{ switch = { value = 2, { value = 1, 10 }, { value = 2, 20 }, { value = 3, 30 } } }");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 20.0);
  }

  SUBCASE("random weights (sumsetstack_unsafe + mulsetstack_unsafe + cmplesseqd2_unsafe)") {
    ds::system sys;
    sys.toggle_safety();
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", "{ random = { { weight = 1, 3 }, { weight = 2, 6 }, { weight = 3, 9 } } }");
    ds::context ctx;
    ctx.prng_state = 125;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 6); // identical to the safe-mode "random" subcase
  }

  SUBCASE("equality (cmpeq2_unsafe via ==)") {
    ds::system sys;
    sys.toggle_safety();
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<bool, void>("script", "{ 5.0 == 5.00000000001 }");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    CHECK(ctx.get_return<bool>() == true);
  }
}

// The math/trig builtins registered by init_math() (sqrt, sin, clamp, ...) are not
// const-foldable (try_eval_const only knows the core arithmetic/boolean ops), so these
// scripts actually emit and run the registered functions.
TEST_CASE("math and trig builtins") {
  struct mcase { const char* script; double expected; };
  const mcase cases[] = {
    { "sqrt(16)",            4.0 },
    { "inversesqrt(16)",     0.25 },
    { "inv(4)",              0.25 },
    { "exp(0)",              1.0 },
    { "abs(-7)",             7.0 },
    { "min(3,5)",            3.0 },
    { "max(3,5)",            5.0 },
    { "ceil(2.3)",           3.0 },
    { "floor(2.7)",          2.0 },
    { "round(2.5)",          3.0 },
    { "trunc(2.9)",          2.0 },
    { "sign(-4)",           -1.0 },
    { "sign(3)",             1.0 },
    { "fract(2.25)",         0.25 },
    { "sin(0)",              0.0 },
    { "cos(0)",              1.0 },
    { "tan(0)",              0.0 },
    { "asin(0)",             0.0 },
    { "acos(1)",             0.0 },
    { "atan(0)",             0.0 },
    { "clamp(5,0,3)",        3.0 },  // clamp(t, lo, hi)
    { "step(5,10)",          1.0 },  // step(edge, x)
    { "smoothstep(0,10,5)",  0.5 },
    { "mix(0,10,0.5)",       5.0 },
    { "fma(2,3,4)",          10.0 },
    { "10 % 3",              1.0 },  // mod operator
  };

  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  for (const auto& c : cases) {
    CAPTURE(c.script);
    const auto cont = sys.parse<double, void>("script", c.script);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == doctest::Approx(c.expected));
  }
}
