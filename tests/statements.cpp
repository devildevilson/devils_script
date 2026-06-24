#include "test_helpers.h"

TEST_CASE("Enum literals") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_enum<title_rank>(&parse_title_rank);
  sys.register_function<&rank>("rank");
  sys.register_function<&rank_is_at_least>("rank_is_at_least");

  SUBCASE("enum callback resolves function arguments") {
    const auto cont = sys.parse<bool, object_ref>("script", "rank_is_at_least = { kingdom }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 3 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    CHECK(ctx.get_return<bool>() == true);
  }

  SUBCASE("enum return compares with enum literal") {
    const auto cont = sys.parse<bool, object_ref>("script", "rank >= kingdom");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 4 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    CHECK(ctx.get_return<bool>() == true);
  }

  SUBCASE("enum literals compare with numbers") {
    {
      const auto cont = sys.parse<bool, void>("script", "kingdom >= 3");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
    }

    {
      const auto cont = sys.parse<bool, void>("script", "2 < kingdom");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
    }
  }

  SUBCASE("function names have priority over enum literals") {
    sys.register_function<&kingdom>("kingdom");

    const auto cont = sys.parse<double, void>("script", "kingdom");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 30.0);
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
    const auto cont = sys.parse<double, void>("script", script1);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 50.0);
  }

  SUBCASE("equality") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<bool, void>("script", script2);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SUBCASE("select") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", script3);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 20.0);
  }

  SUBCASE("sequence") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", script4);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 15.0);
  }

  SUBCASE("chance") {
    { // context seed 1
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<bool, void>("script", script6);
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
      const auto cont = sys.parse<bool, void>("script", script6);
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
      const auto cont = sys.parse<bool, void>("script", script6);
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
    const auto cont = sys.parse<double, void>("script", script7);
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
      const auto cont = sys.parse<bool, void>("script", "{ runtime_false, counted_true }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == false);
      CHECK(g_short_circuit_calls == 0);
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

  SUBCASE("chance and random are deterministic for equal seeds") {
    {
      ds::system::options opts;
      opts.seed = 12345;
      ds::system sys(opts);
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<bool, void>("script", "chance < 0.5");

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
      const auto cont = sys.parse<double, void>("script", "{ random = { { weight = 1, 3 }, { weight = 2, 6 }, { weight = 3, 9 } } }");

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

    CHECK_THROWS(sys.parse<double, void>("script", "ADD = { 5 + AND = { true, 1 } + 10 }"));
    CHECK_THROWS(sys.parse<double, void>("script", "5 + AND = { true, 1 }"));
  }
}
TEST_CASE("switch statement") {
  SUBCASE("number values") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", "{ switch = { value = 2, { value = 1, 10 }, { value = 2, 20 }, { value = 3, 30 } } }");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 20.0);
  }

  SUBCASE("string values") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", "{ switch = { value = red, { value = blue, 10 }, { value = red, 20 } } }");
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
    const auto cont = sys.parse<double, object_ref>("script", "{ switch = { value = this, { value = liege, 10 }, { value = this, 20 } } }");
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
      CHECK_THROWS(sys.parse<double, void>("script", "{ switch = { value = 1, { value = red, 10 } } }"));
    }

    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<&wrong_object>("wrong_object");
      CHECK_THROWS(sys.parse<double, object_ref>("script", "{ switch = { value = this, { value = wrong_object, 10 } } }"));
    }
  }
}
TEST_CASE("Nullable scope operator") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&liege>("liege");
  sys.register_function<&object_ref_is_eleven>("object_ref_is_eleven");
  sys.register_function<&child_id>("child_id");
  sys.register_function<&first_child>("first_child");
  sys.register_function<&nemesis>("nemesis");
  sys.register_function<&object_ref_name>("object_ref_name");

  SUBCASE("?= returns false in condition blocks when scope is invalid") {
    const auto cont = sys.parse<bool, object_ref>("script", "liege ?= { object_ref_is_eleven }");

    {
      ds::context ctx;
      ctx.set_arg(0, object_ref{ -10 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == false);
    }

    {
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<bool>());
      CHECK(ctx.get_return<bool>() == true);
    }
  }

  SUBCASE("?= returns zero in numeric blocks when scope is invalid") {
    const auto cont = sys.parse<double, object_ref>("script", "liege ?= { child_id }");

    {
      ds::context ctx;
      ctx.set_arg(0, object_ref{ -10 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 0.0);
    }

    {
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 11.0);
    }
  }

  SUBCASE("?= skips invalid object branches") {
    const auto cont = sys.parse<object_ref, object_ref>("script", "{ liege ?= { first_child }, nemesis }");

    {
      ds::context ctx;
      ctx.set_arg(0, object_ref{ -10 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<object_ref>());
      CHECK(ctx.get_return<object_ref>().id == 990);
    }

    {
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<object_ref>());
      CHECK(ctx.get_return<object_ref>().id == 111);
    }
  }

  SUBCASE("?= skips invalid string branches") {
    const auto cont = sys.parse<std::string_view, object_ref>("script", "{ liege ?= { object_ref_name }, object_ref_name }");

    {
      ds::context ctx;
      ctx.set_arg(0, object_ref{ -10 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<std::string_view>());
      CHECK(ctx.get_return<std::string_view>() == "fallback");
    }

    {
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<std::string_view>());
      CHECK(ctx.get_return<std::string_view>() == "liege");
    }
  }
}
