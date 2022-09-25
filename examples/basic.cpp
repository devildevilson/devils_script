#include <cstdint>
#include <cstddef>
#include <string>
#include <iostream>

#include "system.h"

// по умолчанию используется add (сложение всех числе подряд в таблице)
const std::string_view script1 = "return { 5, 5, 10 }";
// вызов другой блочной функции
const std::string_view script2 = "return { 1, multiply = { 2, 10 } }";
// кондишен не даст сложить центральное число с остальными
const std::string_view script3 = "return { 5, { condition = false, 5 }, 10 }";
// вызовы обычных функций
const std::string_view script4 = "return { sin = 5, cos = 5, mix = { 1, 2, 0.5 } }";
// по умолчанию используется AND
const std::string_view script5 = "return { true, true, false }";
const std::string_view script51 = "return { 1, 1, 0 }";

const std::string_view script6 = "return { \"val\" }";
const std::string_view script7 = "return { \"more_then_ten\" }";
const std::string_view script8 = "return { add_val = 10 }";

static double mix(const double x, const double y, const double a) { return x * (1.0 - a) + y * a; }

struct test {
  int val;

  test() : val(10) {}
  bool more_then_ten() const { return val > 10; }
  double get_val() const { return val; }
  void add_val(const double num) { val += num; }
};

int main(int argc, char const *argv[]) {
  devils_script::system sys;

  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::bit32, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::package, sol::lib::utf8);

  {
    const auto ret = lua.script(script1);
    CHECK_ERROR_THROW(ret)
    
    const sol::object o = ret;
    const auto num_script = sys.create_number<devils_script::object>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    const double num = num_script.compute(&ctx);
    std::cout << "script1 " << num << "\n"; // 20
  }
  
  {
    const auto ret = lua.script(script2);
    CHECK_ERROR_THROW(ret)
    
    const sol::object o = ret;
    const auto num_script = sys.create_number<devils_script::object>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    const double num = num_script.compute(&ctx);
    std::cout << "script2 " << num << "\n"; // 21
  }
  
  {
    const auto ret = lua.script(script3);
    CHECK_ERROR_THROW(ret)
    
    const sol::object o = ret;
    const auto num_script = sys.create_number<devils_script::object>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    const double num = num_script.compute(&ctx);
    std::cout << "script3 " << num << "\n"; // 15
  }
  
  {
    const auto ret = lua.script(script4);
    CHECK_ERROR_THROW(ret)
    
    const sol::object o = ret;
    const auto num_script = sys.create_number<devils_script::object>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    const double num = num_script.compute(&ctx);
    const double ans = std::sin(5.0) + std::cos(5.0) + mix(1.0, 2.0, 0.5);
    std::cout << "script4 " << num << " ans " << ans << "\n";
  }
  
  {
    const auto ret = lua.script(script5);
    CHECK_ERROR_THROW(ret)
    const sol::object o = ret;
    const auto num_script = sys.create_condition<devils_script::object>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    const bool num = num_script.compute(&ctx);
    const bool ans = true && true && false;
    std::cout << std::boolalpha << "script5 " << num << " ans " << ans << "\n";
  }
  
  // мы можем использовать boolean значения в качестве чисел и наоборот
  {
    const auto ret = lua.script(script5);
    CHECK_ERROR_THROW(ret)
    const sol::object o = ret;
    const auto num_script = sys.create_number<devils_script::object>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    const double num = num_script.compute(&ctx);
    const double ans = double(true) + double(true) + double(false);
    std::cout << std::boolalpha << "script5 " << num << " ans " << ans << "\n";
  }
  
  {
    const auto ret = lua.script(script51);
    CHECK_ERROR_THROW(ret)
    const sol::object o = ret;
    const auto num_script = sys.create_condition<devils_script::object>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    const double num = num_script.compute(&ctx);
    const double ans = bool(1.0) && bool(1.0) && bool(0.0);
    std::cout << std::boolalpha << "script51 " << num << " ans " << ans << "\n";
  }

  using test_t = test*;
  sys.register_usertype<test_t>();

  sys.REG_FUNC(test_t, test::more_then_ten, "more_then_ten")();
  sys.REG_FUNC(test_t, test::get_val, "val")();
  sys.REG_FUNC(test_t, test::add_val, "add_val")();

  test t;

  //using DEVILS_SCRIPT_FULL_NAMESPACE::_create;

  {
    const auto ret = lua.script(script6);
    CHECK_ERROR_THROW(ret)
    
    const sol::object o = ret;
    const auto num_script = sys.create_number<test_t>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    ctx.current = devils_script::object(&t);
    const double num = num_script.compute(&ctx);
    std::cout << "script6 " << num << " test::get_val() " << t.get_val() << "\n";
  }

  {
    const auto ret = lua.script(script7);
    CHECK_ERROR_THROW(ret)
    
    const sol::object o = ret;
    const auto cond_script = sys.create_condition<test_t>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    ctx.current = devils_script::object(&t);
    const bool num = cond_script.compute(&ctx);
    std::cout << std::boolalpha << "script7 " << num << " test::more_then_ten() " << t.more_then_ten() << "\n";
  }

  {
    const auto ret = lua.script(script8);
    CHECK_ERROR_THROW(ret)
    
    const sol::object o = ret;
    const auto eff_script = sys.create_effect<test_t>(o, "basic script");
    devils_script::context ctx("basic script", "num_script", 1);
    ctx.current = devils_script::object(&t);
    eff_script.compute(&ctx);
    std::cout << "script8 test::get_val() " << t.get_val() << "\n";
  }

  return 0;
}