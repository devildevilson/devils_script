// Equivalence tests for the command peephole (system::optimize_commands).
//
// The pass may only change how many instructions run, never what a script computes or what
// describe() reports. Every script below is compiled twice - once with optimizations on, once with
// them off - and the two containers are compared on their result and on their full description
// output. The one deliberate difference is called out in `scopes_match`.
#include <doctest/doctest.h>
#include "devils_script/system.h"

#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace ds = devils_script;

namespace {

struct town {
  int size;
  double wealth;
};

struct realm {
  int treasury;
  std::vector<town*> towns;
};

int town_size(const town* t) { return t->size; }
bool town_rich(const town* t) { return t->wealth > 10.0; }
double town_wealth(const town* t) { return t->wealth; }
int realm_treasury(const realm* r) { return r->treasury; }
realm* town_realm(const town* t);

realm* the_realm = nullptr;
realm* town_realm(const town*) { return the_realm; }

double each_town(realm* r, const std::function<bool(town*)>& filter, const std::function<double(town*)>& fn) {
  double sum = 0.0;
  for (auto* t : r->towns) {
    if (filter && !filter(t)) continue;
    sum += fn(t);
  }
  return sum;
}

// One record per description node, in visit order.
struct node_line {
  std::string name;
  size_t nest;
  std::string value;
  std::string scope;
};

std::string render(const ds::any_stack& v) {
  if (v.type().empty()) return "-";
  std::string out(v.type());
  if (v.is<double>()) out += "=" + std::to_string(v.get<double>());
  else if (v.is<int64_t>()) out += "=" + std::to_string(v.get<int64_t>());
  else if (v.is<bool>()) out += v.get<bool>() ? "=true" : "=false";
  return out;
}

std::vector<node_line> describe_all(const ds::container& script, ds::context& ctx) {
  std::vector<node_line> out;
  script.describe(&ctx, [&](const ds::container::description_entry& e) {
    out.push_back(node_line{ std::string(e.name), e.nest_level, render(e.value), std::string(e.scope.type()) });
  });
  return out;
}

// The pass may change exactly one thing about a description, and only on a `ctx` node: that node is
// a namespace marker for `ctx:arg:` / `ctx:saved:` / `ctx:list:`, and the object it used to report as
// its value or scope was the internal context handle the read never looked at (or, once that handle
// is gone, whatever slot number collision left behind). Dropping the push leaves it with nothing to
// report, which is what it always meant. Every other node must match exactly.
std::string compare(const std::vector<node_line>& plain, const std::vector<node_line>& optimized) {
  if (plain.size() != optimized.size()) return "node count differs";
  for (size_t i = 0; i < plain.size(); ++i) {
    const auto& a = plain[i];
    const auto& b = optimized[i];
    if (a.name != b.name || a.nest != b.nest) return "node " + std::to_string(i) + ": tree differs at " + a.name;
    const bool ctx_node = a.name == "ctx";
    if (a.value != b.value && !(ctx_node && b.value == "-")) return "node " + std::to_string(i) + " (" + a.name + "): value " + a.value + " -> " + b.value;
    if (a.scope != b.scope && !(ctx_node && b.scope.empty())) {
      // A read under `ctx` names the vanished handle as its scope; it loses the scope, nothing else.
      if (!(b.scope.empty() && a.scope == ds::utils::type_name<ds::internal::thisctx>())) 
        return "node " + std::to_string(i) + " (" + a.name + "): scope " + a.scope + " -> " + b.scope;
    }
  }
  return {};
}

void build(ds::system& sys) {
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&town_size>("size");
  sys.register_function<&town_rich>("rich");
  sys.register_function<&town_wealth>("wealth");
  sys.register_function<&town_realm>("realm");
  sys.register_function<&realm_treasury>("treasury");
  sys.register_function_iter<&each_town>("each_town", { "filter", "value" });
}

const char* const scripts[] = {
  "size",
  "realm.treasury",
  "realm = { treasury }",
  "wealth + 1.0",
  "ctx:arg:a + ctx:arg:b",
  "ctx:arg:a * ctx:arg:a + ctx:arg:b",
  "{ ctx_save = { w = wealth }, ctx:saved:w + 1.0 }",
  "{ ctx_save = { w = wealth }, ctx:saved:w + ctx:saved:w }",
  "realm = { each_town = { value = wealth } }",
  "realm = { each_town = { filter = rich, value = wealth } }",
  "{ ctx_save = { limit = 10.0 }, realm = { each_town = { filter = rich, value = wealth + ctx:saved:limit } } }",
  "select = { { condition = false, wealth }, { condition = true, 1.0 }, { 0.0 } }",
  "sequence = { { condition = true, wealth }, { condition = false, 1.0 } }",
  "{ ctx:list:ws = { add_to = wealth, add_to = 2.0 }, ctx:list:ws = { sum = this } }",
  "{ ctx:list:ws = { add_to = wealth, add_to = 2.0 }, ctx:list:ws = { count } }",
  "{ ctx:list:ws = { add_to = wealth, add_to = 2.0 }, ctx:list:ws = { max = this, default = 0.0 } }",
  // A dead context push inside a list-pipeline callback: the pass must relocate the pipeline's
  // packed section bounds, which are stored relative to its own command.
  "{ ctx_save = { bonus = 1.0 }, ctx:list:ws = { add_to = wealth, add_to = 2.0 }, ctx:list:ws = { sum = { this + ctx:saved:bonus } } }",
  "realm = { each_town = { value = { select = { { condition = false, wealth }, { 0.0 } } } } }",
  "{ ctx_save = { total = { realm = { each_town = { value = wealth } } } }, ctx:saved:total / 2.0 }",
};

}  // namespace

TEST_CASE("optimizer: peephole preserves results and descriptions") {
  town t1{ 100, 20.0 };
  town t2{ 30, 7.0 };
  town t3{ 80, 12.0 };
  realm r{ 500, { &t1, &t2, &t3 } };
  the_realm = &r;

  ds::system optimized;
  build(optimized);
  ds::system plain;
  build(plain);
  plain.toggle_optimizations();
  REQUIRE(optimized.optimizations());
  REQUIRE_FALSE(plain.optimizations());

  size_t shortened = 0;
  for (const auto* source : scripts) {
    const std::string script_source = source;
    CAPTURE(script_source);
    const auto fast = optimized.parse<double, town*>("script", source);
    const auto slow = plain.parse<double, town*>("script", source);

    CHECK(fast.cmds.size() <= slow.cmds.size());
    CHECK(fast.locs.size() == fast.cmds.size());
    shortened += fast.cmds.size() < slow.cmds.size();

    for (const bool sized : { false, true }) {
      ds::context fast_ctx(sized ? fast.max_stack : 128, sized ? fast.max_saved : 32);
      ds::context slow_ctx(sized ? slow.max_stack : 128, sized ? slow.max_saved : 32);
      fast_ctx.create_lists(&fast);
      slow_ctx.create_lists(&slow);
      fast_ctx.set_arg(0, &t1);
      slow_ctx.set_arg(0, &t1);
      fast_ctx.set_arg(fast.find_arg("a"), 2.0);
      slow_ctx.set_arg(slow.find_arg("a"), 2.0);
      fast_ctx.set_arg(fast.find_arg("b"), 3.0);
      slow_ctx.set_arg(slow.find_arg("b"), 3.0);

      fast.process(&fast_ctx);
      slow.process(&slow_ctx);
      CHECK(fast_ctx.get_return<double>() == slow_ctx.get_return<double>());
      CHECK(fast_ctx.stack.size() == slow_ctx.stack.size());
      for (size_t i = 0; i < fast.lists.size() && i < slow.lists.size(); ++i)
        CHECK(fast_ctx.lists[i].size() == slow_ctx.lists[i].size());
    }

    ds::context fast_desc;
    ds::context slow_desc;
    fast_desc.create_lists(&fast);
    slow_desc.create_lists(&slow);
    fast_desc.set_arg(0, &t1);
    slow_desc.set_arg(0, &t1);
    const auto difference = compare(describe_all(slow, slow_desc), describe_all(fast, fast_desc));
    CHECK(difference == "");
  }

  // The corpus has to actually exercise the pass, or the checks above prove nothing.
  CHECK(shortened > 0);
}

TEST_CASE("optimizer: direct context reads lose the context push") {
  town t{ 100, 20.0 };
  realm r{ 500, { &t } };
  the_realm = &r;

  ds::system optimized;
  build(optimized);
  ds::system plain;
  build(plain);
  plain.toggle_optimizations();

  // `ctx:arg:a + ctx:arg:b` lowers to context/read/erase twice plus the call and the return; both
  // context pushes and both unwinds are dead because `pushargvalue` reads through `context*`.
  const auto fast = optimized.parse<double, void>("args", "ctx:arg:a + ctx:arg:b");
  const auto slow = plain.parse<double, void>("args", "ctx:arg:a + ctx:arg:b");
  CHECK(slow.cmds.size() == 8);
  CHECK(fast.cmds.size() == 4);
  CHECK(std::count_if(fast.cmds.begin(), fast.cmds.end(), [](const auto& c) {
    return ds::find_basicf_by_fp(c.fp) == ds::basicf::context;
  }) == 0);

  ds::context ctx;
  ctx.set_arg(fast.find_arg("a"), 2.0);
  ctx.set_arg(fast.find_arg("b"), 3.0);
  fast.process(&ctx);
  CHECK(ctx.get_return<double>() == 5.0);
}

TEST_CASE("optimizer: erase_range unwinds exactly like a run of erases") {
  ds::context::stack_t stack(16);
  for (int64_t i = 0; i < 6; ++i) stack.push(double(i));
  ds::context::stack_t reference(16);
  for (int64_t i = 0; i < 6; ++i) reference.push(double(i));

  // Descending single erases and one erase_range over the same slots agree.
  reference.erase(3);
  reference.erase(2);
  reference.erase(1);
  stack.erase_range(1, 3);
  REQUIRE(stack.size() == reference.size());
  for (int64_t i = 0; i < int64_t(stack.size()); ++i)
    CHECK(stack.safe_get<double>(i) == reference.safe_get<double>(i));

  // Out-of-range and empty requests leave the stack alone.
  const size_t before = stack.size();
  stack.erase_range(0, 0);
  stack.erase_range(int64_t(before), 1);
  stack.erase_range(-1, 5);
  CHECK(stack.size() == before);
  stack.erase_range(-1, 1);
  CHECK(stack.size() == before - 1);
}
