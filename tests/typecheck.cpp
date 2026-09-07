#include "test_helpers.h"

namespace {
using kind = ds::system::user_function_type;
static_assert(ds::system::get_user_function_type<void(*)()>() == kind::effect);
static_assert(ds::system::get_user_function_type<bool(*)()>() == kind::condition);
static_assert(ds::system::get_user_function_type<double(*)()>() == kind::arithmetic);
static_assert(ds::system::get_user_function_type<std::string_view(*)()>() == kind::string);
static_assert(ds::system::get_user_function_type<int*(*)()>() == kind::object);
static_assert(ds::system::get_user_function_type<void(*)(), void, true>() == kind::iterator_effect);
static_assert(ds::system::get_user_function_type<void(*)(), bool, true>() == kind::iterator_condition);
static_assert(ds::system::get_user_function_type<void(*)(), double, true>() == kind::iterator_arithmetic);
}

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

TEST_CASE("Arithmetic type registry and overload resolution") {
  SUBCASE("integer literals prefer int64 overload and floating literals prefer double overload") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&numeric_overload_i64>("pick");
    sys.register_function<&numeric_overload_double>("pick");

    {
      const auto cont = sys.parse<int64_t, void>("script", "pick = { 7 }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<int64_t>());
      CHECK(ctx.get_return<int64_t>() == 10);
    }

    {
      const auto cont = sys.parse<int64_t, void>("script", "pick = { 7.5 }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<int64_t>());
      CHECK(ctx.get_return<int64_t>() == 20);
    }
  }

  SUBCASE("custom arithmetic type uses explicit implicit conversion chain") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_arithmetic_type<vec4>("ADD", 30);
    sys.register_implicit_conversion<double, vec4>();
    sys.register_function<&numeric_overload_i64>("pick");
    sys.register_function<&numeric_overload_double>("pick");
    sys.register_function<&numeric_overload_vec4, void>("pick");
    sys.register_function<&vec4_x, void>("vec4_x");
    sys.register_function<&vec4_passthrough, void>("as_vec4");

    {
      const auto cont = sys.parse<int64_t, void>("script", "pick = { as_vec4 = { 2 } }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<int64_t>());
      CHECK(ctx.get_return<int64_t>() == 30);
    }

    {
      const auto cont = sys.parse<double, void>("script", "vec4_x = { 9 }");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 9.0);
    }
  }

  SUBCASE("custom arithmetic type does not convert back without a registered rule") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_arithmetic_type<vec4>("ADD", 30);
    sys.register_implicit_conversion<double, vec4>();
    sys.register_function<&make_vec4, void>("make_vec4");
    sys.register_function<&double_passthrough>("as_double");

    CHECK_THROWS(sys.parse<double, void>("script", "as_double = { make_vec4 = { 3 } }"));
  }
}

TEST_CASE("Arithmetic operators for registered numeric-like types") {
  // This test documents the intended workflow for arithmetic extensions:
  // 1. Register a stack value type as arithmetic, so parse<type>() uses arithmetic block semantics.
  // 2. Register implicit conversions separately; type registration alone never creates casts.
  // 3. Register every operator overload the script language is allowed to use.
  //    For custom value types whose first C++ argument is not a scope, pass HT=void explicitly.
  const ds::system::operator_props mul_props{ 12, ds::system::command_data::math_ftype::binary, ds::system::command_data::associativity::left };
  const ds::system::operator_props add_props{ 11, ds::system::command_data::math_ftype::binary, ds::system::command_data::associativity::left };

  SUBCASE("unary plus and minus support literals, calls and standard precedence") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&i64_a>("a");
    sys.register_function<&i64_b>("b");
    sys.register_function<&i64_identity>("fn1");

    const std::pair<std::string_view, int64_t> cases[] = {
      { "-1",             -1 },
      { "+1",              1 },
      { "fn1 = +2",        2 },
      { "fn1 = -2",       -2 },
      { "fn1 = + 2",       2 },
      { "fn1 = - 2",      -2 },
      { "fn1(-1)",        -1 },
      { "fn1(+1)",         1 },
      { "-2 * 3",         -6 },
      { "-2 * 3 + 10",     4 },
      { "+2 * 3 + 10",    16 },
      { "-a + +b",       -80 },
    };

    for (const auto& [script, expected] : cases) {
      CAPTURE(script);
      const auto cont = sys.parse<int64_t, void>("script", script);
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<int64_t>());
      CHECK(ctx.get_return<int64_t>() == expected);
    }
  }

  SUBCASE("same-type expression uses that type's registered operators") {
    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<&i64_a>("a");
      sys.register_function<&i64_b>("b");
      sys.register_function<&i64_c>("c");
      sys.register_function<&i64_d>("d");
      sys.register_function<&i64_e>("e");

      const auto cont = sys.parse<int64_t, void>("script", "a + b * c - d / e");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<int64_t>());
      CHECK(ctx.get_return<int64_t>() == 152);
    }

    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<&double_a>("a");
      sys.register_function<&double_b>("b");
      sys.register_function<&double_c>("c");
      sys.register_function<&double_d>("d");
      sys.register_function<&double_e>("e");

      const auto cont = sys.parse<double, void>("script", "a + b * c - d / e");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 152.0);
    }

    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_arithmetic_type<vec4>("ADD", 30);
      sys.register_function<&vec4_a, void>("a");
      sys.register_function<&vec4_b, void>("b");
      sys.register_function<&vec4_c, void>("c");
      sys.register_function<&vec4_d, void>("d");
      sys.register_function<&vec4_e, void>("e");
      sys.register_operator<&vec4_mul, void>("*", mul_props);
      sys.register_operator<&vec4_div, void>("/", mul_props);
      sys.register_operator<&vec4_add, void>("+", add_props);
      sys.register_operator<&vec4_sub, void>("-", add_props);
      sys.register_function<&vec4_add, void>("ADD");
      sys.register_function<&vec4_mul, void>("MUL");

      const auto cont = sys.parse<vec4, void>("script", "a + b * c - d / e");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<vec4>());
      CHECK(ctx.get_return<vec4>() == vec4(152.0));
    }

    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_arithmetic_type<vec2>("ADD", 30);
      sys.register_function<&vec2_a, void>("a");
      sys.register_function<&vec2_b, void>("b");
      sys.register_function<&vec2_c, void>("c");
      sys.register_function<&vec2_d, void>("d");
      sys.register_function<&vec2_e, void>("e");
      sys.register_operator<&vec2_mul, void>("*", mul_props);
      sys.register_operator<&vec2_div, void>("/", mul_props);
      sys.register_operator<&vec2_add, void>("+", add_props);
      sys.register_operator<&vec2_sub, void>("-", add_props);
      sys.register_function<&vec2_add, void>("ADD");
      sys.register_function<&vec2_mul, void>("MUL");

      const auto cont = sys.parse<vec2, void>("script", "a + b * c - d / e");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<vec2>());
      CHECK(ctx.get_return<vec2>() == vec2(152.0));
    }

    {
      ds::system sys;
      sys.init_basic_functions();
      sys.init_math();
      sys.register_arithmetic_type<custom_type>("ADD", 40);
      sys.register_function<&custom_a, void>("a");
      sys.register_function<&custom_b, void>("b");
      sys.register_function<&custom_c, void>("c");
      sys.register_function<&custom_d, void>("d");
      sys.register_function<&custom_e, void>("e");
      sys.register_operator<&custom_mul, void>("*", mul_props);
      sys.register_operator<&custom_div, void>("/", mul_props);
      sys.register_operator<&custom_add, void>("+", add_props);
      sys.register_operator<&custom_sub, void>("-", add_props);
      sys.register_function<&custom_add, void>("ADD");
      sys.register_function<&custom_mul, void>("MUL");

      const auto cont = sys.parse<custom_type, void>("script", "a + b * c - d / e");
      ds::context ctx;
      cont.process(&ctx);
      REQUIRE(ctx.is_return<custom_type>());
      CHECK(ctx.get_return<custom_type>() == custom_type(INT64_C(152)));
    }
  }

  SUBCASE("mixed scalar/vector expressions require mixed operator overloads") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_arithmetic_type<vec4>("ADD", 30);
    sys.register_implicit_conversion<double, vec4>();
    sys.register_function<&vec4_a, void>("a");
    sys.register_function<&double_b>("b");
    sys.register_function<&vec4_c, void>("c");

    // Register both operand orders for normal scalar-vector arithmetic. The resolver does not
    // invent commutativity: if scripts may write both `vec4 * double` and `double * vec4`, register
    // both signatures.
    sys.register_operator<&vec4_add_scalar, void>("+", add_props);
    sys.register_operator<&scalar_add_vec4, void>("+", add_props);
    sys.register_operator<&vec4_add, void>("+", add_props);
    sys.register_operator<&vec4_mul_scalar, void>("*", mul_props);
    sys.register_operator<&scalar_mul_vec4, void>("*", mul_props);
    sys.register_function<&vec4_add, void>("ADD");
    sys.register_function<&vec4_mul, void>("MUL");

    const auto cont = sys.parse<vec4, void>("script", "a + b * c");
    ds::context ctx;
    cont.process(&ctx);
    REQUIRE(ctx.is_return<vec4>());
    CHECK(ctx.get_return<vec4>() == vec4(160.0));
  }

  SUBCASE("type registration does not make unsupported mixed operators legal") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_arithmetic_type<vec4>("ADD", 30);
    sys.register_function<&vec4_a, void>("a");
    sys.register_function<&double_b>("b");
    sys.register_function<&vec4_c, void>("c");
    sys.register_operator<&vec4_add, void>("+", add_props);
    sys.register_operator<&vec4_mul, void>("*", mul_props);
    sys.register_function<&vec4_add, void>("ADD");
    sys.register_function<&vec4_mul, void>("MUL");

    CHECK_THROWS(sys.parse<vec4, void>("script", "a + b * c"));
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
