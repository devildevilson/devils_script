#include "devils_script/system.h"

#include <string>
#include <cassert>

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

int city_population(const city* c) { return c->population; }
double city_wealth(const city* c) { return c->wealth; }
country* city_owner(const city* c) { return c->owner; }
size_t city_notable_people_count(const city* c) { return c->notable_people.size(); }

void add_city_population(city* c, int pop) { c->population += pop; }
void add_city_wealth(city* c, double w) { c->wealth += w; }

uint16_t person_age(handle<person> p) { return (*p).age; }
int person_charisma(handle<person> p) { return (*p).charisma; }

// iterator
double each_city(country* c, const std::function<double(city*)>& fn) {
  double val = 0.0;
  for (auto city : c->cities) {
    val += fn(city);
  }
  return val;
}

double each_notable_person(city* c, const std::function<bool(handle<person>)>& filter, const std::function<double(handle<person>)>& fn) {
  double val = 0.0;
  for (const auto& p : c->notable_people) {
    if (filter && !filter(p)) continue;
    val += fn(p);
  }
  return val;
}

const std::string scripts[] = {
  "country.leader:age",
  "country = { each_city = { value = population } }",
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

#define RF(fn) register_function<decltype(&fn), &fn>
#define RFH(fn, handle) register_function<decltype(&fn), &fn, handle>
#define RFI(fn) register_function_iter<decltype(&fn), &fn>
#define RFIH(fn, handle) register_function_iter<decltype(&fn), &fn, handle>

bool print(const std::string_view& name, const std::string_view& desc, const size_t nest, const ds::any_stack& value, const ds::any_stack& scope) {
  for (size_t i = 0; i < nest; ++i) {
    std::cout << "  ";
  }

  std::cout << "'" << name << "' value: '";
  if (value.is<double>()) {
    std::cout << value.get<double>();
  } else if (value.is<int64_t>()) {
    std::cout << value.get<int64_t>();
  } else if (value.is<bool>()) {
    std::cout << value.get<bool>();
  } else std::cout << (value.type().empty() ? "no value" : value.type());
  std::cout << "' scope: '" << (scope.type().empty() ? "no scope" : scope.type()) << "'\n";

  return true;
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

  // Function with unique scope type can have same names
  sys.RF(country::get_population)("population");
  sys.RF(country::get_gdp)("gdp");
  sys.RF(country::add_population)("add_population");
  sys.RF(country::add_gdp)("add_gdp");
  sys.RF(country::leader)("leader");
  sys.RF(city_population)("population");
  sys.RF(city_wealth)("wealth");
  sys.RF(city_owner)("owner");
  sys.RF(city_notable_people_count)("notable_people_count");
  sys.RF(add_city_population)("add_population");
  sys.RF(add_city_wealth)("add_wealth");
  sys.RF(person_age)("age");
  sys.RF(person_charisma)("charisma");
  sys.RFH(person::inc_age, handle<person>)("inc_age");
  sys.RFH(person::add_charisma, handle<person>)("add_charisma");
  sys.RFH(person::country, handle<person>)("country");
  sys.RFH(person::living_in, handle<person>)("living_in");
  sys.RFI(each_city)("each_city", { "value" });
  sys.RFI(each_notable_person)("each_notable_person", { "filter", "value" });

  const auto cont1 = sys.parse<double, handle<person>>(scripts[0]);
  const auto cont2 = sys.parse<double, handle<person>>(scripts[1]);
  const auto cont3 = sys.parse<double, handle<person>>(scripts[2]);
  const auto cont4 = sys.parse<double, handle<person>>(scripts[3]);
  const auto cont5 = sys.parse<double, handle<person>>(scripts[4]);

  ds::context ctx;
  ctx.set_arg(0, p1h);

  {
    ctx.clear();
    ds::node_view v;
    cont1.make_table(&ctx, v);
    v.traverse(&cont1, &print);
    std::cout << "\n";
  }

  {
    ds::node_view v;
    ctx.clear();
    cont2.make_table(&ctx, v);
    v.traverse(&cont2, &print);
    std::cout << "\n";
  }

  {
    ds::node_view v;
    ctx.clear();
    cont3.make_table(&ctx, v);
    v.traverse(&cont3, &print);
    std::cout << "\n";
  }

  {
    ds::node_view v;
    ctx.clear();
    cont4.make_table(&ctx, v);
    v.traverse(&cont4, &print);
    std::cout << "\n";
  }

  {
    ds::node_view v;
    ctx.clear();
    cont5.make_table(&ctx, v);
    v.traverse(&cont5, &print);
    std::cout << "\n";
  }

  return 0;
}