#include "test_helpers.h"

TEST_CASE("Script basics") {
  const std::string script1 = "5";
  const std::string script2 = "5 + 5";
  const std::string script3 = "35 * 2 + (-3) * (10 + 12) + (3 / 4) * max(5,6)";
  const std::string script4 = "{1,2,3,g(4,5,6),NAND={false, false},max={7,8,9}}";

  SUBCASE("script1") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    const auto cont = sys.parse<double, void>("script", script1);

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

    const auto cont = sys.parse<double, void>("script", script2);

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
      cont1 = sys.parse<double, void>("script", script3);
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
      cont2 = sys.parse<double, void>("script", script4);
    }

    {
      ds::context ctx;
      ctx.clear();
      cont2.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      REQUIRE(ctx.get_return<double>() == 31.0);
    }
  }

  SUBCASE("script_container runtime-only usage") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    std::vector<ds::container> compiled;
    // Empty script name keeps the pool to just the referenced tokens for this compactness check.
    compiled.push_back(sys.parse<double, void>("", script2));
    compiled.push_back(sys.parse<double, void>("", script3));
    // `source` is now the compact, deduplicated token pool (not the raw script): it holds only
    // the strings actually referenced by the container, so it stays well under the source length.
    CHECK(compiled[0].string_pool.size() < script2.size());
    CHECK(compiled[1].string_pool.size() < script3.size());
    ds::shrink_to_fit(std::span<ds::container>(compiled));

    std::vector<ds::script_container> scripts;
    scripts.reserve(compiled.size());
    for (auto& cont : compiled) {
      REQUIRE(!cont.block_descs.empty());
      scripts.push_back(std::move(cont).strip_description());
    }
    ds::shrink_to_fit(std::span<ds::script_container>(scripts));

    {
      ds::context ctx;
      ctx.clear();
      scripts[0].process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 10.0);
    }

    {
      ds::context ctx;
      ctx.clear();
      scripts[1].process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 8.5);
    }
  }
}
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
    const auto cont = sys.parse<double, void>("script", script1);
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
    const auto cont = sys.parse<double, void>("script", script2);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 34.0);
  }

  SUBCASE("script3") {
    ds::system sys;
    sys.register_function<&f>("ADD"); // it is required for arithmetic scripts
    sys.register_function<&f>("f");
    const auto cont = sys.parse<double, void>("script", script3);
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
    const auto cont = sys.parse<bool, void>("script", script4);
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
    const auto cont = sys.parse<bool, void>("script", script5);
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
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
    const auto cont = sys.parse<scope1, scope1>("script", script1);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SUBCASE("script2") {
    const auto cont = sys.parse<double, scope1>("script", script2);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 5.0);
  }

  SUBCASE("script3") {
    const auto cont = sys.parse<double, scope1>("script", script3);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    REQUIRE(ctx.get_return<double>() == 1.0);
  }

  SUBCASE("script4") {
    const auto cont = sys.parse<scope1, scope1>("script", script4);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SUBCASE("script5") {
    const auto cont = sys.parse<scope1, scope1>("script", script5);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SUBCASE("script6") {
    const auto cont = sys.parse<scope1, scope1>("script", script6);
    ds::context ctx;
    ctx.set_arg(0, scope1{}); // set root
    cont.process(&ctx);
    REQUIRE(ctx.is_return<scope1>());
  }

  SUBCASE("script7") {
    const auto cont = sys.parse<double, scope1>("script", script7);
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
    const auto cont = sys.parse<bool, object_ref>("script", "is_married_to = liege.first_child.nemesis");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SUBCASE("object block skips false conditioned subblock and returns fallback object") {
    const auto cont = sys.parse<object_ref, object_ref>("script", "{ { condition = false, liege }, this }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<object_ref>());
    REQUIRE(ctx.get_return<object_ref>().id == 1);
  }

  SUBCASE("object block result can be consumed by object-scoped function") {
    const auto cont = sys.parse<bool, object_ref>("script", "object_arg_id_is = { marker, { { condition = false, liege }, this }, 1 }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });
    cont.process(&ctx);
    REQUIRE(ctx.is_return<bool>());
    REQUIRE(ctx.get_return<bool>() == true);
  }

  SUBCASE("wrong object type is rejected in object context") {
    CHECK_THROWS(sys.parse<object_ref, object_ref>("script", "wrong_object"));
  }
}
TEST_CASE("Scoped overload resolution") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&to_scope2>("to_scope2");
  sys.register_function<&overloaded_score>("overloaded");
  sys.register_function<&overloaded_score2>("overloaded");

  SUBCASE("same script name dispatches by root scope") {
    {
      const auto cont = sys.parse<double, scope1>("script", "overloaded");
      ds::context ctx;
      ctx.set_arg(0, scope1{});
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 1.0);
    }

    {
      const auto cont = sys.parse<double, scope2>("script", "overloaded");
      ds::context ctx;
      ctx.set_arg(0, scope2{});
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 2.0);
    }
  }

  SUBCASE("scope chain uses overload for current scope") {
    const auto cont = sys.parse<double, scope1>("script", "to_scope2.overloaded");
    ds::context ctx;
    ctx.set_arg(0, scope1{});
    cont.process(&ctx);
    REQUIRE(ctx.is_return<double>());
    CHECK(ctx.get_return<double>() == 2.0);
  }
}
