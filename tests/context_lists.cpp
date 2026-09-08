#include "test_helpers.h"

TEST_CASE("on_effect callbacks") {
  SUBCASE("callback receives function name return value and arguments") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&effect_sum, &on_effect_sum>("effect_sum");

    const auto cont = sys.parse<double, void>("script", "effect_sum = { 2, 3 }");
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

    const auto cont = sys.parse<void, void>("script", "effect_touch = 7");
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

    const auto cont = sys.parse<double, object_ref>("script", "effect_object_score = 5");
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
    const auto cont = sys.parse<double, void>("script", script1);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SUBCASE("ctx:arg") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", script2);
    ds::context ctx;
    const size_t first_index = cont.find_arg("first");
    ctx.set_arg(first_index, 5.0);
    const size_t second_index = cont.find_arg("second");
    ctx.set_arg(second_index, 5.0);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 10);
  }

  SUBCASE("ctx:arg runtime type checks") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    const auto cont = sys.parse<double, void>("script", "{ ctx:arg:first }");

    {
      ds::context ctx;
      CHECK_THROWS(cont.process(&ctx));
    }

    {
      ds::context ctx;
      ctx.set_arg(cont.find_arg("first"), std::string_view("wrong"));
      CHECK_THROWS(cont.process(&ctx));
    }

    {
      ds::context ctx;
      ctx.set_arg(cont.find_arg("first"), 9.0);
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 9.0);
    }
  }

  SUBCASE("ctx_save objects") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func1>("func1");
    sys.register_function<&func7>("func7");
    const auto cont = sys.parse<double, scope1>("script", script3);
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
    const auto cont = sys.parse<double, scope1>("script", script4);
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
    const auto cont = sys.parse<double, scope2>("script", script5);
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
    const auto cont = sys.parse<double, scope2>("script", script6);
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

    const auto cont = sys.parse<double, object_ref>("script", 
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

    const auto cont = sys.parse<object_ref, object_ref>("script", 
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

    CHECK_THROWS(sys.parse<object_ref, object_ref>("script", "{ ctx:list:children = { add_to = outer, first = child_is_even } }"));

    const auto cont = sys.parse<object_ref, object_ref>("script", 
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
      const auto cont = sys.parse<double, object_ref>("script", 
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
      CHECK_THROWS(sys.parse<double, object_ref>("script", "{ ctx:list:children = { min = child_id } }"));
      const auto cont = sys.parse<double, object_ref>("script", "{ ctx:list:children = { add_to = outer, clear, min = child_id, default = 42 } }");
      ds::context ctx;
      ctx.set_arg(cont.find_arg("root"), object_ref{ 1 });
      ctx.create_lists(&cont);
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 42.0);
    }

    {
      const auto cont = sys.parse<double, object_ref>("script", 
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
    const auto cont = sys.parse<double, void>("script", script7);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 7);

    const size_t first_index = cont.find_arg("first");
    REQUIRE(first_index < ds::context::script_arguments_size);
    // `7` is written as an integer, so that is what the argument slot holds.
    CHECK(ctx.get_arg<int64_t>(first_index) == 7);
  }

  SUBCASE("ctx_set_as object args") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func1>("func1");
    sys.register_function<&func7>("func7");
    const auto cont = sys.parse<double, scope1>("script", script8);
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
    CHECK_THROWS(sys.parse<double, void>("script", "{ ctx_set = { first = 7 }, ctx:arg:first = { func7 } }"));
  }
}

// clear() must restore a clean slate so one context can run a sequence of DIFFERENT
// scripts back-to-back: no leftover stack, return value, saved slots, or list storage
// must contaminate the next run (arguments/lists are re-supplied by the caller per run).
TEST_CASE("context::clear allows reusing one context across different scripts") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  SUBCASE("different arithmetic scripts, same context") {
    const auto a = sys.parse<double, void>("a", "5 + 5");    // 10
    const auto b = sys.parse<double, void>("b", "35 * 2");   // 70

    ds::context ctx;
    a.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 10.0);

    ctx.clear();
    b.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 70.0);

    // and back to the first: clear must work in both directions, not just once.
    ctx.clear();
    a.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 10.0);
  }

  SUBCASE("return type changes between runs") {
    const auto num = sys.parse<double, void>("num", "5 + 5");  // double 10
    const auto cmp = sys.parse<bool, void>("cmp", "5 > 3");    // bool true

    ds::context ctx;
    num.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 10.0);

    ctx.clear();
    cmp.process(&ctx);
    REQUIRE(ctx.is_return<bool>());           // latest return wins, not the stale double
    CHECK(ctx.is_return<double>() == false);
    CHECK(ctx.get_return<bool>() == true);
  }

  SUBCASE("arguments are re-supplied per run") {
    const auto a = sys.parse<double, void>("a", "{ ctx:arg:first, ctx:arg:second }");  // first + second
    const auto b = sys.parse<double, void>("b", "{ ctx:arg:x }");                       // x

    ds::context ctx;
    ctx.set_arg(a.find_arg("first"), 5.0);
    ctx.set_arg(a.find_arg("second"), 5.0);
    a.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 10.0);

    ctx.clear();
    ctx.set_arg(b.find_arg("x"), 42.0);
    b.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 42.0);
  }

  SUBCASE("saved/local values do not leak between scripts") {
    // each script writes its own saved slot before reading it; clear() between runs
    // must not let the first script's saved value affect the second.
    const auto a = sys.parse<double, void>("a",
      "{ ctx_save = { number = 5 }, ctx:saved:number, ctx:saved:number }");  // 10
    const auto b = sys.parse<double, void>("b",
      "{ ctx_save = { number = 3 }, ctx:saved:number }");                    // 3

    ds::context ctx;
    a.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 10.0);

    ctx.clear();
    b.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 3.0);
  }
}

// Repeatedly running a list-using script on one context must give the same result
// every time: create_lists() has to rebuild list storage so elements don't accumulate.
TEST_CASE("context list storage is rebuilt per run via create_lists") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&func7>("func7");
  sys.register_function_iter<&every_on_list>("every_on_list", { "value" });

  // adds the root scope twice, then sums func7 (==5) over the list -> 10.
  const auto cont = sys.parse<double, scope2>("script",
    "{ ctx:list:baby_list = { add_to = outer, add_to = outer, every_on_list = { value = { func7 } } } }");

  ds::context ctx;
  for (int run = 0; run < 3; ++run) {
    ctx.clear();
    ctx.create_lists(&cont);
    ctx.set_arg(cont.find_arg("root"), scope2{});
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 10.0);  // not 20/30: the list did not accumulate across runs
  }
}

// A context can be sized explicitly from a script's parse-time max_stack / max_saved (or a
// nesting-class bucket); the argument stack stays at the fixed script_arguments_size.
TEST_CASE("explicitly sized context") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  const auto cont = sys.parse<double, void>(
    "sized", "{ ctx_save = { number = 5 }, ctx:saved:number, ctx:saved:number }");  // 10, 1 saved slot

  SUBCASE("a context sized to the script's own peaks runs it") {
    REQUIRE(cont.max_saved >= 1);
    ds::context ctx(cont.max_stack, cont.max_saved);
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 10.0);
  }

  SUBCASE("an undersized operand stack is rejected up front by process") {
    ds::context ctx(1, cont.max_saved);
    std::string msg;
    try { cont.process(&ctx); } catch (const std::exception& e) { msg = e.what(); }
    CHECK(msg.find("operand stack too small") != std::string::npos);
  }

  SUBCASE("an undersized saved stack is rejected up front by process") {
    ds::context ctx(cont.max_stack, 0);
    std::string msg;
    try { cont.process(&ctx); } catch (const std::exception& e) { msg = e.what(); }
    CHECK(msg.find("saved-value stack too small") != std::string::npos);
  }
}

// Saved-value reads are runtime-checked (like ctx:arg): reading a slot that was declared at parse
// time but never written at runtime (a ctx_save inside an untaken branch) throws instead of pushing
// a stale/empty value.
TEST_CASE("ctx:saved runtime type check") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  SUBCASE("a conditionally-unwritten saved slot throws when read") {
    // value_or skips the untaken branch's effects: condition false -> the ctx_save branch is not
    // run, so the slot stays empty and reading ctx:saved:x throws instead of pushing a stale value.
    const auto cont = sys.parse<double, void>(
      "cond", "{ value_or = { false, { ctx_save = { x = 5 }, 1.0 }, 2.0 }, ctx:saved:x }");
    ds::context ctx;
    CHECK_THROWS(cont.process(&ctx));
  }

  SUBCASE("the same slot read after it is actually written succeeds") {
    // condition true -> the ctx_save branch runs (x = 5), so the later read is valid (1.0 + 5).
    const auto cont = sys.parse<double, void>(
      "cond", "{ value_or = { true, { ctx_save = { x = 5 }, 1.0 }, 2.0 }, ctx:saved:x }");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 6.0);
  }
}
