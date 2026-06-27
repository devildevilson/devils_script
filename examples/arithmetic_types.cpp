#include "devils_script/system.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace ds = devils_script;

// A custom arithmetic value must still satisfy the VM stack constraints:
// sizeof(T) <= MAXIMUM_STACK_VAL_SIZE and trivially destructible. Four floats fit exactly.
struct vec4 {
  float x;
  float y;
  float z;
  float w;

  explicit vec4(double v) : x(float(v)), y(float(v)), z(float(v)), w(float(v)) {}
  vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

inline bool operator==(const vec4& a, const vec4& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

// Compile-time classification: vec4 is a value arithmetic type, not a scope/object type.
// This is why vec4 does not need valid(), is_valid(), operator bool(), etc.
namespace devils_script {
template <>
struct is_script_arithmetic_type<::vec4> : std::true_type {};
}

// Named values used by scripts below. In a game/application these would usually read from scope.
vec4 vec_a() { return vec4(100.0); }
vec4 vec_b() { return vec4(20.0); }
vec4 vec_c() { return vec4(3.0); }
vec4 vec_d() { return vec4(40.0); }
vec4 vec_e() { return vec4(5.0); }
double scalar_b() { return 20.0; }

// Arithmetic functions are ordinary C++ functions. The script system only uses signatures.
vec4 vec_add(vec4 a, vec4 b) { return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
vec4 vec_sub(vec4 a, vec4 b) { return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
vec4 vec_mul(vec4 a, vec4 b) { return vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
vec4 vec_div(vec4 a, vec4 b) { return vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }

vec4 vec_add_scalar(vec4 a, double b) { return vec4(a.x + float(b), a.y + float(b), a.z + float(b), a.w + float(b)); }
vec4 scalar_add_vec(double a, vec4 b) { return vec_add_scalar(b, a); }
vec4 vec_mul_scalar(vec4 a, double b) { return vec4(a.x * float(b), a.y * float(b), a.z * float(b), a.w * float(b)); }
vec4 scalar_mul_vec(double a, vec4 b) { return vec_mul_scalar(b, a); }

void register_vec4_arithmetic(ds::system& sys) {
  const ds::system::operator_props mul_props{
    12,
    ds::system::command_data::math_ftype::binary,
    ds::system::command_data::associativity::left
  };
  const ds::system::operator_props add_props{
    11,
    ds::system::command_data::math_ftype::binary,
    ds::system::command_data::associativity::left
  };

  // Runtime arithmetic registration: tells parse<vec4>() which block reducer to use for
  // script blocks/root arithmetic. It does not create casts or operators by itself.
  sys.register_arithmetic_type<vec4>("ADD", 30);

  // Explicit implicit conversion graph. This permits scalar literals/double values to become vec4.
  // No reverse edge is registered, so vec4 will not silently convert back to double.
  sys.register_implicit_conversion<double, vec4>();

  // Block reducers used by "{ ... }" and the root parse<vec4>() arithmetic block.
  // The block name passed to register_arithmetic_type must resolve to a registered function.
  sys.register_function<&vec_add, void>("ADD");
  sys.register_function<&vec_mul, void>("MUL");

  // Symbol operators used by expressions such as "a + b * c - d / e".
  // HT=void is important: vec4 is a value argument, not the function scope.
  sys.register_operator<&vec_mul, void>("*", mul_props);
  sys.register_operator<&vec_div, void>("/", mul_props);
  sys.register_operator<&vec_add, void>("+", add_props);
  sys.register_operator<&vec_sub, void>("-", add_props);

  // Mixed scalar/vector operators. The resolver does not assume commutativity; register both orders
  // if both "vec4 * double" and "double * vec4" are valid script expressions.
  sys.register_operator<&vec_add_scalar, void>("+", add_props);
  sys.register_operator<&scalar_add_vec, void>("+", add_props);
  sys.register_operator<&vec_mul_scalar, void>("*", mul_props);
  sys.register_operator<&scalar_mul_vec, void>("*", mul_props);
}

int main() {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();
  register_vec4_arithmetic(sys);

  sys.register_function<&vec_a, void>("a");
  sys.register_function<&vec_b, void>("b");
  sys.register_function<&vec_c, void>("c");
  sys.register_function<&vec_d, void>("d");
  sys.register_function<&vec_e, void>("e");
  sys.register_function<&scalar_b>("scalar_b");

  {
    // Same-type expression. The parser keeps normal precedence: b*c, d/e, then + and -.
    const auto script = sys.parse<vec4, void>("vec_expression", "a + b * c - d / e");
    ds::context ctx;
    script.process(&ctx);
    assert(ctx.is_return<vec4>());
    assert(ctx.get_return<vec4>() == vec4(152.0));
  }

  {
    // Mixed scalar/vector expression. scalar_b*c uses double*vec4, then a+... uses vec4+vec4.
    const auto script = sys.parse<vec4, void>("mixed_expression", "a + scalar_b * c");
    ds::context ctx;
    script.process(&ctx);
    assert(ctx.is_return<vec4>());
    assert(ctx.get_return<vec4>() == vec4(160.0));
  }

  {
    // A numeric literal can become vec4 because double -> vec4 was registered explicitly.
    const auto script = sys.parse<vec4, void>("literal_conversion", "{ 2, a }");
    ds::context ctx;
    script.process(&ctx);
    assert(ctx.is_return<vec4>());
    assert(ctx.get_return<vec4>() == vec4(102.0));
  }

  std::cout << "custom arithmetic type example passed\n";
  return 0;
}
