// Standalone benchmark for script-in-script (`execute`) calls (no test framework).
// Each scenario pairs an INLINE baseline against an EXECUTE version of the same work, so the
// difference is the call-frame save/restore + frame_base overhead the `execute` opcode adds.
#include "devils_script/system.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T>
struct handle { // sizeof(handle) <= 16
  T* ptr;
  size_t type;
  T& operator*() const { return *ptr; }
  bool valid() const { return ptr != nullptr; }
};

struct person {
  uint16_t age;
  int charisma;
  void add_charisma(int c) { charisma += c; }
};

static uint16_t person_age(handle<person> p) { return (*p).age; }

namespace ds = devils_script;

// A name -> compiled sub-script registry wired into the system's script resolver.
struct registry {
  std::unordered_map<std::string, const ds::script_container*> table;
  std::vector<ds::container> storage;   // keeps the compiled sub-scripts alive
  void install(ds::system& sys) {
    sys.set_script_resolver([this](std::string_view name) -> const ds::script_container* {
      const auto it = table.find(std::string(name));
      return it == table.end() ? nullptr : it->second;
    });
  }
};

// keep the optimizer from discarding the measured work
static volatile double g_sink = 0.0;

template <typename F>
static void bench(const char* name, const size_t iters, F&& f) {
  using clock = std::chrono::steady_clock;
  for (size_t i = 0; i < 16; ++i) g_sink += f();   // warmup

  const auto start = clock::now();
  for (size_t i = 0; i < iters; ++i) g_sink += f();
  const auto end = clock::now();

  const double total_ns = std::chrono::duration<double, std::nano>(end - start).count();
  std::printf("%-30s %10.1f ns/op  (%zu iters)\n", name, total_ns / double(iters), iters);
}

int main() {
  person p{ 40, 7 };
  handle<person> ph{ &p, 0 };

  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  sys.register_function<&person_age>("age");
  sys.register_function<&person::add_charisma, handle<person>>("add_charisma");

  registry reg;
  reg.install(sys);
  auto add_sub = [&](const std::string& name, ds::container&& c) {
    reg.storage.push_back(std::move(c));
    reg.table[name] = &reg.storage.back();
  };

  // Sub-scripts the callers will invoke. NOTE: storage uses push_back, so the resolver hands out
  // pointers into a growing vector; compile every sub up front, then never touch `storage` again.
  reg.storage.reserve(16);
  add_sub("addup",    sys.parse<double, void>("addup", "ctx:arg:base + ctx:arg:bonus"));
  add_sub("agebonus", sys.parse<double, handle<person>>("agebonus", "age + ctx:arg:bonus"));
  add_sub("boost",    sys.parse<void, handle<person>>("boost", "add_charisma = ctx:arg:amount"));
  // A switch-based sub exercises packed two-index stack ops (cmpeq2/erase) under frame_base.
  // Literal discriminant (still compiled to cmpeq2/erase at runtime); the chosen branch returns a
  // runtime arg so nothing folds away at parse.
  add_sub("classify", sys.parse<double, void>(
    "classify", "{ switch = { value = 2, { value = 1, 10.0 }, { value = 2, ctx:arg:bump }, { value = 3, 30.0 } } }"));
  // Nesting chain: leaf <- mid <- top, so `top` runs two stacked frame_base offsets.
  add_sub("leaf", sys.parse<double, void>("leaf", "ctx:arg:x + 1.0"));
  add_sub("mid",  sys.parse<double, void>("mid",  "execute = { leaf, x = ctx:arg:x }"));
  add_sub("top",  sys.parse<double, void>("top",  "execute = { mid, x = ctx:arg:x }"));

  // ---- parse cost ----
  std::printf("== parse ==\n");
  bench("parse: inline a+b", 50000, [&] {
    const auto c = sys.parse<double, void>("s", "ctx:arg:a + ctx:arg:b");
    return double(c.cmds.size());
  });
  bench("parse: execute addup", 50000, [&] {
    const auto c = sys.parse<double, void>("s", "execute = { addup, base = ctx:arg:a, bonus = ctx:arg:b }");
    return double(c.cmds.size());
  });

  // ---- root-less call vs inline equivalent ----
  std::printf("== root-less: 1 call vs inline ==\n");
  {
    const auto inl = sys.parse<double, void>("inl", "ctx:arg:a + ctx:arg:b");
    const auto exe = sys.parse<double, void>("exe", "execute = { addup, base = ctx:arg:a, bonus = ctx:arg:b }");
    ds::context ctx; ctx.clear(); ctx.set_arg(0, 10.0); ctx.set_arg(1, 20.0);
    bench("inline a+b (baseline)", 200000, [&] { ctx.clear(); ctx.set_arg(0, 10.0); ctx.set_arg(1, 20.0); inl.process(&ctx); return ctx.get_return<double>(); });
    bench("execute addup (1 call)", 200000, [&] { ctx.clear(); ctx.set_arg(0, 10.0); ctx.set_arg(1, 20.0); exe.process(&ctx); return ctx.get_return<double>(); });
  }

  // ---- rooted call (implicit current-scope push) vs inline equivalent ----
  std::printf("== rooted: 1 call vs inline ==\n");
  {
    const auto inl = sys.parse<double, handle<person>>("inl", "age + ctx:arg:bonus");
    const auto exe = sys.parse<double, handle<person>>("exe", "execute = { agebonus, bonus = ctx:arg:bonus }");
    ds::context ctx; ctx.clear(); ctx.set_arg(0, ph); ctx.set_arg(1, 2.0);
    bench("inline rooted (baseline)", 200000, [&] { ctx.clear(); ctx.set_arg(0, ph); ctx.set_arg(1, 2.0); inl.process(&ctx); return ctx.get_return<double>(); });
    bench("execute rooted (1 call)", 200000, [&] { ctx.clear(); ctx.set_arg(0, ph); ctx.set_arg(1, 2.0); exe.process(&ctx); return ctx.get_return<double>(); });
  }

  // ---- nesting depth: per-level frame_base cost ----
  std::printf("== nesting depth ==\n");
  {
    const auto d1 = sys.parse<double, void>("d1", "execute = { leaf, x = ctx:arg:x }");
    const auto d3 = sys.parse<double, void>("d3", "execute = { top,  x = ctx:arg:x }");
    ds::context ctx; ctx.clear(); ctx.set_arg(0, 5.0);
    bench("execute depth 1", 200000, [&] { ctx.clear(); ctx.set_arg(0, 5.0); d1.process(&ctx); return ctx.get_return<double>(); });
    bench("execute depth 3 (nested)", 200000, [&] { ctx.clear(); ctx.set_arg(0, 5.0); d3.process(&ctx); return ctx.get_return<double>(); });
  }

  // ---- throughput: many calls in one script ----
  std::printf("== fan-out: many calls / script ==\n");
  {
    std::string many = "{ ";
    for (int i = 0; i < 8; ++i) many += "execute = { addup, base = ctx:arg:a, bonus = ctx:arg:b }, ";
    many += "0.0 }";   // block sums the 8 results
    const auto exe = sys.parse<double, void>("many", many);
    ds::context ctx; ctx.clear(); ctx.set_arg(0, 10.0); ctx.set_arg(1, 20.0);
    bench("8 executes in one script", 100000, [&] { ctx.clear(); ctx.set_arg(0, 10.0); ctx.set_arg(1, 20.0); exe.process(&ctx); return ctx.get_return<double>(); });
  }

  // ---- heavy stack arithmetic (switch) inline vs via execute ----
  std::printf("== heavy stack ops (switch) ==\n");
  {
    const auto inl = sys.parse<double, void>(
      "inl", "{ switch = { value = 2, { value = 1, 10.0 }, { value = 2, ctx:arg:bump }, { value = 3, 30.0 } } }");
    const auto exe = sys.parse<double, void>("exe", "execute = { classify, bump = ctx:arg:bump }");
    ds::context ctx; ctx.clear(); ctx.set_arg(0, 20.0);
    bench("inline switch (baseline)", 200000, [&] { ctx.clear(); ctx.set_arg(0, 20.0); inl.process(&ctx); return ctx.get_return<double>(); });
    bench("execute switch sub", 200000, [&] { ctx.clear(); ctx.set_arg(0, 20.0); exe.process(&ctx); return ctx.get_return<double>(); });
  }

  // ---- effect (void) sub-script call ----
  std::printf("== effect call ==\n");
  {
    const auto exe = sys.parse<void, handle<person>>("exe", "execute = { boost, amount = ctx:arg:amount }");
    ds::context ctx; ctx.clear(); ctx.set_arg(0, ph); ctx.set_arg(1, int64_t(1));
    bench("execute effect (void)", 200000, [&] { ctx.clear(); ctx.set_arg(0, ph); ctx.set_arg(1, int64_t(1)); exe.process(&ctx); return 0.0; });
  }

  return 0;
}
