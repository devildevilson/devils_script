#include <doctest/doctest.h>
#include "devils_script/system.h"

#include <array>
#include <vector>

namespace ds = devils_script;

namespace {
struct scope { int64_t id; bool valid() const { return id != 0; } };
using arguments = std::array<int64_t, 4>; // scope id (zero for a free function), then a, b, c
struct observations {
  std::vector<arguments> calls, effects;
  std::vector<std::string_view> names;
  std::vector<int64_t> results;
} observed;

int64_t ordered(int64_t a, int64_t b, int64_t c) {
  observed.calls.push_back({ 0, a, b, c });
  return 10000 * a + 100 * b + c;
}
int64_t scoped_ordered(scope s, int64_t a, int64_t b, int64_t c) {
  observed.calls.push_back({ s.id, a, b, c });
  return 1000000 * s.id + 10000 * a + 100 * b + c;
}
void consume(int64_t a, int64_t b, int64_t c) { (void)ordered(a, b, c); }
void scoped_consume(scope s, int64_t a, int64_t b, int64_t c) { (void)scoped_ordered(s, a, b, c); }

using free_args = std::tuple<int64_t, int64_t, int64_t>;
using scoped_args = std::tuple<scope, int64_t, int64_t, int64_t>;
void on_consume(void* ptr, const std::string_view& name, const free_args& args) {
  auto& out = *static_cast<observations*>(ptr);
  out.effects.push_back({ 0, std::get<0>(args), std::get<1>(args), std::get<2>(args) });
  out.names.push_back(name);
}
void on_ordered(void* ptr, const std::string_view& name, const int64_t& ret, const free_args& args) {
  on_consume(ptr, name, args);
  static_cast<observations*>(ptr)->results.push_back(ret);
}
void on_scoped_consume(void* ptr, const std::string_view& name, const scoped_args& args) {
  auto& out = *static_cast<observations*>(ptr);
  out.effects.push_back({ std::get<0>(args).id, std::get<1>(args), std::get<2>(args), std::get<3>(args) });
  out.names.push_back(name);
}
void on_scoped_ordered(void* ptr, const std::string_view& name, const int64_t& ret, const scoped_args& args) {
  on_scoped_consume(ptr, name, args);
  static_cast<observations*>(ptr)->results.push_back(ret);
}

// Test the runtime call contract independently of parsing/optimization. A sentinel and a scope in
// an outer frame expose double consumption as well as swapped arguments. Distinct values are
// essential: sum(a, b) would hide f(pop(), pop()) on compilers choosing the wrong evaluation order.
template <typename Scope>
void check_call(ds::function_t thunk, bool has_result, bool has_effect) {
  observed = {};
  ds::context ctx;
  ds::script_container scr;
  ctx.userptr = &observed;
  ctx.frame_base = 1;
  ctx.stack.push(INT64_C(-987));
  constexpr bool scoped = !std::is_void_v<Scope>;
  if constexpr (scoped) ctx.stack.push(scope{ 7 });
  const size_t prefix_size = ctx.stack.size();
  for (const int64_t value : { 11, 22, 33 }) ctx.stack.push(value);
  if (has_effect) ctx.stack.push(std::string_view("ordered_call"));

  CHECK(thunk(0, &ctx, &scr) == (has_result ? -2 : -3));
  const std::vector<arguments> expected{ { scoped ? 7 : 0, 11, 22, 33 } };
  CHECK(observed.calls == expected);
  REQUIRE(ctx.stack.size() == prefix_size + size_t(has_result));
  if (has_result) CHECK(ctx.stack.safe_pop<int64_t>() == (scoped ? 7112233 : 112233));
  CHECK(ctx.stack.safe_get<int64_t>(0) == -987);
  if constexpr (scoped) CHECK(ctx.stack.safe_get<scope>(1).id == 7);
  if (has_effect) {
    CHECK(observed.effects == expected);
    CHECK(observed.names == std::vector<std::string_view>{ "ordered_call" });
    if (has_result) CHECK(observed.results == std::vector<int64_t>{ scoped ? 7112233 : 112233 });
    else CHECK(observed.results.empty());
  } else {
    CHECK(observed.effects.empty());
    CHECK(observed.names.empty());
  }
}

template <auto Function, typename Scope, ds::on_effect_t<decltype(Function), Scope> Effect>
void check_user_calls(bool has_result) {
  const ds::function_t thunks[] = {
    &ds::userfunc<Function, Scope, &ds::is_valid<Scope>>,
    &ds::userfunc_unsafe<Function, Scope, &ds::is_valid<Scope>>,
    &ds::userfunc<Function, Scope, &ds::is_valid<Scope>, Effect>,
    &ds::userfunc_unsafe<Function, Scope, &ds::is_valid<Scope>, Effect>,
  };
  for (size_t i = 0; i < std::size(thunks); ++i) {
    CAPTURE(i);
    check_call<Scope>(thunks[i], has_result, i >= 2);
  }
}

int64_t mark(int64_t value) {
  observed.results.push_back(value);
  return value;
}
bool mixed(int64_t first, std::string_view second, double third) {
  return first == 11 && second == "middle" && third == 3.25;
}
} // namespace

TEST_CASE("sequencing: runtime calls preserve argument positions and the outer stack") {
  SUBCASE("free value function") { check_user_calls<&ordered, void, &on_ordered>(true); }
  SUBCASE("scoped value function") { check_user_calls<&scoped_ordered, scope, &on_scoped_ordered>(true); }
  SUBCASE("free void function") { check_user_calls<&consume, void, &on_consume>(false); }
  SUBCASE("scoped void function") { check_user_calls<&scoped_consume, scope, &on_scoped_consume>(false); }
  SUBCASE("free math function") {
    check_call<void>(&ds::mathfunc<&ordered>, true, false);
    check_call<void>(&ds::mathfunc_unsafe<&ordered>, true, false);
  }
  SUBCASE("scoped math function") {
    check_call<scope>(&ds::mathfunc<&scoped_ordered, scope, &ds::is_valid<scope>>, true, false);
    check_call<scope>(&ds::mathfunc_unsafe<&scoped_ordered, scope, &ds::is_valid<scope>>, true, false);
  }
}

TEST_CASE("sequencing: nested script arguments execute once in source order") {
  for (const bool safe : { true, false }) {
    for (const bool optimize : { true, false }) {
      CAPTURE(safe);
      CAPTURE(optimize);
      ds::system sys;
      if (!safe) sys.toggle_safety();
      if (!optimize) sys.toggle_optimizations();
      sys.init_basic_functions();
      sys.init_math();
      sys.register_function<&ordered>("ordered");
      sys.register_function<&mark>("mark");
      sys.register_function<&mixed>("mixed");

      observed = {};
      const auto script = sys.parse<int64_t, void>("s", "ordered(mark(4), ordered(mark(1), mark(2), mark(3)), mark(5))");
      CHECK(observed.calls.empty());
      CHECK(observed.results.empty());
      ds::context ctx;
      script.process(&ctx);
      CHECK(observed.results == std::vector<int64_t>{ 4, 1, 2, 3, 5 });
      CHECK(observed.calls == std::vector<arguments>{ { 0, 1, 2, 3 }, { 0, 4, 10203, 5 } });
      CHECK(ctx.get_return<int64_t>() == 1060305);
      CHECK(ctx.stack.size() == 0);

      const auto typed = sys.parse<bool, void>("s", "mixed(mark(11), 'middle', 3.25)");
      typed.process(&ctx);
      CHECK(ctx.get_return<bool>());
      CHECK(ctx.stack.size() == 0);
    }
  }
}
