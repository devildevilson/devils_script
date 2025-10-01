#include "devils_script/basic_functions.h"
#include <cmath>
#include "devils_script/context.h"
#include "devils_script/container.h"
#include "devils_script/prng.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

static constexpr int64_t make_mask(const size_t count) {
  int64_t val = 0;
  for (size_t i = 0; i < std::min(count, sizeof(int64_t) * CHAR_BIT); ++i) {
    val = val | (int64_t(0x1) << i);
  }
  return val;
}

int64_t packstrid(const uint8_t global_index, const uint32_t pos, const uint32_t size) noexcept {
  return (int64_t(global_index) << (packed_pos_bit_size + packed_size_bit_size)) | (int64_t(pos) << packed_size_bit_size) | (int64_t(size));
}

std::tuple<uint8_t, uint32_t, uint32_t> unpackstrid(const int64_t value) noexcept {
  constexpr int64_t index_mask = make_mask(sizeof(uint8_t) * CHAR_BIT);
  constexpr int64_t pos_mask = make_mask(packed_pos_bit_size);
  constexpr int64_t size_mask = make_mask(packed_size_bit_size);
  return std::make_tuple(uint8_t((value >> (packed_pos_bit_size + packed_size_bit_size)) & index_mask), uint32_t((value >> packed_size_bit_size) & pos_mask), uint32_t((value) & pos_mask));
}

// влияет ли endian на это дело?
int64_t pack2(const int32_t val1, const int32_t val2) noexcept {
  union p2 { struct { int32_t a, b; }; int64_t c; } p2;
  p2.a = val1; p2.b = val2;
  return p2.c;
}

std::tuple<int32_t, int32_t> unpack2(const int64_t val) noexcept {
  union p2 { struct { int32_t a, b; }; int64_t c; } p2;
  p2.c = val;
  return std::make_tuple(p2.a, p2.b);
}

bool check_value(const size_t val, const size_t bits) noexcept {
  const int64_t index_mask = ~(make_mask(bits));
  return (val & index_mask) == 0;
}

bool check_value(const int64_t val, const size_t bits) noexcept {
  const int64_t index_mask = ~(make_mask(bits));
  return (val & index_mask) == 0;
}

int64_t andjump(int64_t arg, context* ctx, const container* scr) {
  const bool b2 = ctx->stack.safe_pop<bool>();
  const bool b1 = ctx->stack.safe_pop<bool>();
  const bool res = b1 && b2;
  ctx->stack.push(res); // должно остаться последнее значение
  if (!res) ctx->current_index = arg - 1;
  return -1;
}

int64_t orjump(int64_t arg, context* ctx, const container* scr) {
  const bool b2 = ctx->stack.safe_pop<bool>();
  const bool b1 = ctx->stack.safe_pop<bool>();
  const bool res = b1 || b2;
  ctx->stack.push(res);
  if (res) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjump(int64_t arg, context* ctx, const container* scr) {
  const bool b = ctx->stack.safe_pop<bool>();
  if (!b) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjumpt(int64_t arg, context* ctx, const container* scr) {
  const bool b = ctx->stack.safe_pop<bool>();
  if (b) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjump_get(int64_t arg, context* ctx, const container*) {
  const bool b = ctx->stack.safe_get<bool>();
  if (!b) ctx->current_index = arg - 1;
  return 0;
}

int64_t condjumpt_get(int64_t arg, context* ctx, const container*) {
  const bool b = ctx->stack.safe_get<bool>();
  if (b) ctx->current_index = arg - 1;
  return 0;
}

int64_t andjump_unsafe(int64_t arg, context* ctx, const container* scr) {
  const bool b2 = ctx->stack.pop<bool>();
  const bool b1 = ctx->stack.pop<bool>();
  const bool res = b1 && b2;
  ctx->stack.push(res); // должно остаться последнее значение
  if (!res) ctx->current_index = arg - 1;
  return -1;
}

int64_t orjump_unsafe(int64_t arg, context* ctx, const container* scr) {
  const bool b2 = ctx->stack.pop<bool>();
  const bool b1 = ctx->stack.pop<bool>();
  const bool res = b1 || b2;
  ctx->stack.push(res);
  if (res) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjump_unsafe(int64_t arg, context* ctx, const container* scr) {
  const bool b = ctx->stack.pop<bool>();
  if (!b) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjumpt_unsafe(int64_t arg, context* ctx, const container* scr) {
  const bool b = ctx->stack.pop<bool>();
  if (b) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjump_get_unsafe(int64_t arg, context* ctx, const container*) {
  const bool b = ctx->stack.get<bool>();
  if (!b) ctx->current_index = arg - 1;
  return 0;
}

int64_t condjumpt_get_unsafe(int64_t arg, context* ctx, const container*) {
  const bool b = ctx->stack.get<bool>();
  if (b) ctx->current_index = arg - 1;
  return 0;
}

int64_t jump(int64_t arg, context* ctx, const container* scr) {
  ctx->current_index = arg - 1;
  return 0;
}

int64_t andbin(int64_t, context* ctx, const container*) {
  const bool n2 = ctx->stack.safe_pop<bool>();
  const bool n1 = ctx->stack.safe_pop<bool>();
  ctx->stack.push(n1 && n2);
  return -1;
}

int64_t orbin(int64_t, context* ctx, const container*) {
  const bool n2 = ctx->stack.safe_pop<bool>();
  const bool n1 = ctx->stack.safe_pop<bool>();
  ctx->stack.push(n1 || n2);
  return -1;
}

int64_t invb(int64_t arg, context* ctx, const container* scr) {
  const bool b = ctx->stack.safe_pop<bool>();
  ctx->stack.push(!b);
  return 0;
}

int64_t sum(int64_t arg, context* ctx, const container* scr) {
  const double n2 = ctx->stack.safe_pop<double>();
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(n1 + n2);
  return -1;
}

int64_t mul(int64_t arg, context* ctx, const container* scr) {
  const double n2 = ctx->stack.safe_pop<double>();
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(n1 * n2);
  return -1;
}

int64_t neg(int64_t arg, context* ctx, const container* scr) {
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(-n1);
  return 0;
}

int64_t pos(int64_t arg, context* ctx, const container* scr) {
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(+n1);
  return 0;
}

int64_t invd(int64_t arg, context* ctx, const container* scr) {
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(1.0 / n1);
  return 0;
}

int64_t cmpeq2(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto &v1 = ctx->stack.get_view(id1);
  const auto &v2 = ctx->stack.get_view(id2);
  ctx->stack.push(memcmp(v1._mem, v2._mem, MAXIMUM_STACK_VAL_SIZE) == 0 && v1.type() == v2.type());
  return 1;
}

int64_t cmplessd2(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.safe_get<double>(id1);
  const auto& v2 = ctx->stack.safe_get<double>(id2);
  ctx->stack.push(v1 < v2);
  return 1;
}

int64_t cmplesseqd2(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.safe_get<double>(id1);
  const auto& v2 = ctx->stack.safe_get<double>(id2);
  ctx->stack.push(v1 <= v2);
  return 1;
}

// sumset 1, 2
int64_t sumsetstack(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.safe_get<double>(id1);
  const auto& v2 = ctx->stack.safe_get<double>(id2);
  ctx->stack.set(id1, v1 + v2);
  return 0;
}

int64_t mulsetstack(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.safe_get<double>(id1);
  const auto& v2 = ctx->stack.safe_get<double>(id2);
  ctx->stack.set(id1, v1 * v2);
  return 0;
}

int64_t andbin_unsafe(int64_t, context* ctx, const container*) {
  const bool n2 = ctx->stack.pop<bool>();
  const bool n1 = ctx->stack.pop<bool>();
  ctx->stack.push(n1 && n2);
  return -1;
}

int64_t orbin_unsafe(int64_t, context* ctx, const container*) {
  const bool n2 = ctx->stack.pop<bool>();
  const bool n1 = ctx->stack.pop<bool>();
  ctx->stack.push(n1 || n2);
  return -1;
}

int64_t invb_unsafe(int64_t arg, context* ctx, const container* scr) {
  const bool b = ctx->stack.pop<bool>();
  ctx->stack.push(!b);
  return 0;
}

int64_t sum_unsafe(int64_t arg, context* ctx, const container* scr) {
  const double n2 = ctx->stack.pop<double>();
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(n1 + n2);
  return -1;
}

int64_t mul_unsafe(int64_t arg, context* ctx, const container* scr) {
  const double n2 = ctx->stack.pop<double>();
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(n1 * n2);
  return -1;
}

int64_t neg_unsafe(int64_t arg, context* ctx, const container* scr) {
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(-n1);
  return 0;
}

int64_t pos_unsafe(int64_t arg, context* ctx, const container* scr) {
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(+n1);
  return 0;
}

int64_t invd_unsafe(int64_t arg, context* ctx, const container* scr) {
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(1.0 / n1);
  return 0;
}

int64_t cmpeq2_unsafe(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.get_view(id1);
  const auto& v2 = ctx->stack.get_view(id2);
  ctx->stack.push(memcmp(v1._mem, v2._mem, MAXIMUM_STACK_VAL_SIZE) == 0);
  return 1;
}

int64_t cmplessd2_unsafe(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.get<double>(id1);
  const auto& v2 = ctx->stack.get<double>(id2);
  ctx->stack.push(v1 < v2);
  return 1;
}

int64_t cmplesseqd2_unsafe(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.get<double>(id1);
  const auto& v2 = ctx->stack.get<double>(id2);
  ctx->stack.push(v1 <= v2);
  return 1;
}

// sumset 1, 2
int64_t sumsetstack_unsafe(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.get<double>(id1);
  const auto& v2 = ctx->stack.get<double>(id2);
  ctx->stack.set(id1, v1 + v2);
  return 0;
}

int64_t mulsetstack_unsafe(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  const auto& v1 = ctx->stack.get<double>(id1);
  const auto& v2 = ctx->stack.get<double>(id2);
  ctx->stack.set(id1, v1 * v2);
  return 0;
}

int64_t pushbool(int64_t arg, context* ctx, const container* scr) {
  ctx->stack.push(bool(arg));
  return 1;
}

int64_t pushvalue(int64_t arg, context* ctx, const container* scr) {
  ctx->stack.push(std::bit_cast<double>(arg));
  return 1;
}

int64_t pushint(int64_t arg, context* ctx, const container* scr) {
  ctx->stack.push(arg);
  return 1;
}

int64_t pushstring(int64_t arg, context* ctx, const container* scr) {
  const auto [global_index, pos, size] = unpackstrid(arg);
  const auto tmp = std::string_view(scr->globals[global_index]);
  const auto str = std::string_view(tmp.data()+pos, size);
  ctx->stack.push(str);
  return 1;
}

int64_t pushroot(int64_t arg, context* ctx, const container* scr) {
  ctx->stack.push(ctx->safe_get_arg<any_stack>(0));
  return 1;
}

int64_t pushthis(int64_t arg, context* ctx, const container*) {
  const auto& el = ctx->stack.element(arg);
  const auto type = ctx->stack.type(arg);
  ctx->stack.push(type, el);
  return 1;
}

int64_t pushprev(int64_t arg, context* ctx, const container*) {
  const auto& el = ctx->stack.element(arg);
  const auto type = ctx->stack.type(arg);
  ctx->stack.push(type, el);
  return 1;
}

int64_t pushreturn(int64_t arg, context* ctx, const container* scr) {
  const auto &el = ctx->stack.element();
  const auto type = ctx->stack.type();
  ctx->stack.erase();
  ctx->set_return(type, el);
  return 0;
}

int64_t pusharg(int64_t arg, context* ctx, const container* scr) {
  //ctx->stack.push(scr->args_type[arg], ctx->_args[arg]);
  return 1;
}

int64_t pushinvalid(int64_t, context* ctx, const container*) {
  stack_element el;
  el.invalidate();
  ctx->stack.push(std::string_view(), el);
  return 1;
}

int64_t erase(int64_t arg, context* ctx, const container* scr) {
  ctx->stack.erase(arg);
  return -1;
}

int64_t pushcurrent(int64_t arg, context* ctx, const container*) {
  ctx->stack.push(ctx->stack.get_view());
  return 1;
}

int64_t pushchance(int64_t arg, context* ctx, const container* scr) {
  const auto state = std::bit_cast<uint64_t>(arg);
  const auto val = prng::mix(ctx->prng_state, state, scr->prng_state);
  const double norm = prng::prng_normalize(val);
  ctx->stack.push(norm);
  return 1;
}

int64_t pushargcontext(int64_t, context* ctx, const container* scr) {
  ctx->stack.push(internal::thisarg{ ctx, scr });
  return 1;
}

int64_t pushcontext(int64_t, context* ctx, const container*) {
  ctx->stack.push(internal::thisctx{ ctx });
  return 1;
}

int64_t pushargvalue(int64_t arg, context* ctx, const container* scr) {
  ctx->stack.push(ctx->get_arg<any_stack>(arg));
  return 1;
}

int64_t setargrvalue(int64_t arg, context* ctx, const container* scr) {
  ctx->set_arg(arg, ctx->stack.pop<any_stack>());
  return -1;
}

int64_t setarglvalue(int64_t arg, context* ctx, const container* scr) {
  const auto [id1, id2] = unpack2(arg);
  ctx->set_arg(id2, ctx->stack.get<any_stack>(id1));
  return 0;
}

int64_t pushctxvalue(int64_t arg, context* ctx, const container*) {
  //ctx->stack.push(ctx->get_arg<any_stack>(arg));
  ctx->stack.push(ctx->get_saved<any_stack>(arg));
  return 1;
}

int64_t savectxrvalue(int64_t arg, context* ctx, const container*) {
  ctx->set_saved(arg, ctx->stack.pop<any_stack>());
  return -1;
}

int64_t savectxlvalue(int64_t arg, context* ctx, const container*) {
  const auto [id1, id2] = unpack2(arg);
  ctx->set_saved(id2, ctx->stack.get<any_stack>(id1));
  return 0;
}

int64_t pushlist(int64_t arg, context* ctx, const container*) {
  ctx->stack.push(internal::thisctxlist{ ctx, size_t(arg) });
  return 1;
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}

