#pragma once
#include <doctest/doctest.h>
#include <optional>
#include <span>
#include <utility>
#include <functional>
#include <tuple>
#include "devils_script/system.h"

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE::DEVILS_SCRIPT_INNER_NAMESPACE;
#else
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE;
#endif

inline double g(const double v1, const double v2, const double v3) noexcept { return v1 + v2 + v3; }
inline double f(const double v1, const double v2) noexcept { return v1 + v2; }
inline bool m(const bool v1, const bool v2) noexcept { return v1 && v2; }
struct scope1 { bool valid() const { return true; } };
struct scope2 { bool is_valid() const { return true; } };
struct scope3 { operator bool() const { return true; } };
inline scope2 func1(scope1, std::string_view) { return scope2{}; }
inline double func2(scope3, double, double) { return 1; }
inline double func3(double a, double b, double c) { return a + b + c; }
inline double func4([[maybe_unused]] const std::string_view& str) { return 1; }
inline double func7(scope2) { return 5; }
inline scope3 to_scope3(scope2) { return scope3{}; }
inline scope2 to_scope2(scope1) { return scope2{}; }
inline scope3 func8(scope2, std::string_view) { return scope3{}; }
inline double func9(scope1, double) { return 10; }
inline double overloaded_score(scope1) { return 1; }
inline double overloaded_score2(scope2) { return 2; }

struct object_ref {
  int64_t id;
  bool valid() const { return id != 0; }
};

struct checked_ref {
  int64_t id;
  bool valid() const { return id != 0; }
};

inline double checked_score(checked_ref cur) { return double(cur.id); }
inline bool checked_ref_is_even(const checked_ref& cur) { return cur.id % 2 == 0; }

inline object_ref liege(object_ref cur) { return object_ref{ cur.id + 10 }; }
inline object_ref even_child(object_ref) { return object_ref{ 2 }; }
inline object_ref first_child(object_ref cur) { return object_ref{ cur.id + 100 }; }
inline object_ref nemesis(object_ref cur) { return object_ref{ cur.id + 1000 }; }
inline bool is_married_to(object_ref cur, object_ref other) { return cur.id == 1 && other.id == 1111; }
inline bool object_id_is(object_ref cur, int64_t id) { return cur.id == id; }
inline bool object_arg_id_is(std::string_view, object_ref cur, int64_t id) { return cur.id == id; }
inline scope2 wrong_object(object_ref) { return scope2{}; }
inline void object_effect(object_ref) {}
inline double runtime_num() { return 3.0; }
inline int g_short_circuit_calls = 0;
inline bool runtime_true() { return true; }
inline bool runtime_false() { return false; }
inline bool counted_true() { g_short_circuit_calls += 1; return true; }
inline bool counted_false() { g_short_circuit_calls += 1; return false; }
enum class title_rank : int64_t {
  barony = 1,
  duchy = 2,
  kingdom = 3,
  empire = 4,
};

inline title_rank rank(object_ref cur) { return static_cast<title_rank>(cur.id); }
inline bool rank_is_at_least(object_ref cur, title_rank r) { return cur.id >= static_cast<int64_t>(r); }
inline double kingdom() { return 30.0; }
inline bool object_ref_is_eleven(object_ref cur) { return cur.id == 11; }
inline std::string_view object_ref_name(object_ref cur) { return cur.id == 11 ? "liege" : "fallback"; }
inline std::optional<title_rank> parse_title_rank(const std::string_view name) {
  if (name == "barony") return title_rank::barony;
  if (name == "duchy") return title_rank::duchy;
  if (name == "kingdom") return title_rank::kingdom;
  if (name == "empire") return title_rank::empire;
  return std::nullopt;
}

struct effect_stats {
  int calls = 0;
  std::string_view name;
  double ret = 0.0;
  double arg0 = 0.0;
  double arg1 = 0.0;
  int64_t scope_id = 0;
};

inline double effect_sum(double a, double b) { return a + b; }
inline void on_effect_sum(void* ptr, const std::string_view& name, const double& ret, const std::tuple<double, double>& args) {
  auto* stats = static_cast<effect_stats*>(ptr);
  stats->calls += 1;
  stats->name = name;
  stats->ret = ret;
  stats->arg0 = std::get<0>(args);
  stats->arg1 = std::get<1>(args);
}

inline void effect_touch(double) {}
inline void on_effect_touch(void* ptr, const std::string_view& name, const std::tuple<double>& args) {
  auto* stats = static_cast<effect_stats*>(ptr);
  stats->calls += 1;
  stats->name = name;
  stats->arg0 = std::get<0>(args);
}

inline double effect_object_score(object_ref scope, double bonus) { return double(scope.id) + bonus; }
inline void on_effect_object_score(void* ptr, const std::string_view& name, const double& ret, const std::tuple<object_ref, double>& args) {
  auto* stats = static_cast<effect_stats*>(ptr);
  stats->calls += 1;
  stats->name = name;
  stats->ret = ret;
  stats->scope_id = std::get<0>(args).id;
  stats->arg0 = std::get<1>(args);
}
inline double func6(scope2, const std::function<double(scope3)>& fn) { return fn(scope3{}); }
inline double func10(scope2, const std::function<double(scope2)>& fn1, const std::function<double(scope2)>& fn2) { return fn1(scope2{}) + fn2(scope2{}); }
inline double child_id(object_ref child) { return double(child.id); }
inline bool child_is_even(object_ref child) { return child.id % 2 == 0; }
inline double child_count_one(object_ref) { return 1.0; }
inline double child_count_two(object_ref) { return 2.0; }
inline int g_any_child_value_calls = 0;
inline bool counted_child_is_even(object_ref child) { g_any_child_value_calls += 1; return child.id % 2 == 0; }
inline bool any_child(
  object_ref root,
  ds::script_function<bool(object_ref)> value,
  ds::script_function<bool(object_ref)> filter,
  ds::script_function<double(object_ref)> count
) {
  if (!root.valid() || !value) return false;
  const size_t required = count ? size_t(count(root)) : 1;
  size_t successes = 0;
  const object_ref children[] = { object_ref{ 1 }, object_ref{ 2 }, object_ref{ 3 }, object_ref{ 4 } };
  for (const auto child : children) {
    if (filter && !filter(child)) continue;
    successes += size_t(value(child));
    if (successes >= required) return true;
  }
  return false;
}
inline double every_on_list(const ds::internal::thisctxlist &l, const std::function<double(scope2)>& fn) {
  double val = 0.0;
  for (size_t i = 0; i < l.ctx->lists[l.idx].size(); ++i) {
    val += fn(l.ctx->lists[l.idx][i].get<scope2>());
  }
  return val;
}
// Parses with a default (throwing) error callback so malformed scripts raise.
inline void parse_void_d(const std::string& script) {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  (void)sys.parse<double, void>("script", script);
}
