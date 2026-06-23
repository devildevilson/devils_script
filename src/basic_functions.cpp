#include "devils_script/basic_functions.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "devils_script/context.h"
#include "devils_script/container.h"
#include "devils_script/prng.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

namespace {

void check_script_arg_type(const int64_t arg, const context* ctx, const script_container* scr) {
  if (scr == nullptr || arg < 0 || static_cast<size_t>(arg) >= scr->args.size()) return;

  const auto expected = scr->args[static_cast<size_t>(arg)].type;
  if (expected.empty() || type_is_any_type(expected)) return;

  const auto actual = ctx->arg_type(arg + int64_t(ctx->arg_base));
  if (actual != expected) {
    throw std::runtime_error(std::format("Script argument #{} has type '{}', expected '{}'", arg, actual, expected));
  }
}

}

static constexpr int64_t make_mask(const size_t count) {
  int64_t val = 0;
  for (size_t i = 0; i < std::min(count, sizeof(int64_t) * CHAR_BIT); ++i) {
    val = val | (int64_t(0x1) << i);
  }
  return val;
}

int64_t packstrid(const uint32_t pos, const uint32_t size) noexcept {
  return (int64_t(pos) << packed_size_bit_size) | (int64_t(size));
}

std::tuple<uint32_t, uint32_t> unpackstrid(const int64_t value) noexcept {
  constexpr int64_t pos_mask = make_mask(packed_pos_bit_size);
  constexpr int64_t size_mask = make_mask(packed_size_bit_size);
  return std::make_tuple(uint32_t((value >> packed_size_bit_size) & pos_mask), uint32_t((value) & size_mask));
}

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

int64_t andjump(int64_t arg, context* ctx, const script_container*) {
  const bool b2 = ctx->stack.safe_pop<bool>();
  const bool b1 = ctx->stack.safe_pop<bool>();
  const bool res = b1 && b2;
  ctx->stack.push(res);
  if (!res) ctx->current_index = arg - 1;
  return -1;
}

int64_t orjump(int64_t arg, context* ctx, const script_container*) {
  const bool b2 = ctx->stack.safe_pop<bool>();
  const bool b1 = ctx->stack.safe_pop<bool>();
  const bool res = b1 || b2;
  ctx->stack.push(res);
  if (res) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjump(int64_t arg, context* ctx, const script_container*) {
  const bool b = ctx->stack.safe_pop<bool>();
  if (!b) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjump_get(int64_t arg, context* ctx, const script_container*) {
  const bool b = ctx->stack.safe_get<bool>();
  if (!b) ctx->current_index = arg - 1;
  return 0;
}

int64_t condjumpt_get(int64_t arg, context* ctx, const script_container*) {
  const bool b = ctx->stack.safe_get<bool>();
  if (b) ctx->current_index = arg - 1;
  return 0;
}

int64_t andjump_unsafe(int64_t arg, context* ctx, const script_container*) {
  const bool b2 = ctx->stack.pop<bool>();
  const bool b1 = ctx->stack.pop<bool>();
  const bool res = b1 && b2;
  ctx->stack.push(res);
  if (!res) ctx->current_index = arg - 1;
  return -1;
}

int64_t orjump_unsafe(int64_t arg, context* ctx, const script_container*) {
  const bool b2 = ctx->stack.pop<bool>();
  const bool b1 = ctx->stack.pop<bool>();
  const bool res = b1 || b2;
  ctx->stack.push(res);
  if (res) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjump_unsafe(int64_t arg, context* ctx, const script_container*) {
  const bool b = ctx->stack.pop<bool>();
  if (!b) ctx->current_index = arg - 1;
  return -1;
}

int64_t condjump_get_unsafe(int64_t arg, context* ctx, const script_container*) {
  const bool b = ctx->stack.get<bool>();
  if (!b) ctx->current_index = arg - 1;
  return 0;
}

int64_t condjumpt_get_unsafe(int64_t arg, context* ctx, const script_container*) {
  const bool b = ctx->stack.get<bool>();
  if (b) ctx->current_index = arg - 1;
  return 0;
}

int64_t jump(int64_t arg, context* ctx, const script_container*) {
  ctx->current_index = arg - 1;
  return 0;
}

int64_t jumpinvalid(int64_t arg, context* ctx, const script_container*) {
  if (ctx->stack.invalid(-1)) {
    ctx->stack.erase();
    ctx->current_index = arg - 1;
    return -1;
  }
  return 0;
}

int64_t andbin(int64_t, context* ctx, const script_container*) {
  const bool n2 = ctx->stack.safe_pop<bool>();
  const bool n1 = ctx->stack.safe_pop<bool>();
  ctx->stack.push(n1 && n2);
  return -1;
}

int64_t invb(int64_t, context* ctx, const script_container*) {
  const bool b = ctx->stack.safe_pop<bool>();
  ctx->stack.push(!b);
  return 0;
}

int64_t sum(int64_t, context* ctx, const script_container*) {
  const double n2 = ctx->stack.safe_pop<double>();
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(n1 + n2);
  return -1;
}

int64_t mul(int64_t, context* ctx, const script_container*) {
  const double n2 = ctx->stack.safe_pop<double>();
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(n1 * n2);
  return -1;
}

int64_t neg(int64_t, context* ctx, const script_container*) {
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(-n1);
  return 0;
}

int64_t pos(int64_t, context* ctx, const script_container*) {
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(+n1);
  return 0;
}

int64_t invd(int64_t, context* ctx, const script_container*) {
  const double n1 = ctx->stack.safe_pop<double>();
  ctx->stack.push(1.0 / n1);
  return 0;
}

int64_t cmpeq2(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto &v1 = ctx->stack.get_view(id1);
  const auto &v2 = ctx->stack.get_view(id2);
  if (v1.type() != v2.type()) {
    ctx->stack.push(false);
  } else if (v1.type() == utils::type_name<std::string_view>()) {
    ctx->stack.push(ctx->stack.safe_get<std::string_view>(id1) == ctx->stack.safe_get<std::string_view>(id2));
  } else {
    ctx->stack.push(memcmp(v1._mem, v2._mem, MAXIMUM_STACK_VAL_SIZE) == 0);
  }
  return 1;
}

int64_t cmplessd2(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.safe_get<double>(id1);
  const auto& v2 = ctx->stack.safe_get<double>(id2);
  ctx->stack.push(v1 < v2);
  return 1;
}

int64_t cmplesseqd2(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.safe_get<double>(id1);
  const auto& v2 = ctx->stack.safe_get<double>(id2);
  ctx->stack.push(v1 <= v2);
  return 1;
}

int64_t sumsetstack(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.safe_get<double>(id1);
  const auto& v2 = ctx->stack.safe_get<double>(id2);
  ctx->stack.set(id1, v1 + v2);
  return 0;
}

int64_t mulsetstack(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.safe_get<double>(id1);
  const auto& v2 = ctx->stack.safe_get<double>(id2);
  ctx->stack.set(id1, v1 * v2);
  return 0;
}

int64_t andbin_unsafe(int64_t, context* ctx, const script_container*) {
  const bool n2 = ctx->stack.pop<bool>();
  const bool n1 = ctx->stack.pop<bool>();
  ctx->stack.push(n1 && n2);
  return -1;
}

int64_t invb_unsafe(int64_t, context* ctx, const script_container*) {
  const bool b = ctx->stack.pop<bool>();
  ctx->stack.push(!b);
  return 0;
}

int64_t sum_unsafe(int64_t, context* ctx, const script_container*) {
  const double n2 = ctx->stack.pop<double>();
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(n1 + n2);
  return -1;
}

int64_t mul_unsafe(int64_t, context* ctx, const script_container*) {
  const double n2 = ctx->stack.pop<double>();
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(n1 * n2);
  return -1;
}

int64_t neg_unsafe(int64_t, context* ctx, const script_container*) {
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(-n1);
  return 0;
}

int64_t pos_unsafe(int64_t, context* ctx, const script_container*) {
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(+n1);
  return 0;
}

int64_t invd_unsafe(int64_t, context* ctx, const script_container*) {
  const double n1 = ctx->stack.pop<double>();
  ctx->stack.push(1.0 / n1);
  return 0;
}

int64_t cmpeq2_unsafe(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.get_view(id1);
  const auto& v2 = ctx->stack.get_view(id2);
  if (v1.type() != v2.type()) {
    ctx->stack.push(false);
  } else if (v1.type() == utils::type_name<std::string_view>()) {
    ctx->stack.push(ctx->stack.get<std::string_view>(id1) == ctx->stack.get<std::string_view>(id2));
  } else {
    ctx->stack.push(memcmp(v1._mem, v2._mem, MAXIMUM_STACK_VAL_SIZE) == 0);
  }
  return 1;
}

int64_t cmplessd2_unsafe(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.get<double>(id1);
  const auto& v2 = ctx->stack.get<double>(id2);
  ctx->stack.push(v1 < v2);
  return 1;
}

int64_t cmplesseqd2_unsafe(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.get<double>(id1);
  const auto& v2 = ctx->stack.get<double>(id2);
  ctx->stack.push(v1 <= v2);
  return 1;
}

int64_t sumsetstack_unsafe(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.get<double>(id1);
  const auto& v2 = ctx->stack.get<double>(id2);
  ctx->stack.set(id1, v1 + v2);
  return 0;
}

int64_t mulsetstack_unsafe(int64_t arg, context* ctx, const script_container*) {
  const auto [a, b] = unpack2(arg);
  const int64_t fb = int64_t(ctx->frame_base);
  const int64_t id1 = a + fb, id2 = b + fb;
  const auto& v1 = ctx->stack.get<double>(id1);
  const auto& v2 = ctx->stack.get<double>(id2);
  ctx->stack.set(id1, v1 * v2);
  return 0;
}

int64_t pushbool(int64_t arg, context* ctx, const script_container*) {
  ctx->stack.push(bool(arg));
  return 1;
}

int64_t pushvalue(int64_t arg, context* ctx, const script_container*) {
  ctx->stack.push(std::bit_cast<double>(arg));
  return 1;
}

int64_t pushint(int64_t arg, context* ctx, const script_container*) {
  ctx->stack.push(arg);
  return 1;
}

int64_t pushstring(int64_t arg, context* ctx, const script_container* scr) {
  const auto [pos, size] = unpackstrid(arg);
  const auto str = scr->get_string(script_container::string_ref{ pos, size });
  ctx->stack.push(str);
  return 1;
}

int64_t pushroot(int64_t, context* ctx, const script_container*) {
  check_script_arg_type(0, ctx, ctx->current_script);
  ctx->stack.push(ctx->safe_get_arg<any_stack>(ctx->arg_base));
  return 1;
}

int64_t pushthis(int64_t arg, context* ctx, const script_container*) {
  const int64_t idx = arg + int64_t(ctx->frame_base);
  const auto& el = ctx->stack.element(idx);
  const auto type = ctx->stack.type(idx);
  ctx->stack.push(type, el);
  return 1;
}

int64_t pushprev(int64_t arg, context* ctx, const script_container*) {
  const int64_t idx = arg + int64_t(ctx->frame_base);
  const auto& el = ctx->stack.element(idx);
  const auto type = ctx->stack.type(idx);
  ctx->stack.push(type, el);
  return 1;
}

int64_t pushreturn(int64_t, context* ctx, const script_container*) {
  const auto &el = ctx->stack.element();
  const auto type = ctx->stack.type();
  ctx->stack.erase();
  ctx->set_return(type, el);
  return 0;
}

int64_t pusharg(int64_t, context*, const script_container*) {
  return 1;
}

int64_t pushinvalid(int64_t, context* ctx, const script_container*) {
  stack_element el;
  el.invalidate();
  ctx->stack.push(std::string_view(), el);
  return 1;
}

int64_t erase(int64_t arg, context* ctx, const script_container*) {
  ctx->stack.erase(arg + int64_t(ctx->frame_base));
  return -1;
}

int64_t pushcurrent(int64_t, context* ctx, const script_container*) {
  ctx->stack.push(ctx->stack.get_view());
  return 1;
}

int64_t pushchance(int64_t arg, context* ctx, const script_container* scr) {
  const auto state = std::bit_cast<uint64_t>(arg);
  const auto val = prng::mix(ctx->prng_state, state, scr->prng_state);
  const double norm = prng::prng_normalize(val);
  ctx->stack.push(norm);
  return 1;
}

int64_t pushargcontext(int64_t, context* ctx, const script_container* scr) {
  ctx->stack.push(internal::thisarg{ ctx, scr });
  return 1;
}

int64_t pushcontext(int64_t, context* ctx, const script_container*) {
  ctx->stack.push(internal::thisctx{ ctx });
  return 1;
}

int64_t pushargvalue(int64_t arg, context* ctx, const script_container* scr) {
  check_script_arg_type(arg, ctx, scr);
  ctx->stack.push(ctx->get_arg<any_stack>(arg + int64_t(ctx->arg_base)));
  return 1;
}

int64_t setargrvalue(int64_t arg, context* ctx, const script_container*) {
  ctx->set_arg(arg + int64_t(ctx->arg_base), ctx->stack.pop<any_stack>());
  return -1;
}

int64_t setarglvalue(int64_t arg, context* ctx, const script_container*) {
  const auto [id1, id2] = unpack2(arg);
  ctx->set_arg(id2 + int64_t(ctx->arg_base), ctx->stack.get<any_stack>(id1 + int64_t(ctx->frame_base)));
  return 0;
}

int64_t pushctxvalue(int64_t arg, context* ctx, const script_container*) {
  ctx->stack.push(ctx->get_saved<any_stack>(arg + int64_t(ctx->saved_base)));
  return 1;
}

int64_t savectxrvalue(int64_t arg, context* ctx, const script_container*) {
  ctx->set_saved(arg + int64_t(ctx->saved_base), ctx->stack.pop<any_stack>());
  return -1;
}

int64_t savectxlvalue(int64_t arg, context* ctx, const script_container*) {
  const auto [id1, id2] = unpack2(arg);
  ctx->set_saved(id2 + int64_t(ctx->saved_base), ctx->stack.get<any_stack>(id1 + int64_t(ctx->frame_base)));
  return 0;
}

int64_t pushlist(int64_t arg, context* ctx, const script_container*) {
  ctx->stack.push(internal::thisctxlist{ ctx, size_t(arg) + ctx->list_base });
  return 1;
}

namespace {
template <typename T>
T run_list_callback(context* ctx, const script_container* scr, const size_t start, const size_t end, const std::string_view& type, const stack_element& input) {
  ctx->stack.push(type, input);
  container_view v(scr, start, end);
  v.process(ctx);
  return ctx->stack.safe_pop<T>();
}

any_stack run_default_callback(context* ctx, const script_container* scr, const size_t start, const size_t end) {
  container_view v(scr, start, end);
  v.process(ctx);
  return ctx->stack.safe_pop<any_stack>();
}

void push_any_to_list(std::vector<stack_element>& list, const any_stack& val) {
  stack_element el;
  el.set(val.view());
  list.emplace_back(el);
}
}

int64_t list_op_data(int64_t, context*, const script_container*) { return 0; }

// Push the command name (packed string-pool ref in `arg`) onto the stack just before an effect call,
// so the on_effect callback can read it as an ordinary stack value instead of a per-command side table.
int64_t push_command_name(int64_t arg, context* ctx, const script_container* scr) {
  const auto [pos, size] = unpackstrid(arg);
  ctx->stack.push(scr->get_string(pos, size));
  return 1;
}

int64_t list_pipeline(int64_t arg, context* ctx, const script_container* scr) {
  // Metadata lives inline in the instruction stream, not a side table: the opcode arg packs
  // (kind, list_index); the three following cmd slots carry, as immediates read here, the callback
  // ranges, the resume point and the captured input element type. P is this opcode's own index.
  const size_t P = ctx->current_index;
  const auto [kind_raw, list_index_raw] = unpack2(arg);
  const auto kind = container::list_pipeline_kind(kind_raw);
  const size_t list_index = size_t(list_index_raw);

  const auto [vs_off, ve_off] = unpack2(scr->cmds[P + 1].arg);
  const auto [ds_off, end_off] = unpack2(scr->cmds[P + 2].arg);
  const auto [it_pos, it_size] = unpackstrid(scr->cmds[P + 3].arg);

  const size_t value_start = P + size_t(vs_off);
  const size_t value_end = P + size_t(ve_off);
  const size_t default_start = P + size_t(ds_off);
  const size_t end = P + size_t(end_off);
  const size_t default_end = end;  // a default section, when present, is always the last one

  auto& list = ctx->lists[list_index + ctx->list_base];  // physical slot; scr->lists is per-container metadata, not offset
  const auto type = scr->lists[list_index].type;
  const auto stored_input = it_size == 0 ? std::string_view() : scr->get_string(it_pos, it_size);
  const auto input_type = stored_input.empty() ? type : stored_input;

  switch (kind) {
    case container::list_pipeline_kind::add_to: {
      const auto val = run_default_callback(ctx, scr, default_start, default_end);
      if (!type.empty() && val.type() != type) throw std::runtime_error(std::format("List expects '{}', got '{}'", type, val.type()));
      push_any_to_list(list, val);
      break;
    }

    case container::list_pipeline_kind::clear: {
      list.clear();
      break;
    }

    case container::list_pipeline_kind::filter: {
      size_t out = 0;
      for (size_t i = 0; i < list.size(); ++i) {
        if (run_list_callback<bool>(ctx, scr, value_start, value_end, input_type, list[i])) {
          if (out != i) list[out] = list[i];
          out += 1;
        }
      }
      list.resize(out);
      break;
    }

    case container::list_pipeline_kind::map: {
      std::vector<stack_element> tmp;
      tmp.reserve(list.size());
      std::string_view mapped_type;
      for (auto& item : list) {
        const auto val = run_list_callback<any_stack>(ctx, scr, value_start, value_end, input_type, item);
        if (mapped_type.empty()) mapped_type = val.type();
        if (mapped_type != val.type()) throw std::runtime_error(std::format("List map returned mixed types '{}' and '{}'", mapped_type, val.type()));
        push_any_to_list(tmp, val);
      }
      list.swap(tmp);
      break;
    }

    case container::list_pipeline_kind::count: {
      ctx->stack.push(double(list.size()));
      break;
    }

    case container::list_pipeline_kind::empty: {
      ctx->stack.push(list.empty());
      break;
    }

    case container::list_pipeline_kind::any: {
      bool ret = false;
      for (auto& item : list) {
        if (run_list_callback<bool>(ctx, scr, value_start, value_end, input_type, item)) { ret = true; break; }
      }
      ctx->stack.push(ret);
      break;
    }

    case container::list_pipeline_kind::all: {
      bool ret = true;
      for (auto& item : list) {
        if (!run_list_callback<bool>(ctx, scr, value_start, value_end, input_type, item)) { ret = false; break; }
      }
      ctx->stack.push(ret);
      break;
    }

    case container::list_pipeline_kind::none: {
      bool ret = true;
      for (auto& item : list) {
        if (run_list_callback<bool>(ctx, scr, value_start, value_end, input_type, item)) { ret = false; break; }
      }
      ctx->stack.push(ret);
      break;
    }

    case container::list_pipeline_kind::count_if: {
      double ret = 0.0;
      for (auto& item : list) ret += double(run_list_callback<bool>(ctx, scr, value_start, value_end, input_type, item));
      ctx->stack.push(ret);
      break;
    }

    case container::list_pipeline_kind::sum: {
      double ret = 0.0;
      for (auto& item : list) ret += run_list_callback<double>(ctx, scr, value_start, value_end, input_type, item);
      ctx->stack.push(ret);
      break;
    }

    case container::list_pipeline_kind::min:
    case container::list_pipeline_kind::max:
    case container::list_pipeline_kind::average: {
      bool found = false;
      double ret = 0.0;
      double sum = 0.0;
      size_t count = 0;
      for (auto& item : list) {
        const double val = run_list_callback<double>(ctx, scr, value_start, value_end, input_type, item);
        if (!found) { ret = val; found = true; }
        else if (kind == container::list_pipeline_kind::min) ret = std::min(ret, val);
        else if (kind == container::list_pipeline_kind::max) ret = std::max(ret, val);
        sum += val;
        count += 1;
      }
      if (!found) {
        const auto def = run_default_callback(ctx, scr, default_start, default_end);
        ctx->stack.push(def);
      } else {
        ctx->stack.push(kind == container::list_pipeline_kind::average ? sum / double(count) : ret);
      }
      break;
    }

    case container::list_pipeline_kind::first:
    case container::list_pipeline_kind::last: {
      int64_t found = -1;
      for (size_t i = 0; i < list.size(); ++i) {
        if (run_list_callback<bool>(ctx, scr, value_start, value_end, input_type, list[i])) {
          found = int64_t(i);
          if (kind == container::list_pipeline_kind::first) break;
        }
      }
      if (found >= 0) {
        ctx->stack.push(type, list[size_t(found)]);
      } else {
        const auto def = run_default_callback(ctx, scr, default_start, default_end);
        ctx->stack.push(def);
      }
      break;
    }
  }

  ctx->current_index = end - 1;
  return 0;
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}

