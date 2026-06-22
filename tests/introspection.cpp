// Golden tests for the human-readable description mode (container::describe).
// Locks the current partial-evaluation output so future stack/context
// refactors can be checked against a known-good baseline.
#include <doctest/doctest.h>
#include "devils_script/system.h"

#include <set>
#include <sstream>
#include <string>

template <typename T>
struct handle { // sizeof(handle) <= 16
  T* ptr;
  size_t type;

  T& operator*() const { return *ptr; }
  bool valid() const { return ptr != nullptr; }
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

static double each_city(country* c, const std::function<double(city*)>& fn) {
  double val = 0.0;
  for (auto city : c->cities) val += fn(city);
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

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE::DEVILS_SCRIPT_INNER_NAMESPACE;
#else
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE;
#endif

// Formats a single description node exactly like examples/desc.cpp prints it.
static bool collect(std::string& out, const std::string_view& name, const size_t nest, const ds::any_stack& value, const ds::any_stack& scope) {
  std::ostringstream ss;
  for (size_t i = 0; i < nest; ++i) ss << "  ";
  ss << "'" << name << "' value: '";
  if (value.is<double>()) ss << value.get<double>();
  else if (value.is<int64_t>()) ss << value.get<int64_t>();
  else if (value.is<bool>()) ss << value.get<bool>();
  else ss << (value.type().empty() ? "no value" : value.type());
  ss << "' scope: '" << (scope.type().empty() ? "no scope" : scope.type()) << "'\n";
  out += ss.str();
  return true;
}

static bool has_text(const std::string& str, const std::string_view text) {
  return str.find(text) != std::string::npos;
}

TEST_CASE("Description golden (partial evaluation)") {
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

  sys.register_function<&country::get_population>("population");
  sys.register_function<&country::get_gdp>("gdp");
  sys.register_function<&country::add_population>("add_population");
  sys.register_function<&country::add_gdp>("add_gdp");
  sys.register_function<&country::leader>("leader");
  sys.register_function<&city_population>("population");
  sys.register_function<&city_wealth>("wealth");
  sys.register_function<&city_owner>("owner");
  sys.register_function<&city_notable_people_count>("notable_people_count");
  sys.register_function<&add_city_population>("add_population");
  sys.register_function<&add_city_wealth>("add_wealth");
  sys.register_function<&person_age>("age");
  sys.register_function<&person_charisma>("charisma");
  sys.register_function<&person::inc_age, handle<person>>("inc_age");
  sys.register_function<&person::add_charisma, handle<person>>("add_charisma");
  sys.register_function<&person::country, handle<person>>("country");
  sys.register_function<&person::living_in, handle<person>>("living_in");
  sys.register_function_iter<&each_city>("each_city", { "value" });
  sys.register_function_iter<&each_notable_person>("each_notable_person", { "filter", "value" });

  const auto run = [&](const std::string& script) {
    const auto cont = sys.parse<double, handle<person>>("script", script);
    ds::context ctx;
    ctx.set_arg(0, p1h);
    ctx.clear();
    std::string out;
    cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      collect(out, entry.name, entry.nest_level, entry.value, entry.scope);
    });
    return out;
  };

  const auto run_legacy = [&](const std::string& script) {
    const auto cont = sys.parse<double, handle<person>>("script", script);
    ds::context ctx;
    ctx.set_arg(0, p1h);
    ctx.clear();
    ds::node_view v;
    cont.make_table(&ctx, v);
    std::string out;
    v.traverse(&cont, [&](const std::string_view& name, const std::string_view&, const size_t nest, const ds::any_stack& value, const ds::any_stack& scope) {
      return collect(out, name, nest, value, scope);
    });
    return out;
  };

  SUBCASE("legacy make_table matches describe") {
    const std::string script = "country = { each_city = { value = population } }";
    CHECK(run_legacy(script) == run(script));
  }

  SUBCASE("every description string resolves from the compact pool") {
    // After dropping the raw source and the globals table, all description text must round-trip
    // through the single `string_pool`. Walk every block_description / command name and assert
    // each non-sentinel reference resolves to a non-empty range inside the pool.
    const std::string script = "country = { each_city = { value = population } }";
    const auto cont = sys.parse<double, handle<person>>("script", script);

    using ref_t = ds::script_container::string_ref;
    const auto is_empty_ref = [](const ref_t& r) { return r.start == SIZE_MAX && r.count == SIZE_MAX; };
    const auto check_ref = [&](const ref_t& r) {
      if (is_empty_ref(r)) return;                 // {SIZE_MAX, SIZE_MAX} == "no name" sentinel
      const auto text = cont.get_string(r);
      CHECK_FALSE(text.empty());
      if (r.count != SIZE_MAX) {                   // offset ref (not a basicf opcode id): must lie in the pool
        CHECK(r.start + r.count <= cont.string_pool.size());
        CHECK(std::string_view(cont.string_pool).substr(r.start, r.count) == text);
      }
    };

    REQUIRE(!cont.block_descs.empty());
    for (const auto& d : cont.block_descs) { check_ref(d.name); check_ref(d.custom_description); }
    for (const auto& n : cont.command_names) check_ref(n);

    // ...and describe surfaces every expected named node, each resolved from the pool.
    ds::context ctx;
    ctx.set_arg(0, p1h);
    ctx.clear();
    std::set<std::string> names;
    cont.describe(&ctx, [&](const ds::container::description_entry& entry) { names.insert(std::string(entry.name)); });
    for (const std::string_view expected : { "country", "each_city", "value", "population" })
      CHECK(names.count(std::string(expected)) == 1);
  }

  SUBCASE("registered functions dump includes signatures and iterator metadata") {
    const std::string dump = sys.dump_registered_functions();

    CHECK(has_text(dump, "registered functions: "));
    CHECK(has_text(dump, "function 'population' scope='"));
    CHECK(has_text(dump, "function 'each_city' scope='"));
    CHECK(has_text(dump, "returns='double'"));
    CHECK(has_text(dump, "kind=iterator"));
    CHECK(has_text(dump, "signature='"));
    CHECK(has_text(dump, "signature='double ("));
    CHECK(has_text(dump, "signature='int ("));
    CHECK(has_text(dump, "operator '+'"));
  }

  SUBCASE("scope path") {
    const std::string expected =
      "'ADD' value: '26' scope: 'handle<person>'\n"
      "  'country' value: 'country*' scope: 'handle<person>'\n"
      "    'leader' value: 'handle<person>' scope: 'country*'\n"
      "      'age' value: '26' scope: 'handle<person>'\n";
    CHECK(run("country.leader:age") == expected);
  }

  SUBCASE("iterator single") {
    const std::string expected =
      "'ADD' value: '343' scope: 'handle<person>'\n"
      "  'country' value: 'country*' scope: 'handle<person>'\n"
      "    'each_city' value: '343' scope: 'country*'\n"
      "      'value' value: 'no value' scope: 'no scope'\n"
      "        'population' value: 'no value' scope: 'no scope'\n";
    CHECK(run("country = { each_city = { value = population } }") == expected);
  }

  SUBCASE("iterator nested") {
    const std::string expected =
      "'ADD' value: '18' scope: 'handle<person>'\n"
      "  'country' value: 'country*' scope: 'handle<person>'\n"
      "    'each_city' value: '18' scope: 'country*'\n"
      "      'value' value: 'no value' scope: 'no scope'\n"
      "        'each_notable_person' value: 'no value' scope: 'no scope'\n"
      "          'value' value: 'no value' scope: 'no scope'\n"
      "            'charisma' value: 'no value' scope: 'no scope'\n";
    CHECK(run("country = { each_city = { value = { each_notable_person = { value = charisma } } } }") == expected);
  }

  SUBCASE("ctx_save + division") {
    const std::string expected =
      "'ADD' value: '0.0606061' scope: 'handle<person>'\n"
      "  'this' value: '0.0606061' scope: 'handle<person>'\n"
      "    'living_in' value: 'city*' scope: 'handle<person>'\n"
      "      'ctx_save' value: 'city*' scope: 'city*'\n"
      "        'each_notable_person' value: '33' scope: 'city*'\n"
      "          'value' value: 'no value' scope: 'no scope'\n"
      "            'age' value: 'no value' scope: 'no scope'\n"
      "      '/' value: '0.0606061' scope: 'city*'\n"
      "        'notable_people_count' value: '2' scope: 'city*'\n"
      "        'ctx' value: '33' scope: 'city*'\n"
      "          'saved' value: '33' scope: 'devils_script::internal::thisctx'\n";
    CHECK(run("this:living_in = { ctx_save = { val1 = { each_notable_person = { value = age } } }, notable_people_count / ctx:saved:val1 }") == expected);
  }

  SUBCASE("ctx_save object + filter") {
    const std::string expected =
      "'ADD' value: '13' scope: 'handle<person>'\n"
      "  'ctx_save' value: 'handle<person>' scope: 'handle<person>'\n"
      "    'this' value: 'handle<person>' scope: 'handle<person>'\n"
      "  'this' value: '13' scope: 'handle<person>'\n"
      "    'living_in' value: 'city*' scope: 'handle<person>'\n"
      "      'each_notable_person' value: '13' scope: 'city*'\n"
      "        'filter' value: 'no value' scope: 'no scope'\n"
      "          '!=' value: 'no value' scope: 'no scope'\n"
      "            'this' value: 'no value' scope: 'no scope'\n"
      "            'ctx' value: 'no value' scope: 'no scope'\n"
      "              'saved' value: 'no value' scope: 'no scope'\n"
      "        'value' value: 'no value' scope: 'no scope'\n"
      "          'age' value: 'no value' scope: 'no scope'\n";
    CHECK(run("{ ctx_save = { cur_player = this }, this:living_in = { each_notable_person = { filter = this != ctx:saved:cur_player, value = age } } }") == expected);
  }
}
