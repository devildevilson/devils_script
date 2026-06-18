// Minimal standalone benchmark (no test framework).
// Replaces the former Catch2 benchmark harness.
#include "devils_script/system.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <functional>
#include <vector>

template <typename T>
struct handle { // sizeof(handle) <= 16
  T* ptr;
  size_t type;

  T& operator*() const { return *ptr; } // std invoke
  bool valid() const { return ptr != nullptr; } // check validity
};

struct country;
struct person;

struct city {
  std::string name;
  int population;
  double wealth;

  country* owner;
  std::vector<handle<person>> notable_people;
};

struct country {
  std::string name;
  int population;
  double gdp;

  handle<person> cur_leader;
  std::vector<city*> cities;

  int get_population() const { return population; }
  double get_gdp() const { return gdp; }

  void add_population(const int people) { population += people; }
  void add_gdp(const double val) { gdp += val; }

  handle<person> leader() const { return cur_leader; }
};

struct person {
  std::string name;
  uint16_t age;
  int charisma;

  struct country* current_country;
  struct city* live_in;

  void inc_age() { ++age; }
  void add_charisma(int c) { charisma += c; }
  struct country* country() const { return current_country; }
  struct city* living_in() const { return live_in; }
};

static int city_population(const city* c) { return c->population; }
static double city_wealth(const city* c) { return c->wealth; }
static country* city_owner(const city* c) { return c->owner; }
static size_t city_notable_people_count(const city* c) { return c->notable_people.size(); }

static void add_city_population(city* c, int pop) { c->population += pop; }
static void add_city_wealth(city* c, double w) { c->wealth += w; }

static uint16_t person_age(handle<person> p) { return (*p).age; }
static int person_charisma(handle<person> p) { return (*p).charisma; }

// iterator
static double each_city(country* c, const std::function<double(city*)>& fn) {
  double val = 0.0;
  for (auto city : c->cities) {
    val += fn(city);
  }
  return val;
}

static double each_notable_person(city* c, const std::function<bool(handle<person>)>& filter, const std::function<double(handle<person>)>& fn) {
  double val = 0.0;
  for (const auto& p : c->notable_people) {
    if (filter && !filter(p)) continue;
    val += fn(p);
  }
  return val;
}

const std::string scripts[] = {
  "country.country_leader:age",
  "country = { each_city = { value = city_population } }",
  "country = { each_city = { value = { each_notable_person = { value = charisma } } } }",
  "this:living_in = { ctx_save = { val1 = { each_notable_person = { value = age } } }, notable_people_count / ctx:saved:val1 }",
  "{ ctx_save = { cur_player = this }, this:living_in = { each_notable_person = { filter = this != ctx:saved:cur_player, value = age } } }",
  // ???
};

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE::DEVILS_SCRIPT_INNER_NAMESPACE;
#else
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE;
#endif


// keep the optimizer from discarding the measured work
static volatile double g_sink = 0.0;

template <typename F>
static void bench(const char* name, const size_t iters, F&& f) {
  using clock = std::chrono::steady_clock;
  // warmup
  for (size_t i = 0; i < 16; ++i) g_sink += f();

  const auto start = clock::now();
  for (size_t i = 0; i < iters; ++i) g_sink += f();
  const auto end = clock::now();

  const double total_ns = std::chrono::duration<double, std::nano>(end - start).count();
  std::printf("%-24s %10.1f ns/op  (%zu iters)\n", name, total_ns / double(iters), iters);
}

int main() {
  person p1{ "Mary", 20, 5, nullptr, nullptr };
  person p2{ "Alaska", 13, 2, nullptr, nullptr };
  person p3{ "Alexey", 26, 7, nullptr, nullptr };
  person p4{ "John", 10, 1, nullptr, nullptr };
  person p5{ "Sor", 24, 3, nullptr, nullptr };

  handle<person> p1h{ &p1, 123 };
  handle<person> p2h{ &p2, 123 };
  handle<person> p3h{ &p3, 123 };
  handle<person> p4h{ &p4, 123 };
  handle<person> p5h{ &p5, 123 };

  city c1{ "Moscow", 100, 20.0, nullptr, { p1h, p2h } };
  city c2{ "Tokyo", 167, 36.0, nullptr, { p3h, p4h } };
  city c3{ "London", 76, 17.0, nullptr, { p5h } };

  country co1{ "world", 1723, 4072.0, p3h, { &c1, &c2, &c3 } };

  c1.owner = &co1;
  c2.owner = &co1;
  c3.owner = &co1;
  p1h.ptr->live_in = &c1;
  p2h.ptr->live_in = &c1;
  p3h.ptr->live_in = &c2;
  p4h.ptr->live_in = &c2;
  p5h.ptr->live_in = &c3;
  p1h.ptr->current_country = &co1;
  p2h.ptr->current_country = &co1;
  p3h.ptr->current_country = &co1;
  p4h.ptr->current_country = &co1;
  p5h.ptr->current_country = &co1;

  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  sys.register_function<&country::get_population>("country_population");
  sys.register_function<&country::get_gdp>("country_gdp");
  sys.register_function<&country::add_population>("country_add_population");
  sys.register_function<&country::add_gdp>("country_add_gdp");
  sys.register_function<&country::leader>("country_leader");
  sys.register_function<&city_population>("city_population");
  sys.register_function<&city_wealth>("city_wealth");
  sys.register_function<&city_owner>("city_owner");
  sys.register_function<&city_notable_people_count>("notable_people_count");
  sys.register_function<&add_city_population>("city_add_population");
  sys.register_function<&add_city_wealth>("city_add_wealth");
  sys.register_function<&person_age>("age");
  sys.register_function<&person_charisma>("charisma");
  sys.register_function<&person::inc_age, handle<person>>("inc_age");
  sys.register_function<&person::add_charisma, handle<person>>("add_charisma");
  sys.register_function<&person::country, handle<person>>("country");
  sys.register_function<&person::living_in, handle<person>>("living_in");
  sys.register_function_iter<&each_city>("each_city", { "value" });
  sys.register_function_iter<&each_notable_person>("each_notable_person", { "filter", "value" });

  constexpr size_t script_count = sizeof(scripts) / sizeof(scripts[0]);

  std::printf("== parse ==\n");
  for (size_t i = 0; i < script_count; ++i) {
    char name[32];
    std::snprintf(name, sizeof(name), "script%zu parse", i + 1);
    bench(name, 10000, [&] {
      const auto cont = sys.parse<double, handle<person>>(scripts[i]);
      return double(cont.cmds.size());
    });
  }

  std::printf("== execution ==\n");
  for (size_t i = 0; i < script_count; ++i) {
    const auto cont = sys.parse<double, handle<person>>(scripts[i]);
    ds::context ctx;
    ctx.set_arg(0, p1h); // set root
    ctx.create_lists(&cont);

    char name[32];
    std::snprintf(name, sizeof(name), "script%zu execution", i + 1);
    bench(name, 100000, [&] {
      ctx.clear();
      cont.process(&ctx);
      return ctx.get_return<double>();
    });
  }

  std::printf("== description ==\n");
  for (size_t i = 0; i < script_count; ++i) {
    const auto cont = sys.parse<double, handle<person>>(scripts[i]);
    ds::context ctx;
    ctx.set_arg(0, p1h);
    ctx.create_lists(&cont);

    char name[32];
    std::snprintf(name, sizeof(name), "script%zu describe", i + 1);
    bench(name, 100000, [&] {
      ctx.clear();
      size_t count = 0;
      cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
        count += 1;
        if (entry.state == ds::container::description_value_state::value && entry.value.is<double>()) {
          g_sink += entry.value.get<double>() * 0.0;
        }
      });
      return double(count);
    });
  }

  return 0;
}
