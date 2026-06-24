#include "test_helpers.h"

TEST_CASE("Type checking and valid argument checks") {
  SUBCASE("parse-time type checks reject wrong contexts") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&func9>("func9");
    sys.register_function<&object_id_is>("object_id_is");
    sys.register_function<&wrong_object>("wrong_object");

    CHECK_THROWS(sys.parse<double, void>("script", "unknown_function"));
    CHECK_THROWS(sys.parse<double, scope2>("script", "func9 = { 1 }"));
    CHECK_THROWS(sys.parse<double, scope1>("script", "func9 = { 1, 2 }"));
    CHECK_THROWS(sys.parse<bool, object_ref>("script", "object_id_is = { red }"));
    CHECK_THROWS(sys.parse<object_ref, object_ref>("script", "wrong_object"));
  }

  SUBCASE("runtime valid checks reject invalid scopes") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_id_is>("object_id_is");

    const auto cont = sys.parse<bool, object_ref>("script", "object_id_is = { 1 }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 0 });
    CHECK_THROWS(cont.process(&ctx));
  }

  SUBCASE("root argument runtime type is checked") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_id_is>("object_id_is");

    const auto cont = sys.parse<bool, object_ref>("script", "object_id_is = { 1 }");
    ds::context ctx;
    ctx.set_arg(0, 1.0);
    CHECK_THROWS(cont.process(&ctx));
  }

  SUBCASE("default is_valid<HT> guards scoped functions") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&checked_score>("checked_score");

    const auto cont = sys.parse<double, checked_ref>("script", "checked_score");

    {
      ds::context ctx;
      ctx.set_arg(0, checked_ref{ 7 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 7.0);
    }

    {
      ds::context ctx;
      ctx.set_arg(0, checked_ref{ 0 });
      CHECK_THROWS(cont.process(&ctx));
    }
  }

  SUBCASE("custom is_valid<HT> guard is used when registering scoped functions") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&checked_score, checked_ref, &checked_ref_is_even>("checked_score");

    const auto cont = sys.parse<double, checked_ref>("script", "checked_score");

    {
      ds::context ctx;
      ctx.set_arg(0, checked_ref{ 4 });
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 4.0);
    }

    {
      ds::context ctx;
      ctx.set_arg(0, checked_ref{ 3 });
      CHECK_THROWS(cont.process(&ctx));
    }
  }

  SUBCASE("return values keep declared type") {
    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      const auto cont = sys.parse<std::string_view, void>("script", "red");
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
      const auto cont = sys.parse<object_ref, object_ref>("script", "liege");
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
      const auto cont = sys.parse<void, object_ref>("script", "object_effect");
      ds::context ctx;
      ctx.set_arg(0, object_ref{ 1 });
      CHECK_NOTHROW(cont.process(&ctx));
      CHECK(ctx.return_type().empty());
    }
  }
}
// Locks in the diagnostic (raise_error) branches: malformed scripts must be rejected
// at parse time rather than miscompiling.
TEST_CASE("error branches are rejected at parse") {
  SUBCASE("value_or with mismatched 2nd/3rd argument types") {
    CHECK_THROWS(parse_void_d("{ value_or = { false, 5, true } }"));
  }

  SUBCASE("select condition placement rules") {
    // non-last block missing 'condition'
    CHECK_THROWS(parse_void_d("{ select = { { 10 }, { condition = true, 20 }, { 30 } } }"));
    // last block must NOT carry 'condition'
    CHECK_THROWS(parse_void_d("{ select = { { condition = true, 10 }, { condition = true, 20 } } }"));
  }

  SUBCASE("sequence requires condition in every block") {
    CHECK_THROWS(parse_void_d("{ sequence = { { 5 }, { condition = true, 10 } } }"));
  }

  SUBCASE("switch structural errors") {
    CHECK_THROWS(parse_void_d("{ switch = { value = 1, { 10 } } }"));          // case without 'value'
    CHECK_THROWS(parse_void_d("{ switch = { value = 1 } }"));                  // no case blocks
    CHECK_THROWS(parse_void_d("{ switch = { { value = 1, 10 } } }"));          // no top-level 'value'
    CHECK_THROWS(parse_void_d("{ switch = { value = { 1, 2 }, { value = 1, 10 } } }")); // top 'value' with >1 expr
  }

  SUBCASE("random node without weight") {
    CHECK_THROWS(parse_void_d("{ random = { { 3 }, { weight = 2, 6 } } }"));
  }

  SUBCASE("loading an unsaved context value") {
    CHECK_THROWS(parse_void_d("{ ctx:saved:never_saved }"));
  }

  SUBCASE("reading a saved value at a mismatched type") {
    // 'b' is saved as bool; the script's double return type can't accept it -> parse error.
    CHECK_THROWS(parse_void_d("{ ctx_save = { b = true }, ctx:saved:b }"));
  }

  SUBCASE("'this' without a scope") {
    CHECK_THROWS(parse_void_d("{ this }"));
  }
}
