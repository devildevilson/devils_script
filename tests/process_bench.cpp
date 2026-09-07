// Process/codegen audit: timings include current_index reset and a volatile result sink.
// "simplified" cases are hand-written equivalents, not an optimizer implemented by this target.
#include "devils_script/system.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace ds = devils_script;
static double input_value = 7.0;
static double x() { return input_value; }
static bool predicate() { return input_value > 0.0; }
static volatile double sink = 0.0;

template <bool Safe>
static int64_t add_immediate(int64_t arg, ds::context* ctx, const ds::script_container*) {
  const double value = Safe ? ctx->stack.safe_pop<double>() : ctx->stack.pop<double>();
  ctx->stack.push(value + std::bit_cast<double>(arg));
  return 0;
}

struct sample {
  std::string name;
  std::string source;
  ds::container script;
  ds::context ctx;
  std::vector<double> timings;
};

template <typename R>
void add(std::vector<sample>& samples, const ds::system& sys, const char* name, const char* source) {
  samples.push_back({name, source, sys.parse<R, void>(name, source), {}, {}});
}

int main(int argc, char** argv) {
  const bool dump = argc > 1 && std::string_view(argv[1]) == "--disassemble";
  for (const bool unsafe : {false, true}) {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&x>("x");
    sys.register_function<&predicate>("predicate");
    if (unsafe) sys.toggle_safety();
    std::vector<sample> samples;
    add<double>(samples, sys, "literal double", "10.0");
    add<double>(samples, sys, "constant double sum", "5.0 + 5.0");
    add<int64_t>(samples, sys, "constant int sum", "5 + 5");
    add<int64_t>(samples, sys, "constant int simplified", "10");
    add<double>(samples, sys, "constant max", "max(3.0, 5.0)");
    add<double>(samples, sys, "constant max simplified", "5.0");
    add<double>(samples, sys, "dynamic x", "x");
    add<double>(samples, sys, "x plus constant subtree", "x + (2.0 * 3.0)");
    add<double>(samples, sys, "x plus folded subtree", "x + 6.0");
    add<double>(samples, sys, "x plus zero", "x + 0.0");
    add<double>(samples, sys, "three dynamic adds", "x + x + x");
    add<double>(samples, sys, "two ctx args", "ctx:arg:a + ctx:arg:b");
    add<bool>(samples, sys, "single predicate", "predicate");
    add<bool>(samples, sys, "two predicates", "{ predicate, predicate }");
    add<bool>(samples, sys, "false and predicate", "{ false, predicate }");
    add<bool>(samples, sys, "false simplified", "false");
    add<double>(samples, sys, "constant select", "{ select = { { condition = false, 10 }, { condition = true, 20 }, { 100 } } }");
    add<double>(samples, sys, "constant select simplified", "20.0");
    add<double>(samples, sys, "constant sequence", "{ sequence = { { condition = true, 5 }, { condition = true, 10 }, { condition = false, 15 } } }");
    add<double>(samples, sys, "constant sequence simplified", "15.0");

    // The unoptimized baselines: same sources, compiled with the peephole turned off. Every sample
    // above goes through it, so these are what the pass is actually measured against.
    ds::system plain;
    plain.init_basic_functions();
    plain.init_math();
    plain.register_function<&x>("x");
    plain.register_function<&predicate>("predicate");
    if (unsafe) plain.toggle_safety();
    plain.toggle_optimizations();
    add<double>(samples, plain, "two ctx args unoptimized", "ctx:arg:a + ctx:arg:b");
    add<double>(samples, plain, "three dynamic adds unoptimized", "x + x + x");
    add<double>(samples, plain, "constant select unoptimized", "{ select = { { condition = false, 10 }, { condition = true, 20 }, { 100 } } }");

    // Bounded experiment on a known straight-line stream. Runtime-only prototype: fusing a call with
    // its immediate operand needs a fused handler per registered operation, which the pass does not
    // generate. Kept as the measurement that would justify adding one.
    add<double>(samples, sys, "add immediate prototype", "x + 6.0");
    {
      auto& c = samples.back().script;
      if (c.cmds.size() != 4 || c.cmds[1].fp != &ds::pushvalue)
        throw std::runtime_error("addition codegen changed; review prototype");
      const auto add = ds::container::command(unsafe ? &add_immediate<false> : &add_immediate<true>, c.cmds[1].arg);
      c.cmds = {c.cmds[0], add, c.cmds[3]};
      c.locs = {c.locs[0], c.locs[2], c.locs[3]};
      c.cmd_node = {c.cmd_node[0], c.cmd_node[2], c.cmd_node[3]};
    }

    for (auto& s : samples) {
      for (size_t i = 0; i < s.script.args.size(); ++i) s.ctx.set_arg(i, double(i + 2));
      for (size_t i = 0; i < 1000; ++i) {
        s.ctx.current_index = 0;
        s.script.process(&s.ctx);
      }
      if (s.name == "two ctx args" && s.ctx.get_return<double>() != 5.0)
        throw std::runtime_error("direct argument reads produced a wrong result");
      if (s.name == "two ctx args unoptimized" && s.ctx.get_return<double>() != 5.0)
        throw std::runtime_error("unoptimized argument reads produced a wrong result");
      if (s.name == "add immediate prototype" && s.ctx.get_return<double>() != 13.0)
        throw std::runtime_error("immediate addition prototype produced a wrong result");
    }
    constexpr size_t iterations = 200000;
    constexpr size_t rounds = 7;
    // Rotate scenario order between rounds to reduce fixed-order frequency/thermal bias.
    for (size_t round = 0; round < rounds; ++round) {
      for (size_t offset = 0; offset < samples.size(); ++offset) {
        auto& s = samples[(offset + round * 3) % samples.size()];
        const bool integer = ds::type_is_integral(s.script.return_type);
        const bool boolean = ds::type_is_bool(s.script.return_type);
        const auto start = std::chrono::steady_clock::now();
        if (boolean) {
          for (size_t i = 0; i < iterations; ++i) {
            s.ctx.current_index = 0; s.script.process(&s.ctx);
            sink = sink + double(s.ctx.get_return<bool>());
          }
        } else if (integer) {
          for (size_t i = 0; i < iterations; ++i) {
            s.ctx.current_index = 0; s.script.process(&s.ctx);
            sink = sink + double(s.ctx.get_return<int64_t>());
          }
        } else {
          for (size_t i = 0; i < iterations; ++i) {
            s.ctx.current_index = 0; s.script.process(&s.ctx);
            sink = sink + s.ctx.get_return<double>();
          }
        }
        const auto end = std::chrono::steady_clock::now();
        s.timings.push_back(std::chrono::duration<double, std::nano>(end - start).count() / iterations);
      }
    }
    std::printf("== %s; %zu rounds x %zu iterations ==\n", unsafe ? "unsafe" : "safe", rounds, iterations);
    for (auto& s : samples) {
      std::sort(s.timings.begin(), s.timings.end());
      std::printf("%-30s %3zu cmds %8.2f ns/op [%8.2f, %8.2f]\n", s.name.c_str(), s.script.cmds.size(), s.timings[rounds/2], s.timings.front(), s.timings.back());
      if (dump && !unsafe) std::printf("source: %s\n%s\n", s.source.c_str(), ds::disassemble(s.script).c_str());
    }
  }
}
