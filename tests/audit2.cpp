// Regressions for the 2026-10-19 audit (AUDIT.md, top section). Item numbers refer to it.
#include <doctest/doctest.h>
#include "devils_script/system.h"

#include <cmath>
#include <functional>
#include <optional>
#include <string>

namespace ds = devils_script;

namespace {

struct big { int64_t a[4]; big* other; };
void touch(big* b) { b->a[1] += 1; }
big* other(big* b) { return b->other; }
int64_t big_id(big* b) { return b->a[2]; }
const big* self_const(big* b) { return b; }
// The qualifier is the point: it stays part of the function type and used to make it an integer.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
const bool const_truth() { return true; }
#pragma GCC diagnostic pop
void add_to_big(big* b, const int64_t x) { b->a[0] += x; }

int ticks = 0;
void tick() { ++ticks; }
int64_t idi(const int64_t v) { return v; }
bool truth = true;
bool nb() { return truth; }
double boom() { throw std::runtime_error("boom"); }

struct item { int64_t id; bool valid() const { return id != 0; } };
void each_item_do(const std::function<void(item)>& value) {
  for (int64_t i = 1; i <= 3; ++i) value(item{ i });
}
void each_item_if(const std::function<bool(item)>& filter, const std::function<void(item)>& value) {
  for (int64_t i = 1; i <= 3; ++i) if (filter(item{ i })) value(item{ i });
}

// Every configuration a script can be compiled and run in. The optimizer used to hide some of the
// stack-model defects by deleting the push/erase pair that exposed them.
struct config { bool safe; bool optimize; };
constexpr config configs[] = { { true, true }, { true, false }, { false, true }, { false, false } };

ds::system make(const config cfg) {
  ds::system sys;
  if (!cfg.safe) sys.toggle_safety();
  if (!cfg.optimize) sys.toggle_optimizations();
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&touch>("touch");
  sys.register_function<&other>("other");
  sys.register_function<&add_to_big>("add_to_big");
  sys.register_function<&big_id>("big_id");
  sys.register_function<&self_const>("self_const");
  sys.register_function<&const_truth>("const_truth");
  sys.register_function<&tick>("tick");
  sys.register_function<&idi, void>("idi");
  sys.register_function<&nb>("nb");
  sys.register_function<&boom>("boom");
  sys.register_function_iter<&each_item_do>("each_item_do", { "value" });
  sys.register_function_iter<&each_item_if>("each_item_if", { "filter", "value" });
  return sys;
}

}  // namespace

TEST_CASE("audit2 #1: an effect in a block does not shift later stack slots") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);

    {
      big second{ { 0, 0, 0, 0 }, nullptr };
      big root{ { 0, 0, 0, 0 }, &second };
      const auto script = sys.parse<void, big*>("s", "{ touch, other = { add_to_big = 10 } }");
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(root.a[1] == 1);
      CHECK(second.a[0] == 10);
      CHECK(root.a[0] == 0);
    }

    {
      big root{ { 0, 0, 0, 0 }, nullptr };
      const auto script = sys.parse<void, big*>("s", "{ add_to_big = 1, this = { add_to_big = 10 } }");
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(root.a[0] == 11);
    }

    {
      const auto script = sys.parse<void, void>("s", "{ ctx_save = { s = 1.5 }, ctx_save = { r = ctx:saved:s + 1.0 } }");
      ds::context ctx;
      script.process(&ctx);
      CHECK(ctx.get_saved<double>(script.find_saved("r")) == 2.5);
    }

    {
      // `r` used to receive the internal `thisctx` handle.
      const auto script = sys.parse<void, void>("s", "{ ctx_set = { a = 1.5 }, ctx_save = { r = ctx:arg:a } }");
      ds::context ctx;
      script.process(&ctx);
      CHECK(ctx.get_saved<double>(script.find_saved("r")) == 1.5);
    }

    {
      // `r` used to be the switch value 3 instead of the selected branch.
      ticks = 0;
      const auto script = sys.parse<void, void>("s", "{ tick, ctx_save = { r = { switch = { value = 3, { value = 3, true } } } } }");
      ds::context ctx;
      script.process(&ctx);
      CHECK(ticks == 1);
      CHECK(ctx.get_saved<bool>(script.find_saved("r")) == true);
    }

    {
      // Used to crash in cmpeq2, comparing against the wrong slot.
      ticks = 0;
      const auto script = sys.parse<void, void>("s", "{ tick, tick, switch = { value = 3, { value = 2, tick }, { value = 3, tick } } }");
      ds::context ctx;
      script.process(&ctx);
      CHECK(ticks == 3);
      CHECK(ctx.stack.size() == 0);
    }

    {
      // Control flow in an effect block leaves no placeholder behind either; each form is followed
      // by a scope block, whose index would be off by one.
      for (const auto source : {
        "{ select = { { condition = true, tick }, { tick } }, other = { add_to_big = 10 } }",
        "{ sequence = { { condition = true, tick }, { condition = true, tick } }, other = { add_to_big = 10 } }",
        "{ switch = { value = 1, { value = 1, tick }, { value = 2, tick } }, other = { add_to_big = 10 } }",
        "{ random = { { weight = 1, tick }, { weight = 1, tick } }, other = { add_to_big = 10 } }",
        "{ tick, each_item_do = { value = { tick } }, other = { add_to_big = 10 } }",
      }) {
        CAPTURE(source);
        big second{ { 0, 0, 0, 0 }, nullptr };
        big root{ { 0, 0, 0, 0 }, &second };
        const auto script = sys.parse<void, big*>("s", source);
        ds::context ctx;
        ctx.set_arg(0, &root);
        script.process(&ctx);
        CHECK(second.a[0] == 10);
        CHECK(root.a[0] == 0);
        CHECK(ctx.stack.size() == 0);
      }
    }

    {
      // Effect bodies of iterators are effect blocks too.
      ticks = 0;
      const auto script = sys.parse<void, void>("s", "{ tick, each_item_do = { value = { tick, tick } }, tick }");
      ds::context ctx;
      script.process(&ctx);
      CHECK(ticks == 8);
      CHECK(ctx.stack.size() == 0);
    }
  }
}

TEST_CASE("audit2 #2: a value-less child is not folded as an operand") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);

    // In a block an effect is a legitimate child that adds nothing.
    for (const auto& [source, expected] : { std::pair{ "{ 1.0, ctx_save = { y = 2.0 } }", 1.0 },
                                            std::pair{ "{ ctx_save = { y = 2.0 }, 1.0 }", 1.0 },
                                            std::pair{ "{ 1.0, ctx_save = { y = 2.0 }, 2.0 }", 3.0 },
                                            std::pair{ "{ 2.0, ctx_save = { y = 3.0 }, ctx:saved:y }", 5.0 },
                                            std::pair{ "MUL = { 2.0, ctx_save = { y = 2.0 }, 3.0 }", 6.0 } }) {
      CAPTURE(source);
      const auto script = sys.parse<double, void>("s", source);
      ds::context ctx;
      script.process(&ctx);
      CHECK(ctx.get_return<double>() == expected);
      CHECK(ctx.get_saved<double>(script.find_saved("y")) != 0.0);
    }

    // An operator operand has to be a value; these used to compile and fold over a missing operand.
    for (const auto source : { "2 * { value = 3 }", "1 + { custom_description = x }" }) {
      CAPTURE(source);
      CHECK_THROWS(sys.parse<double, void>("s", source));
    }
    CHECK_THROWS(sys.parse<bool, void>("s", "true and { value = false }"));
  }
}

TEST_CASE("audit2 #5: a branch result has the type its block promises") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);

    // An integer branch in a floating-point block is converted, also when it is not a literal.
    for (const auto source : {
      "select = { { condition = true, idi(20) }, { 1.5 } }",
      "switch = { value = 1, { value = 1, idi(20) }, { value = 2, 1.5 } }",
      "random = { { weight = 2, idi(20) } }",
      "random = { { weight = idi(2), 20.0 } }",
      "{ 0.5, select = { { condition = true, 20 }, { 1.5 } }, -0.5 }",
    }) {
      CAPTURE(source);
      const auto script = sys.parse<double, void>("s", source);
      ds::context ctx;
      script.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 20.0);
    }

    // A branch that cannot become the promised type is rejected instead of leaking through: an int
    // script used to return a double, a double script a bool.
    for (const auto source : {
      "select = { { condition = false, 10 }, { 2.5 } }",
      "switch = { value = 1, { value = 1, 2.5 } }",
    }) {
      CAPTURE(source);
      CHECK_THROWS(sys.parse<int64_t, void>("s", source));
    }
    CHECK_THROWS(sys.parse<double, void>("s", "select = { { condition = true, true }, { 1.0 } }"));

    // A number is not a truth value: every child of a boolean block is a bool, the first one too.
    for (const auto source : { "AND = { 5, true }", "{ 2.5, true }", "OR = { false, 1 }", "select = { { 5 } }" }) {
      CAPTURE(source);
      CHECK_THROWS(sys.parse<bool, void>("s", source));
    }
  }
}

TEST_CASE("audit2 #31: a condition is bool inside a numeric block") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    for (const bool value : { true, false }) {
      truth = value;
      {
        const auto script = sys.parse<double, void>("s", "select = { { condition = nb, 1.5 }, { 2.5 } }");
        ds::context ctx;
        script.process(&ctx);
        CHECK(ctx.get_return<double>() == (value ? 1.5 : 2.5));
      }
      {
        const auto script = sys.parse<int64_t, void>("s", "{ ADD = { { condition = nb, 100 }, 3 } }");
        ds::context ctx;
        script.process(&ctx);
        CHECK(ctx.get_return<int64_t>() == (value ? 103 : 3));
      }
    }
  }
}

namespace {
template <typename R>
R run(const ds::system& sys, const char* source) {
  const auto script = sys.parse<R, void>("s", source);
  ds::context ctx;
  script.process(&ctx);
  if constexpr (!std::is_void_v<R>) {
    REQUIRE(ctx.template is_return<R>());
    return ctx.template get_return<R>();
  }
}
}  // namespace

TEST_CASE("audit2 #3: a value-producing switch has a default or fails loudly") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);

    // The last case without 'value' is the default.
    CHECK(run<double>(sys, "switch = { value = 1, { value = 1, 10 }, { 30 } }") == 10.0);
    CHECK(run<double>(sys, "switch = { value = 2, { value = 1, 10 }, { 30 } }") == 30.0);
    CHECK(run<double>(sys, "{ 100, switch = { value = 2, { value = 1, 10 }, { 30 } } }") == 130.0);
    CHECK(run<int64_t>(sys, "switch = { value = 3, { value = 1, 10 }, { value = 2, 20 }, { idi(30) } }") == 30);

    // Without a default, no match used to leave the value missing: an empty return, a garbage read
    // inside an expression.
    for (const auto source : { "switch = { value = 2, { value = 1, 0.5 } }", "{ 100, switch = { value = 2, { value = 1, 10 } } }" }) {
      CAPTURE(source);
      const auto script = sys.parse<double, void>("s", source);
      ds::context ctx;
      CHECK_THROWS_WITH(script.process(&ctx), doctest::Contains("found no matching case"));
    }

    // In an effect block no match simply does nothing, and a default runs.
    ticks = 0;
    run<void>(sys, "{ switch = { value = 2, { value = 1, tick } }, tick }");
    CHECK(ticks == 1);
    ticks = 0;
    run<void>(sys, "{ switch = { value = 2, { value = 1, tick }, { tick, tick } }, tick }");
    CHECK(ticks == 3);

    CHECK_THROWS(sys.parse<double, void>("s", "switch = { value = 1, { 5 }, { value = 1, 10 } }"));   // default not last
    CHECK_THROWS(sys.parse<double, void>("s", "switch = { value = 1, { 5 } }"));                       // only a default
  }
}

TEST_CASE("audit2 #4: random rejects a negative or NaN weight") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);

    for (const auto source : {
      "random = { { weight = -1, 3.0 }, { weight = -2, 4.0 } }",
      "{ 100, random = { { weight = 1, 3.0 }, { weight = -2, 4.0 } } }",
      "random = { { weight = 0.0 / 0, 3.0 } }",
    }) {
      CAPTURE(source);
      const auto script = sys.parse<double, void>("s", source);
      ds::context ctx;
      CHECK_THROWS_WITH(script.process(&ctx), doctest::Contains("'random' weight must be a non-negative number"));
    }

    // Zero weights are fine; with all of them zero the first branch is taken.
    CHECK(run<double>(sys, "random = { { weight = 0, 3.0 }, { weight = 0, 4.0 } }") == 3.0);
    const double picked = run<double>(sys, "random = { { weight = 1, 3.0 }, { weight = 2, 4.0 } }");
    CHECK((picked == 3.0 || picked == 4.0));
  }
}

TEST_CASE("audit2 #6: a list cannot be changed inside its own pipeline") {
  auto sys = make(configs[0]);
  for (const auto source : {
    "{ ctx:list:xs = { add_to = 1.0, add_to = 2.0 }, ctx:list:xs = { sum = { ADD = { ctx:list:xs = { add_to = 3.0 }, 1.0 } } } }",
    "{ ctx:list:xs = { add_to = 1.0 }, ctx:list:xs = { any = { AND = { ctx:list:xs = { add_to = 3.0 }, false } } } }",
    "{ ctx:list:xs = { add_to = 1.0 }, ctx:list:xs = { filter = { AND = { ctx:list:xs = { clear }, true } } }, 0.0 }",
  }) {
    CAPTURE(source);
    CHECK_THROWS_WITH(((void)sys.parse<double, void>("s", source)), doctest::Contains("inside its own pipeline"));
  }
  // Another list, or only reading the iterated one, is fine.
  const auto script = sys.parse<double, void>("s",
    "{ ctx:list:xs = { add_to = 1.0, add_to = 2.0 }, ctx:list:xs = { sum = { ADD = { ctx:list:ys = { add_to = 3.0 }, ctx:list:xs = { count } } } } }");
  ds::context ctx;
  ctx.create_lists(&script);
  script.process(&ctx);
  CHECK(ctx.get_return<double>() == 4.0);
}

TEST_CASE("audit2 #7: a runtime type tag does not point into the container") {
  auto sys = make(configs[0]);
  big root{ { 0, 0, 0, 0 }, nullptr };
  ds::context ctx;
  const char* pool_begin = nullptr;
  const char* pool_end = nullptr;
  {
    // The list element's type reaches the saved slot through `ctx_save_as` inside the callback.
    auto script = sys.parse<double, big*>("a",
      "{ ctx:list:kids = { add_to = this }, ctx:list:kids = { sum = { value_or = { true, { ctx_save_as = el, 1.0 }, 0.0 } } } }");
    pool_begin = script.string_pool.data();
    pool_end = pool_begin + script.string_pool.size();
    ctx.create_lists(&script);
    ctx.set_arg(0, &root);
    script.process(&ctx);
    CHECK(ctx.get_return<double>() == 1.0);
  }   // the container, and its string pool, are gone
  const auto tag = ctx.saved_stack.type(0);
  CHECK(tag == ds::utils::type_name<big*>());
  CHECK_FALSE((tag.data() >= pool_begin && tag.data() < pool_end));
}

TEST_CASE("audit2 #8 #9: remove_from of an absent value, clamp with crossed bounds") {
  auto sys = make(configs[0]);
  const auto list = sys.parse<double, void>("s", "{ ctx:list:xs = { add_to = 1.0, remove_from = 2.0 }, ctx:list:xs = { count } }");
  ds::context ctx;
  ctx.create_lists(&list);
  list.process(&ctx);
  CHECK(ctx.get_return<double>() == 1.0);   // the 1.0 used to be removed instead

  const auto empty = sys.parse<double, void>("s", "{ ctx:list:xs = { remove_from = 2.0 }, ctx:list:xs = { count } }");
  ds::context ectx;
  ectx.create_lists(&empty);
  empty.process(&ectx);
  CHECK(ectx.get_return<double>() == 0.0);

  CHECK(run<double>(sys, "clamp(5, 0, 10)") == 5.0);
  const auto crossed = sys.parse<double, void>("s", "clamp(5, 10, 0)");
  ds::context cctx;
  CHECK_THROWS_WITH(crossed.process(&cctx), doctest::Contains("lower bound"));
}

TEST_CASE("audit2 #10 #11 #12 #19: calls and blocks without enough values") {
  auto sys = make(configs[0]);
  // Each of these used to crash the parser (an empty stack, unbounded recursion) or, for one argument,
  // return it unchanged without calling the function.
  for (const auto source : { "", "{}", "// only a comment", "max()", "ADD = {}", "MUL", "{ custom_description = x }", "max(1)", "step(0.5)" }) {
    CAPTURE(source);
    CHECK_THROWS(sys.parse<double, void>("s", source));
  }
  for (const auto source : { "", "AND", "AND = {}", "OR = { }", "NOR", "true and AND" }) {
    CAPTURE(source);
    CHECK_THROWS(sys.parse<bool, void>("s", source));
  }
  CHECK(run<double>(sys, "max(1, 2.5, 2)") == 2.5);
  CHECK(run<double>(sys, "ADD = { 1.5 }") == 1.5);
  CHECK(run<bool>(sys, "AND = { true }"));
}

TEST_CASE("audit2 #14 #35: streamed rows keep the root scope and roll back on error") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);

    // Rows: each one is compiled as it arrives; the second one is rejected and must leave nothing.
    tavl::parser p;
    sys.configure_parser(p);
    p.flush("touch\n{ tick, bogus_function }\nadd_to_big = 5\n");
    p.finish();
    ds::system::parse_context parse_ctx;
    ds::container c;
    parse_ctx.init<void, big*>(sys, c);
    size_t rejected = 0;
    for (;;) {
      const auto [ev, err] = sys.parse("streamed", p, parse_ctx, c);
      if (!err.no_error()) rejected += 1;
      if (ev.type == tavl::event_type::eof) break;
    }
    CHECK(rejected == 1);

    big root{ { 0, 0, 0, 0 }, nullptr };
    ticks = 0;
    ds::context ctx;
    ctx.set_arg(0, &root);
    c.process(&ctx);
    CHECK(root.a[1] == 1);
    CHECK(root.a[0] == 5);
    CHECK(ticks == 0);
    CHECK(ctx.stack.size() == 0);
  }
}

TEST_CASE("audit2 #25: folded != agrees with the runtime for NaN") {
  auto sys = make(configs[0]);
  CHECK(run<bool>(sys, "1.0 != (0.0 / 0.0)"));
  CHECK_FALSE(run<bool>(sys, "1.0 == (0.0 / 0.0)"));
}

TEST_CASE("audit2 #15: a scoped call inside an executed sub-script reads its own scope") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    const auto rsub = sys.parse<int64_t, big*>("rsub", "big_id");
    sys.set_script_resolver([&](std::string_view) -> const ds::script_container* { return &rsub; });
    // The caller's stack holds its own root and the `other` scope below the sub-script's frame; the
    // sub-script used to read the caller's slot 0 (the root) instead of its own root.
    const auto caller = sys.parse<int64_t, big*>("caller", "{ execute = { rsub }, other = { execute = { rsub } } }");
    big second{ { 0, 0, 20, 0 }, nullptr };
    big root{ { 0, 0, 1, 0 }, &second };
    ds::context ctx;
    ctx.set_arg(0, &root);
    caller.process(&ctx);
    CHECK(ctx.get_return<int64_t>() == 21);
  }
}

TEST_CASE("audit2 #16: a scope path with a plain right-hand side keeps its size") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    big third{ { 0, 0, 300, 0 }, nullptr };
    big second{ { 0, 0, 20, 0 }, &third };
    big root{ { 0, 0, 1, 0 }, &second };
    const auto run_root = [&](const char* source) {
      const auto script = sys.parse<int64_t, big*>("s", source);
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      return ctx.get_return<int64_t>();
    };
    // The path used to swallow the following row: 600 instead of 301.
    CHECK(run_root("{ other.other = big_id, big_id }") == 301);
    CHECK(run_root("{ big_id, other.other = big_id }") == 301);   // last row: used to read past the output
    CHECK(run_root("other.other = big_id") == 300);
    CHECK(run_root("other.other = { big_id }") == 300);
  }
}

TEST_CASE("audit2 #17: a saved scope keeps the rest of its path") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    big third{ { 0, 0, 300, 0 }, nullptr };
    big second{ { 0, 0, 20, 0 }, &third };
    big root{ { 0, 0, 1, 0 }, &second };
    for (const auto& [source, expected] : {
      std::pair{ "{ ctx_save = { x = this }, ctx:saved:x.other:big_id }", 20 },
      std::pair{ "{ ctx_save = { x = this }, ctx:saved:x.other = { big_id } }", 20 },
      std::pair{ "{ ctx_save = { x = this }, ctx:saved:x.other.other:big_id }", 300 },
      std::pair{ "{ ctx_save = { x = this }, ctx:saved:x = { big_id } }", 1 },
    }) {
      CAPTURE(source);
      const auto script = sys.parse<int64_t, big*>("s", source);
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(ctx.get_return<int64_t>() == expected);
    }
  }
}

TEST_CASE("audit2 #18: a function without parameters rejects arguments") {
  auto sys = make(configs[0]);
  CHECK_THROWS_WITH(((void)sys.parse<int64_t, big*>("s", "big_id = 5")), doctest::Contains("takes no arguments"));
  CHECK_THROWS(((void)sys.parse<int64_t, big*>("s", "big_id = nonexistent_thing")));
  CHECK_THROWS_WITH(((void)sys.parse<void, big*>("s", "touch = 5")), doctest::Contains("takes no arguments"));
  CHECK_THROWS_WITH(((void)sys.parse<void, big*>("s", "touch = { tick }")), doctest::Contains("takes no arguments"));
  // A scope-returning function may still be followed by a block in the scope it returns.
  big second{ { 0, 0, 20, 0 }, nullptr };
  big root{ { 0, 0, 1, 0 }, &second };
  const auto script = sys.parse<int64_t, big*>("s", "other = { big_id }");
  ds::context ctx;
  ctx.set_arg(0, &root);
  script.process(&ctx);
  CHECK(ctx.get_return<int64_t>() == 20);
}

TEST_CASE("audit2 #20: ctx_set_as names an argument slot") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    big second{ { 0, 0, 20, 0 }, nullptr };
    big root{ { 0, 0, 1, 0 }, &second };
    {
      // Setting the same name twice reuses its slot; it used to add a second `o`, read stale.
      const auto script = sys.parse<int64_t, big*>("s", "{ ctx_set_as = o, other = { ctx_set_as = o }, ctx:arg:o = { big_id } }");
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(ctx.get_return<int64_t>() == 20);
      CHECK(script.args.size() == 2);   // root + o
    }
    {
      // A saved value of the same name used to make it overwrite the root argument.
      const auto script = sys.parse<int64_t, big*>("s", "{ ctx_save = { o = 1 }, other = { ctx_set_as = o }, big_id }");
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(ctx.get_return<int64_t>() == 1);
      CHECK(ctx.get_arg<big*>(0) == &root);
    }
  }
}

TEST_CASE("audit2 #21 #44: execute arguments and value_or branches convert honestly") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    const auto sub = sys.parse<int64_t, void>("isub", "ctx:arg:n + 1");
    const auto dsub = sys.parse<double, void>("dsub", "ctx:arg:x * 2.0");
    sys.set_script_resolver([&](std::string_view name) -> const ds::script_container* { return name == "isub" ? &sub : &dsub; });

    // A double used to be truncated into an integer argument (2.7 -> 2), UB when out of range.
    CHECK_THROWS(((void)sys.parse<int64_t, void>("s", "execute = { isub, n = 2.7 }")));
    CHECK(run<int64_t>(sys, "execute = { isub, n = 2 }") == 3);
    CHECK(run<double>(sys, "execute = { dsub, x = 2 }") == 4.0);   // integer -> floating point is implicit

    // value_or: each branch is converted inside the branch, the result has the value's type.
    for (const bool flag : { true, false }) {
      truth = flag;
      CHECK(run<double>(sys, "value_or = { nb, idi(2), 0.5 }") == (flag ? 2.0 : 0.5));
      CHECK(run<double>(sys, "value_or = { nb, 0.5, idi(2) }") == (flag ? 0.5 : 2.0));
      CHECK(run<bool>(sys, "EQ = { value_or = { nb, 1, 2 }, idi(1) }") == flag);   // used to be rejected: 'view' vs 'long'


    }
    CHECK_THROWS(((void)sys.parse<int64_t, void>("s", "value_or = { nb, 1, 2.5 }")));
  }
}

TEST_CASE("audit2 #22 #28 #29: context reuse") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);

    {
      // #22: after clear() a read before the write is an error again, not the previous run's value.
      const auto script = sys.parse<double, void>("s", "{ value_or = { nb, { ctx_save = { x = 7.0 }, 1.0 }, 2.0 }, ctx:saved:x }");
      ds::context ctx;
      truth = true;
      script.process(&ctx);
      CHECK(ctx.get_return<double>() == 8.0);
      ctx.clear();
      truth = false;
      if (cfg.safe) CHECK_THROWS(script.process(&ctx));
    }

    {
      // #29: a second process() without clear() starts over, also after an exception.
      const auto first = sys.parse<double, void>("a", "3.5");
      const auto second = sys.parse<double, void>("b", "{ idi(10), idi(20) }");
      ds::context ctx;
      first.process(&ctx);
      CHECK(ctx.get_return<double>() == 3.5);
      second.process(&ctx);
      CHECK(ctx.get_return<double>() == 30.0);
      second.process(&ctx);
      CHECK(ctx.get_return<double>() == 30.0);
      CHECK(ctx.stack.size() == 0);
    }

    {
      // #28: lists without create_lists() are a clear error, not an out-of-bounds access.
      const auto script = sys.parse<double, void>("s", "{ ctx:list:xs = { add_to = 1.0 }, ctx:list:xs = { count } }");
      ds::context ctx;
      CHECK_THROWS_WITH(script.process(&ctx), doctest::Contains("create_lists"));
      ctx.create_lists(&script);
      script.process(&ctx);
      CHECK(ctx.get_return<double>() == 1.0);
    }
  }
}

TEST_CASE("audit2 #23: describe() never runs a void iterator") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    for (const auto source : {
      "{ tick, each_item_do = { value = { tick } }, tick }",
      "{ tick, each_item_if = { filter = { true }, value = { tick } }, tick }",
    }) {
      CAPTURE(source);
      const auto script = sys.parse<void, void>("s", source);
      ticks = 0;
      ds::context ctx;
      bool described_iterator = false;
      script.describe(&ctx, [&](const ds::container::description_entry& e) {
        described_iterator = described_iterator || e.name == "each_item_do" || e.name == "each_item_if";
      });
      CHECK(ticks == 0);
      CHECK(described_iterator);
      ds::context run_ctx;
      script.process(&run_ctx);
      CHECK(ticks == 5);
    }
  }
}

TEST_CASE("audit2 #26 #27: execute recursion and argument frames are bounded") {
  auto sys = make(configs[0]);
  {
    // A script that executes itself through the resolver used to overflow the C++ stack.
    ds::container self;
    sys.set_script_resolver([&](std::string_view) -> const ds::script_container* { return &self; });
    self = sys.parse<void, void>("self", "tick");             // registered first so `execute` resolves
    self = sys.parse<void, void>("self", "{ tick, execute = { self } }");
    ticks = 0;
    ds::context ctx;
    CHECK_THROWS_WITH(self.process(&ctx), doctest::Contains("nests deeper than"));
    CHECK(ctx.execute_depth == 0);
  }
  {
    // Three scripts with three arguments each, chained: 9 slots of an 8-slot argument stack. This
    // used to parse and fail only at run time.
    const auto s3 = sys.parse<double, void>("s3", "ctx:arg:a + ctx:arg:b + ctx:arg:c");
    sys.set_script_resolver([&](std::string_view) -> const ds::script_container* { return &s3; });
    const auto s2 = sys.parse<double, void>("s2", "{ ctx:arg:a + ctx:arg:b, execute = { s3, a = ctx:arg:c, b = 1.0, c = 1.0 } }");
    sys.set_script_resolver([&](std::string_view) -> const ds::script_container* { return &s2; });
    CHECK(s2.max_args == 6);
    CHECK_THROWS_WITH(((void)sys.parse<double, void>("s1", "{ ctx:arg:a + ctx:arg:b + ctx:arg:c, execute = { s2, a = 1.0, b = 1.0, c = 1.0 } }")),
                      doctest::Contains("argument slots"));
  }
}

TEST_CASE("audit2 #30: describe() stops at a failing command") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    const auto script = sys.parse<double, void>("s", "{ 2.0 * boom, idi(3) * 1.5 }");
    ds::context ctx;
    std::vector<std::string> failed;
    // Used to go on without boom's result: the `*` read a slot below the stack (unsafe), and the
    // second row was described from the wrong values.
    script.describe(&ctx, [&](const ds::container::description_entry& e) {
      if (!e.error.empty()) failed.emplace_back(e.name);
    });
    CHECK(std::find(failed.begin(), failed.end(), "boom") != failed.end());
    CHECK(std::find(failed.begin(), failed.end(), "idi") != failed.end());   // after the failure: not evaluated
  }
}

TEST_CASE("audit2 #32 #33: ignored conditions and grouped operands are rejected") {
  auto sys = make(configs[0]);
  // #32: nothing evaluated these conditions: 30 and 31.
  CHECK_THROWS_WITH(((void)sys.parse<double, void>("s", "max(1, { condition = false, 30 })")), doctest::Contains("never evaluated"));
  CHECK_THROWS_WITH(((void)sys.parse<double, void>("s", "1 + { condition = false, 30 }")), doctest::Contains("never evaluated"));
  // Where a condition does mean something, it still works.
  CHECK(run<double>(sys, "ADD = { { condition = false, 30 }, 1 }") == 1.0);
  CHECK(run<double>(sys, "max(1, { select = { { condition = true, 30 }, { 2 } } })") == 30.0);

  // #33: groups used to be spliced into the parent: (1, 2) * 3 == 6, () + 1 == 1, [1, 2] + 1 == 4.
  for (const auto source : { "(1, 2) * 3", "() + 1", "[1, 2] + 1", "max((1, 5), 2)" }) {
    CAPTURE(source);
    CHECK_THROWS_WITH(((void)sys.parse<double, void>("s", source)), doctest::Contains("must hold exactly one value"));
  }
  CHECK(run<double>(sys, "(1 + 2) * 3") == 9.0);
  CHECK(run<double>(sys, "max((1 + 5), 2)") == 6.0);
}

TEST_CASE("audit2 #34: a named slot or a list keeps a consistent type") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    // Retyping the slot used to compile and then fail the earlier read at run time.
    CHECK_THROWS_WITH(((void)sys.parse<int64_t, void>("s", "{ ctx_save = { o = 1 }, ctx:saved:o, ctx_save = { o = 2.5 } }")),
                      doctest::Contains("already holds"));
    CHECK_THROWS_WITH(((void)sys.parse<double, void>("s", "{ ctx_set = { a = true }, ctx_set = { a = 2.5 }, 1.0 }")), doctest::Contains("already holds"));
    // A value that converts implicitly is stored as the slot's type.
    CHECK(run<double>(sys, "{ ctx_save = { o = 1.5 }, ctx_save = { o = 2 }, ctx:saved:o }") == 2.0);

    // add_to then map: the add_to check used the list's final (mapped) type and rejected a valid script.
    big second{ { 0, 0, 20, 0 }, nullptr };
    big root{ { 0, 0, 1, 0 }, &second };
    const auto script = sys.parse<double, big*>("s",
      "{ ctx:list:k = { add_to = this, add_to = other }, ctx:list:k = { map = { big_id * 1.5 }, sum = this } }");
    ds::context ctx;
    ctx.create_lists(&script);
    ctx.set_arg(0, &root);
    script.process(&ctx);
    CHECK(ctx.get_return<double>() == 31.5);
  }
}

TEST_CASE("audit2 #37: ?= on a scope path guards the path") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    big second{ { 0, 0, 20, 0 }, nullptr };
    big root{ { 0, 0, 1, 0 }, &second };
    for (const auto& [source, expected] : {
      std::pair{ "{ other.other ?= { big_id }, 5 }", 5 },    // used to run big_id on a null scope
      std::pair{ "{ other ?= { big_id }, 5 }", 25 },
      std::pair{ "{ this.other ?= { big_id }, 5 }", 25 },
    }) {
      CAPTURE(source);
      const auto script = sys.parse<int64_t, big*>("s", source);
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(ctx.get_return<int64_t>() == expected);
    }
  }
}

TEST_CASE("audit2 #40: const-qualified types are the same stack types") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    big root{ { 0, 0, 7, 0 }, nullptr };
    {
      // A `const big*` value used to carry a `const` type tag, so the saved-value check failed.
      const auto script = sys.parse<int64_t, big*>("s", "{ ctx_save = { x = self_const }, ctx:saved:x = { big_id } }");
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(ctx.get_return<int64_t>() == 7);
    }
    {
      const auto script = sys.parse<int64_t, big*>("s", "big_id");
      ds::context ctx;
      ctx.set_arg(0, static_cast<const big*>(&root));   // used to fail the argument type check
      script.process(&ctx);
      CHECK(ctx.get_return<int64_t>() == 7);
    }
    {
      const auto script = sys.parse<big*, big*>("s", "self_const");
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(ctx.safe_get_return<big*>() == &root);
    }
    // `const bool` used to be typed as an integer.
    CHECK(run<bool>(sys, "const_truth"));
  }
}

namespace {
int64_t use_big(big* b) { return b->a[2]; }
int64_t use_item(item i) { return i.id * 100; }
}

TEST_CASE("audit2 #41: an explicit HT=void keeps the first parameter in the signature") {
  auto sys = make(configs[0]);
  // Both used to be listed with an empty signature (the first parameter taken for a scope), so the
  // second registration was rejected as a duplicate.
  sys.register_function<&use_big, void>("use_x");
  CHECK_NOTHROW(sys.register_function<&use_item, void>("use_x"));
  big root{ { 0, 0, 7, 0 }, nullptr };
  const auto script = sys.parse<int64_t, big*>("s", "use_x = this");
  ds::context ctx;
  ctx.set_arg(0, &root);
  script.process(&ctx);
  CHECK(ctx.get_return<int64_t>() == 7);
}

namespace {
// A member-function scope is the object itself, stored by value on the stack.
struct holder {
  int64_t base;
  bool valid() const { return true; }
  int64_t each(const std::function<int64_t(item)>& value) const { int64_t sum = 0; for (int64_t i = 1; i <= 3; ++i) sum += value(item{ i }); return sum + base; }
};
holder get_holder() { return holder{ 100 }; }
int64_t item_id(item i) { return i.id; }
}

TEST_CASE("audit2 #43: an iterator can be a member function") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    auto sys = make(cfg);
    sys.register_function<&get_holder>("holder");
    sys.register_function<&item_id>("item_id");
    sys.register_function_iter<&holder::each>("each", { "value" });   // used not to compile (tuple_cat of a non-tuple)
    CHECK(run<int64_t>(sys, "holder = { each = { value = item_id } }") == 106);
  }
}

TEST_CASE("audit2 #46: expression depth is bounded") {
  auto sys = make(configs[0]);
  const auto chain = [](const char* op, const size_t n) {
    std::string s = "1";
    for (size_t i = 0; i < n; ++i) { s += ' '; s += op; s += " 1"; }
    return s;
  };
  // Used to overflow the C++ stack (prefix recursion; the deep tree of a long operator chain).
  std::string prefix;
  for (size_t i = 0; i < 20000; ++i) prefix += "- ";
  prefix += "1";
  CHECK_THROWS_WITH(((void)sys.parse<int64_t, void>("s", prefix)), doctest::Contains("nests deeper than"));
  CHECK_THROWS_WITH(((void)sys.parse<int64_t, void>("s", chain("+", 20000))), doctest::Contains("nests deeper than"));
  CHECK(run<int64_t>(sys, chain("+", 200).c_str()) == 201);
}

TEST_CASE("audit2 #39: empty blocks and synthetic block names") {
  auto sys = make(configs[0]);
  // An empty effect script does nothing; it used to be "Could not find function '__effect_block'".
  for (const auto source : { "", "// only a comment", "{}", "{ tick, {} }" }) {
    CAPTURE(source);
    ticks = 0;
    CHECK_NOTHROW(run<void>(sys, source));
  }
  // An empty string script used to return the string "__string_block".
  CHECK_THROWS(((void)sys.parse<std::string_view, void>("s", "")));
  CHECK_THROWS(((void)sys.parse<std::string_view, void>("s", "__string_block")));
  CHECK_THROWS(((void)sys.parse<std::string_view, void>("s", "__empty_lvalue")));
  {
    // A quoted string stays a string. (The result views the container's pool: keep it alive.)
    const auto quoted = sys.parse<std::string_view, void>("s", "\"__string_block\"");
    ds::context ctx;
    quoted.process(&ctx);
    CHECK(ctx.get_return<std::string_view>() == "__string_block");
  }
}

TEST_CASE("audit2 #44: integer variants of max/min/abs, sequence and list reducers") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    // Through double, 2^53 + 1 rounded down.
    CHECK(run<int64_t>(sys, "max(9007199254740993, 0)") == 9007199254740993LL);
    CHECK(run<int64_t>(sys, "min(-9007199254740993, 0)") == -9007199254740993LL);
    CHECK(run<int64_t>(sys, "abs(-9007199254740993)") == 9007199254740993LL);
    CHECK(run<double>(sys, "max(1, 2.5)") == 2.5);
    // An integer sequence used to be rejected (it always accumulated a double).
    CHECK(run<int64_t>(sys, "sequence = { { condition = true, 5 }, { condition = true, 10 }, { condition = false, 15 } }") == 15);
    // Reducers over an integer list used to reject the integer callback result.
    const auto script = sys.parse<double, void>("s",
      "{ ctx:list:xs = { add_to = 1, add_to = 2, add_to = 6 }, ctx:list:xs = { sum = this }, ctx:list:xs = { max = this, default = { 0.0 } } }");
    ds::context ctx;
    ctx.create_lists(&script);
    script.process(&ctx);
    CHECK(ctx.get_return<double>() == 15.0);
  }
}

TEST_CASE("audit2 #42: a scoped function on an untyped argument") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    big who{ { 0, 0, 42, 0 }, nullptr };
    for (const auto source : { "ctx:arg:who = { big_id }", "ctx:arg:who.big_id" }) {
      CAPTURE(source);
      // Used to be "Could not find function 'big_id' for scope type 'view'".
      const auto script = sys.parse<int64_t, void>("s", source);
      REQUIRE(script.find_arg("who") < script.args.size());
      CHECK(script.args[script.find_arg("who")].type == ds::utils::type_name<big*>());   // the type is committed
      ds::context ctx;
      ctx.set_arg(script.find_arg("who"), &who);
      script.process(&ctx);
      CHECK(ctx.get_return<int64_t>() == 42);
      if (cfg.safe) {
        ds::context wrong;
        wrong.set_arg(script.find_arg("who"), 1.5);
        CHECK_THROWS(script.process(&wrong));
      }
    }
  }
}

namespace {
double opt2(double a, std::optional<double> b) { return b ? a + *b : -a; }
double opt_first(std::optional<double> a) { return a ? *a : -1.0; }
}

TEST_CASE("audit2 #38: std::optional parameters") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    sys.register_function<&opt2>("opt2");
    sys.register_function<&opt2>("opt2n", { "a", "b" });
    sys.register_function<&opt_first>("opt_first");
    // Every form used to fail: the value was checked against `std::optional<double>`, an absent
    // argument pushed nothing, a missing named one was an error, and an optional first parameter
    // was taken for a scope.
    CHECK(run<double>(sys, "opt2(1.0, 2.0)") == 3.0);
    CHECK(run<double>(sys, "opt2(1.0, 2)") == 3.0);   // integer -> double into the optional
    CHECK(run<double>(sys, "opt2(1.5)") == -1.5);
    CHECK(run<double>(sys, "opt2n = { a = 1.0, b = 4.0 }") == 5.0);
    CHECK(run<double>(sys, "opt2n = { a = 1.0 }") == -1.0);
    CHECK(run<double>(sys, "opt_first(2.5)") == 2.5);
    CHECK(run<double>(sys, "opt_first") == -1.0);
  }
}

TEST_CASE("audit2 #36: f(...) and f{...} compile like f = {...}") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    // A scope path callee used to stay one unknown name; `=` rows in braces were rejected.
    for (const auto source : { "other:add_to_big = 5", "other:add_to_big(5)", "other:add_to_big{ 5 }", "other{ add_to_big = 5 }", "other = { add_to_big = 5 }" }) {
      CAPTURE(source);
      big second{ { 0, 0, 0, 0 }, nullptr };
      big root{ { 0, 0, 0, 0 }, &second };
      const auto script = sys.parse<void, big*>("s", source);
      ds::context ctx;
      ctx.set_arg(0, &root);
      script.process(&ctx);
      CHECK(second.a[0] == 5);
    }
    CHECK_THROWS_WITH(((void)sys.parse<double, void>("s", "max(1)(2)")), doctest::Contains("needs a function name"));
  }
}

namespace {
enum class shade : int64_t { light = 1, dark = 2 };
std::optional<shade> parse_shade(const std::string_view name) {
  if (name == "light") return shade::light;
  if (name == "dark") return shade::dark;
  return std::nullopt;
}
}

TEST_CASE("audit2 P3: enum results, NaN in min/max, malformed paths, type names in messages") {
  auto sys = make(configs[0]);
  sys.register_enum<shade>(&parse_shade);

  // An enum script result used to be readable only as an integer.
  const auto script = sys.parse<shade, void>("s", "dark");
  ds::context ctx;
  script.process(&ctx);
  CHECK(ctx.is_return<shade>());
  CHECK(ctx.get_return<shade>() == shade::dark);
  CHECK(ctx.safe_get_return<shade>() == shade::dark);

  // max(NaN, 1) was NaN and max(1, NaN) was 1.
  CHECK(std::isnan(run<double>(sys, "max(0.0 / 0.0, 1.0)")));
  CHECK(std::isnan(run<double>(sys, "max(1.0, 0.0 / 0.0)")));
  CHECK(std::isnan(run<double>(sys, "min(1.0, 0.0 / 0.0)")));

  for (const auto source : { "other.big_id.", "other:big_id:", "other..big_id", ":big_id" }) {
    CAPTURE(source);
    CHECK_THROWS(((void)sys.parse<int64_t, big*>("s", source)));
  }

  const ds::any_stack value(int64_t(3));
  CHECK_THROWS_WITH((void)value.get<double>(), doctest::Contains(std::string(ds::utils::type_name<double>())));
}

TEST_CASE("audit2: an object block whose last child is skipped still leaves an operand") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    CHECK(run<bool>(sys, "(not (true) == { { condition = true, false } })"));
    // No child fires: the block leaves `false`, like a skipped `?=` in a bool context. It used to
    // leave `==` one operand short - an out-of-bounds read in unsafe mode.
    CHECK(run<bool>(sys, "(not (true) == { { condition = false, true } })"));
  }
}

TEST_CASE("audit2 #24: an integer expression in a floating-point block is computed in integers") {
  for (const auto cfg : configs) {
    CAPTURE(cfg.safe);
    CAPTURE(cfg.optimize);
    auto sys = make(cfg);
    // Arguments convert as late as possible, so unfolded code agrees with the constant folder: the
    // integer `-` and `+` are chosen and only the result becomes a double.
    CHECK(run<double>(sys, "9007199254740993 + -idi(7)") == run<double>(sys, "9007199254740993 + -7"));
    CHECK(run<double>(sys, "idi(9007199254740993) - 7") == 9007199254740986.0);
    const auto rem = sys.parse<double, void>("s", "idi(2) % 0");
    ds::context ctx;
    CHECK_THROWS_WITH(rem.process(&ctx), doctest::Contains("Integer remainder by zero"));   // not fmod's NaN
  }
}

TEST_CASE("audit2 #45: chance is fixed per script and occurrence, varied by the context state") {
  auto sys = make(configs[0]);
  const auto roll = [&](const ds::container& script, const uint64_t frame) {
    ds::context ctx;
    ctx.prng_state = frame;   // the external, mutable part: e.g. the frame index
    script.process(&ctx);
    return ctx.get_return<double>();
  };
  const auto loot = sys.parse<double, void>("loot", "chance");
  const auto plague = sys.parse<double, void>("plague", "chance");
  const auto loot_again = sys.parse<double, void>("loot", "chance");
  // Same script, same frame: the same roll, also when parsed again.
  CHECK(roll(loot, 7) == roll(loot, 7));
  CHECK(roll(loot, 7) == roll(loot_again, 7));
  // Another frame rolls again.
  CHECK(roll(loot, 7) != roll(loot, 8));
  // Another script rolls on its own; both used to draw identical states from the system seed.
  CHECK(roll(loot, 7) != roll(plague, 7));
}
