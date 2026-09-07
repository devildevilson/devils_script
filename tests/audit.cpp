#include <doctest/doctest.h>
#include "devils_script/system.h"
#include <limits>
#include <cmath>
namespace ds = devils_script;
namespace {
int effects = 0;
void bump() { ++effects; }
int64_t integer() { return 9007199254740993LL; }
int fold_calls = 0;
double counting_add(const double a, const double b) { ++fold_calls; return a + b + 100; }
double dbl(const double x) { return x; }
bool truth(bool x) { return x; }
double choose_bool(bool) { return 1; }
double choose_double(double) { return 2; }
struct nontrivial_copy {
  int x;
  nontrivial_copy(const nontrivial_copy& rhs) : x(rhs.x) {}
};
struct no_default {
  int64_t x;
  no_default() = delete;
  explicit no_default(int64_t x) : x(x) {}
};
static_assert(!ds::valid_stack_type<nontrivial_copy>);
static_assert(ds::valid_stack_type<no_default>);
static_assert(!std::is_nothrow_default_constructible_v<ds::context>);
}

TEST_CASE("audit: byte storage round trips without a default constructor") {
  ds::context::stack_t stack(4);
  stack.push(no_default{123});
  stack.push(1.25);
  stack.erase(1);
  CHECK(stack.safe_get<no_default>().x == 123);
  ds::any_stack copy(stack.get_view());
  CHECK(copy.get<no_default>().x == 123);
  ds::stack_element el;
  el.set(no_default{456});
  CHECK(el.rawget<no_default>().x == 456);
  el.set(true);
  for (size_t i = sizeof(bool); i < sizeof(el.mem); ++i) CHECK(el.mem[i] == 0);
  CHECK(ds::element_view{} == ds::element_view{});
  CHECK_FALSE(ds::element_view{}.valid());
  CHECK(copy.view().rawget<ds::element_view>().type() == copy.type());
}

TEST_CASE("audit: conversion cost and generated path agree") {
  for (const auto safety : {ds::system::safety::safe, ds::system::safety::unsafe}) {
    ds::system sys;
    sys.init_basic_functions();
    if (sys.safety() != (safety == ds::system::safety::safe)) sys.toggle_safety();
    sys.register_implicit_conversion<int64_t, double>(1);
    sys.register_implicit_conversion<int64_t, bool>(10);
    sys.register_implicit_conversion<double, bool>(1);
    const auto i = ds::utils::type_name<int64_t>();
    const auto d = ds::utils::type_name<double>();
    const auto b = ds::utils::type_name<bool>();
    CHECK(sys.implicit_conversion_cost(i, b) == 2);
    sys.register_function<&integer>("integer");
    sys.register_function<&truth>("truth");
    ds::context ctx;
    auto script = sys.parse<bool, void>("conversion", "truth = integer");
    CHECK(std::count_if(script.cmds.begin(), script.cmds.end(), [](const auto& c) {
      return c.fp == &ds::convert<int64_t, double> || c.fp == &ds::convert_unsafe<int64_t, double>;
    }) == 1);
    script.process(&ctx);
    CHECK(ctx.get_return<bool>());
    sys.register_function<&choose_bool>("choose");
    sys.register_function<&choose_double>("choose");
    const auto chosen = sys.parse<double, void>("overload", "choose = integer");
    ctx.clear();
    chosen.process(&ctx);
    CHECK(ctx.get_return<double>() == 2.0);
    sys.register_implicit_conversion<double, bool>(0);
    sys.register_implicit_conversion<bool, double>(0);
    CHECK(sys.implicit_conversion_cost(i, b) == 1);
    CHECK(sys.implicit_conversion_cost(b, d) == 0);
    sys.register_implicit_conversion<int64_t, bool>(1);
    const auto tie = sys.parse<bool, void>("tie", "truth = integer");
    CHECK(std::count_if(tie.cmds.begin(), tie.cmds.end(), [](const auto& c) {
      return c.fp == &ds::convert<int64_t, bool> || c.fp == &ds::convert_unsafe<int64_t, bool>;
    }) == 1);
    // Re-registering an existing edge replaces its cost.
    sys.register_implicit_conversion<int64_t, bool>(0);
    CHECK(sys.implicit_conversion_cost(i, b) == 0);
    CHECK_THROWS(sys.register_implicit_conversion<int64_t, bool>(-1));
    CHECK(sys.implicit_conversion_cost(i, b) == 0);
  }
  ds::system overflow;
  overflow.register_implicit_conversion<int64_t, double>(INT32_MAX);
  overflow.register_implicit_conversion<double, bool>(1);
  CHECK_THROWS(overflow.implicit_conversion_cost(ds::utils::type_name<int64_t>(), ds::utils::type_name<bool>()));
  overflow.register_implicit_conversion<int64_t, bool>(2);
  CHECK(overflow.implicit_conversion_cost(ds::utils::type_name<int64_t>(), ds::utils::type_name<bool>()) == 2);
}

TEST_CASE("audit: describe never executes a subscript") {
  ds::system sys;
  sys.init_basic_functions(); sys.init_math();
  sys.register_function<&bump>("bump");
  const auto effect = sys.parse<void, void>("effect", "bump");
  sys.set_script_resolver([&](std::string_view) -> const ds::script_container* { return &effect; });
  const auto sub = sys.parse<double, void>("sub", "{ execute = { effect }, ctx_set = { v = ctx:arg:v + 1.0 }, ctx:list:nums = { add_to = 9.0 }, 7.0 }");
  sys.set_script_resolver([&](std::string_view name) -> const ds::script_container* { return name == "sub" ? &sub : &effect; });
  for (auto source : {
    "{ ctx_save = { x = 2.0 }, ctx:list:nums = { add_to = 1.0 }, execute = { sub, nums, v = ctx:saved:x } }",
    "{ execute = { effect }, 4.0 }",
    "{ ctx:list:xs = { add_to = 1.0 }, ctx:list:xs = { sum = { execute = { sub, v = 2.0 } } } }"
  }) {
    const auto caller = sys.parse<double, void>("caller", source);
    ds::context ctx;
    ctx.create_lists(&caller);
    ctx.set_saved(0, 33.0);
    effects = 0;
    bool unavailable = false;
    caller.describe(&ctx, [&](const ds::container::description_entry& e) {
      if (e.name == "execute") { CHECK(e.state == ds::container::description_value_state::unavailable); unavailable = true; }
    });
    CHECK(unavailable);
    CHECK(effects == 0);
    CHECK(ctx.get_saved<double>(0) == 33.0);
    for (const auto& list : ctx.lists) CHECK(list.empty());
    CHECK(ctx.current_script == nullptr);
    CHECK_FALSE(ctx.describing);
  }
}

TEST_CASE("audit: nested failure restores frames and in-out lists") {
  ds::system sys;
  sys.init_basic_functions(); sys.init_math();
  const auto sub = sys.parse<void, void>("sub", "{ ctx_set = { v = ctx:arg:v + 10.0 }, ctx:list:nums = { add_to = 9.0 }, assert = { false, broken } }");
  ds::container middle;
  sys.set_script_resolver([&](std::string_view name) -> const ds::script_container* { return name == "sub" ? &sub : &middle; });
  middle = sys.parse<void, void>("middle", "execute = { sub, nums, v = ctx:arg:v }");
  const auto caller = sys.parse<double, void>("caller", "{ ctx_save = { x = 2.0 }, ctx:list:nums = { add_to = 1.0 }, execute = { middle, nums, v = ctx:saved:x }, 4.0 }");
  ds::context ctx;
  ctx.create_lists(&caller);
  ctx.set_return(42.0);
  CHECK_THROWS(caller.process(&ctx));
  CHECK(ctx.current_script == nullptr);
  CHECK(ctx.frame_base == 0);
  CHECK(ctx.arg_base == 0);
  CHECK(ctx.saved_base == 0);
  CHECK(ctx.list_base == 0);
  CHECK(ctx.get_return<double>() == 42.0);
  CHECK(ctx.get_saved<double>(caller.find_saved("x")) == 2.0);
  REQUIRE(ctx.lists.size() == caller.lists.size());
  REQUIRE(ctx.lists[caller.find_list("nums")].size() == 2);
  CHECK(ctx.lists[caller.find_list("nums")][1].get<double>() == 9.0);
  ctx.clear();
  CHECK(ctx.current_script == nullptr);
  CHECK(ctx.return_type().empty());
  const auto ok = sys.parse<double, void>("ok", "12.0");
  ok.process(&ctx);
  CHECK(ctx.get_return<double>() == 12.0);
}

TEST_CASE("audit: constant folding keeps the semantics of the unfolded stream") {
  ds::system sys;
  sys.init_basic_functions(); sys.init_math();
  sys.register_function<&dbl>("dbl");
  ds::context ctx;

  // Folding still happens: a constant sub-expression collapses to pushvalue/pushbool + pushreturn.
  const auto folded = sys.parse<double, void>("folded", "1.0 - 2.0 - 3.0");
  CHECK(folded.cmds.size() == 2);
  folded.process(&ctx);
  CHECK(ctx.get_return<double>() == -4.0);

  for (const auto* source : { "AND = { true, true }", "NOR = { false, false }", "2.0 >= 2.0", "not false" }) {
    const auto boolean = sys.parse<bool, void>("boolean", source);
    CHECK(boolean.cmds.size() == 2);
    ctx.clear();
    boolean.process(&ctx);
    CHECK(ctx.get_return<bool>());
  }

  // The accumulator starts at the first operand, so folding an n-ary block cannot add a leading
  // identity - that used to turn every negative zero into a positive one.
  for (const auto* source : { "-0.0", "unary_minus = 0.0", "ADD = { -0.0 }", "-0.0 * 1.0", "MUL = { -0.0, 1.0 }" }) {
    const auto zero = sys.parse<double, void>("zero", source);
    ctx.clear();
    zero.process(&ctx);
    CHECK(std::signbit(ctx.get_return<double>()));
  }

  // A folded constant may only stand in for a value codegen would have produced here. Mixing kinds
  // needs a conversion the folder cannot see, so these must be rejected exactly as when unfolded.
  CHECK_THROWS(sys.parse<bool, void>("num_as_bool", "0.0"));
  CHECK_THROWS(sys.parse<bool, void>("num_as_bool", "1.0 + 2.0"));
  CHECK_THROWS(sys.parse<double, void>("bool_as_num", "true and true"));
  CHECK_THROWS(sys.parse<bool, void>("mixed_eq", "true == 1.0"));
  CHECK_THROWS([&] { const auto s = sys.parse<bool, void>("num_block", "AND = { 1.0, 2.0 }"); ctx.clear(); s.process(&ctx); }());

  // Comparisons fold to what the runtime opcode computes: doubles compare with a tolerance, and an
  // integer literal outside an integral block is a double there too.
  ctx.clear();
  const auto eps = sys.parse<bool, void>("eps", "1.0 == 1.0000000001");
  CHECK(eps.cmds.size() == 2);
  eps.process(&ctx);
  CHECK(ctx.get_return<bool>());

  // Integral blocks are never folded: the folder works in double and cannot represent int64 exactly.
  const auto integral = sys.parse<int64_t, void>("integral", "9007199254740992 + 1");
  CHECK(integral.cmds.size() > 2);
  ctx.clear();
  integral.process(&ctx);
  CHECK(ctx.get_return<int64_t>() == 9007199254740993LL);
}

TEST_CASE("audit: constant folding never replaces a user-registered operator") {
  ds::system sys;
  sys.init_basic_functions();
  CHECK(sys.is_builtin_function("ADD"));
  CHECK_FALSE(sys.is_builtin_function("+"));
  sys.register_operator<&counting_add>("+",
    ds::system::operator_props{ 11, ds::system::command_data::math_ftype::binary, ds::system::command_data::associativity::left });
  CHECK_FALSE(sys.is_builtin_function("+"));

  fold_calls = 0;
  const auto script = sys.parse<double, void>("user_op", "1.0 + 2.0 + 3.0");
  ds::context ctx;
  script.process(&ctx);
  CHECK(fold_calls == 2);
  CHECK(ctx.get_return<double>() == 206.0);

  // Names the consumer did not touch keep folding.
  const auto builtin = sys.parse<double, void>("builtin_block", "ADD = { 1.0, 2.0 }");
  CHECK(builtin.cmds.size() == 2);
  ctx.clear();
  builtin.process(&ctx);
  CHECK(ctx.get_return<double>() == 3.0);
}
