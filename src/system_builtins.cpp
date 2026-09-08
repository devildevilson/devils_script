#include "devils_script/system.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <format>
#include <optional>

namespace devils_script {

#define EPSILON 0.000001

namespace internal {
using script_uint_t = std::make_unsigned_t<script_int_t>;

// Integer arithmetic wraps, two's complement, like the hardware underneath it. Signed overflow is
// undefined in C++, so every operation goes through the unsigned type and back - which also gives
// constant folding a contract it can reproduce exactly at compile time.
static script_int_t rawaddi(const script_int_t val1, const script_int_t val2) noexcept { return script_int_t(script_uint_t(val1) + script_uint_t(val2)); }
static script_int_t rawmuli(const script_int_t val1, const script_int_t val2) noexcept { return script_int_t(script_uint_t(val1) * script_uint_t(val2)); }
static script_int_t rawsubi(const script_int_t val1, const script_int_t val2) noexcept { return script_int_t(script_uint_t(val1) - script_uint_t(val2)); }
static script_int_t rawposi(const script_int_t val1) noexcept { return +val1; }
static script_int_t rawnegi(const script_int_t val1) noexcept { return script_int_t(0u - script_uint_t(val1)); }
// Integer remainder. Division by zero and INT64_MIN % -1 are trapping in hardware, so both are
// rejected as script errors rather than left to the CPU.
static script_int_t rawmodi(const script_int_t val1, const script_int_t val2) {
  if (val2 == 0) throw std::runtime_error("Integer remainder by zero");
  if (val2 == -1) return 0;
  return val1 % val2;
}
static bool rawmorei(const script_int_t val1, const script_int_t val2) noexcept { return val1 > val2; }
static bool rawlessi(const script_int_t val1, const script_int_t val2) noexcept { return val1 < val2; }
static bool rawmoreeqi(const script_int_t val1, const script_int_t val2) noexcept { return val1 >= val2; }
static bool rawlesseqi(const script_int_t val1, const script_int_t val2) noexcept { return val1 <= val2; }
static script_float_t rawadd(const script_float_t val1, const script_float_t val2) noexcept { return val1 + val2; }
static script_float_t rawmul(const script_float_t val1, const script_float_t val2) noexcept { return val1 * val2; }
static script_float_t rawsub(const script_float_t val1, const script_float_t val2) noexcept { return val1 - val2; }
static script_float_t rawdiv(const script_float_t val1, const script_float_t val2) noexcept { return val1 / val2; }
static script_float_t rawmod(const script_float_t val1, const script_float_t val2) noexcept { return std::fmod(val1, val2); }
static script_float_t rawpos(const script_float_t val1) noexcept { return +val1; }
static script_float_t rawneg(const script_float_t val1) noexcept { return -val1; }
static bool rawnot(const bool val1) noexcept { return !val1; }

static bool rawmore(const script_float_t val1, const script_float_t val2) noexcept { return val1 > val2; }
static bool rawless(const script_float_t val1, const script_float_t val2) noexcept { return val1 < val2; }
static bool rawmoreeq(const script_float_t val1, const script_float_t val2) noexcept { return val1 >= val2; }
static bool rawlesseq(const script_float_t val1, const script_float_t val2) noexcept { return val1 <= val2; }

static script_float_t rawmax(const script_float_t val1, const script_float_t val2) noexcept { return std::max(val1, val2); }
static script_float_t rawmin(const script_float_t val1, const script_float_t val2) noexcept { return std::min(val1, val2); }
static script_float_t rawabs(const script_float_t val1) noexcept { return std::abs(val1); }
static script_float_t rawceil(const script_float_t val1) noexcept { return std::ceil(val1); }
static script_float_t rawfloor(const script_float_t val1) noexcept { return std::floor(val1); }
static script_float_t rawround(const script_float_t val1) noexcept { return std::round(val1); }
static script_float_t rawtrunc(const script_float_t val1) noexcept { return std::trunc(val1); }
static script_float_t rawexp(const script_float_t val1) noexcept { return std::exp(val1); }
static script_float_t rawsqrt(const script_float_t val1) noexcept { return std::sqrt(val1); }
static script_float_t rawinversesqrt(const script_float_t val1) noexcept { return 1.0 / std::sqrt(val1); }
static script_float_t rawsin(const script_float_t val1) noexcept { return std::sin(val1); }
static script_float_t rawcos(const script_float_t val1) noexcept { return std::cos(val1); }
static script_float_t rawasin(const script_float_t val1) noexcept { return std::asin(val1); }
static script_float_t rawacos(const script_float_t val1) noexcept { return std::acos(val1); }
static script_float_t rawtan(const script_float_t val1) noexcept { return std::tan(val1); }
static script_float_t rawatan(const script_float_t val1) noexcept { return std::atan(val1); }
static script_float_t rawinc(const script_float_t val1) noexcept { return val1 + 1.0; }
static script_float_t rawdec(const script_float_t val1) noexcept { return val1 - 1.0; }
static script_float_t rawinv(const script_float_t val1) noexcept { return 1.0 / val1; }
// The only way back from floating-point to integer. It is explicit on purpose: `/` always produces
// a floating-point value, and an implicit narrowing rule would put the silent precision loss this
// type system just removed straight back in.
static script_int_t rawtoint(const script_float_t val1) {
  if (!(val1 >= -9223372036854775808.0 && val1 < 9223372036854775808.0))
    throw std::runtime_error(std::format("'to_int' cannot represent {} as an integer", val1));
  return script_int_t(val1);
}

static bool raweqb(const bool val1, const bool val2) noexcept { return val1 == val2; }
static bool raweqi(const script_int_t val1, const script_int_t val2) noexcept { return val1 == val2; }
static bool raweqd(const script_float_t val1, const script_float_t val2) noexcept { return std::abs(val1 - val2) < EPSILON; }
static bool raweqs(const std::string_view& val1, const std::string_view& val2) noexcept { return val1 == val2; }
// Equality is compiled by hand, so unlike an ordinary call it cannot convert its first operand -
// by the time both types are known that operand is already buried under the second. Comparing an
// integer against a floating-point value therefore gets its own opcodes instead of a conversion.
static bool raweqid(const script_int_t val1, const script_float_t val2) noexcept { return raweqd(script_float_t(val1), val2); }
static bool raweqdi(const script_float_t val1, const script_int_t val2) noexcept { return raweqd(val1, script_float_t(val2)); }
static bool raweq (const element_view& val1, const element_view& val2) noexcept { 
  return val1 == val2;
}

static bool operator_or(const bool val1, const bool val2) noexcept { return val1 || val2; }
static bool operator_and(const bool val1, const bool val2) noexcept { return val1 && val2; }

static script_float_t rawsign(const script_float_t v1) noexcept { return v1 > 0.0 ? 1.0 : (v1 < 0.0 ? -1.0 : 0.0); }
static script_float_t rawfma(const script_float_t v1, const script_float_t v2, const script_float_t v3) noexcept { return v1 * v2 + v3; }
static script_float_t rawfract(const script_float_t v1) noexcept { return v1 - rawfloor(v1); }
static script_float_t rawmix(const script_float_t v1, const script_float_t v2, const script_float_t v3) noexcept { return v1 * (1.0 - v3) + v2 * v3; }
static script_float_t rawsclamp(const script_float_t t, const script_float_t v1, const script_float_t v2) noexcept { return std::clamp(t, v1, v2);  }
static script_float_t rawsmoothstep(const script_float_t v1, const script_float_t v2, const script_float_t x) noexcept {
  if (x <= v1) return 0.0;
  if (x >= v2) return 1.0;
  const script_float_t t = rawsclamp((x - v1) / (v2 - v1), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}
static script_float_t rawstep(const script_float_t v1, const script_float_t x) noexcept { return x < v1 ? 0.0 : 1.0; }
static script_float_t rawrndmix1(const script_float_t v1) noexcept { return prng::prng_normalize(prng::mix(uint64_t(pack_float(v1)))); }
static script_float_t rawrndmix(const script_float_t v1, const script_float_t v2) noexcept { return prng::prng_normalize(prng::mix(uint64_t(pack_float(v1)), uint64_t(pack_float(v2)))); }
}
#define RFI(func) register_function<&func>
#define ROI(func) register_operator<&func>

namespace {
// Marks everything registered inside init_math() / init_basic_functions() as built-in, so constant
// folding can tell those registrations apart from a consumer's own overloads. Restores the previous
// value on scope exit, including when a registration raises.
struct builtin_registration_scope {
  bool* flag;
  bool prev;
  explicit builtin_registration_scope(bool& f) noexcept : flag(&f), prev(f) { f = true; }
  ~builtin_registration_scope() noexcept { *flag = prev; }
};
}

void system::init_math() {
  builtin_registration_scope builtins(registering_builtins);
  ROI(internal::rawposi)("unary_plus", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawnegi)("unary_minus", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawpos)("unary_plus", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawneg)("unary_minus", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawnot)("not", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawinc)("++", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawdec)("--", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawmuli)("*", { 12, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmul)("*", { 12, command_data::math_ftype::binary, command_data::associativity::left });
  // `/` deliberately has no integer overload: division is the one operation where the integer answer
  // is almost never the intended one, so both operands convert and the result is floating-point.
  ROI(internal::rawdiv)("/", { 12, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmodi)("%", { 12, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmod)("%", { 12, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawaddi)("+", { 11, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawsubi)("-", { 11, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawadd)("+", { 11, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawsub)("-", { 11, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmoreeqi)(">=", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawlesseqi)("<=", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmorei)(">", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawlessi)("<", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmoreeq)(">=", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawlesseq)("<=", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmore)(">", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawless)("<", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::operator_and)("and", { 3, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::operator_or)("or", { 2, command_data::math_ftype::binary, command_data::associativity::left });
  
  RFI(internal::rawmax)("max");
  RFI(internal::rawmin)("min");
  RFI(internal::rawabs)("abs");
  RFI(internal::rawceil)("ceil");
  RFI(internal::rawfloor)("floor");
  RFI(internal::rawround)("round");
  RFI(internal::rawtrunc)("trunc");
  RFI(internal::rawexp)("exp");
  RFI(internal::rawsqrt)("sqrt");
  RFI(internal::rawinversesqrt)("inversesqrt");
  RFI(internal::rawsin)("sin");
  RFI(internal::rawcos)("cos");
  RFI(internal::rawasin)("asin");
  RFI(internal::rawacos)("acos");
  RFI(internal::rawtan)("tan");
  RFI(internal::rawatan)("atan");
  RFI(internal::rawinv)("inv");
  RFI(internal::rawtoint)("to_int");

  RFI(internal::rawsign)("sign");
  RFI(internal::rawfma)("fma");
  RFI(internal::rawfract)("fract");
  RFI(internal::rawmix)("mix");
  RFI(internal::rawsclamp)("clamp");
  RFI(internal::rawsmoothstep)("smoothstep");
  RFI(internal::rawstep)("step");

  RFI(internal::rawrndmix1)("rndmix1");
  RFI(internal::rawrndmix)("rndmix");
}

namespace internal {
static any_stack value_or(const bool, const element_view&, const element_view&) { return any_stack{}; }
static any_object thisfn() { return any_object{}; }
static any_object prevfn() { return any_object{}; }
static any_stack selectfn(const bool, const element_view&) { return any_stack{}; }
static any_stack switchfn(const bool, const element_view&) { return any_stack{}; }

enum class ctx_value {};
static thisctx ctx() { return thisctx{}; }
static ignore_value ctx_save(const element_view&) { return ignore_value{}; }
static ignore_value ctx_save_as(ctx_value) { return ignore_value{}; }
static any_stack loadctx(thisctx, ctx_value) { return any_stack{}; }

enum class arg_value {};
static thisarg arg() { return thisarg{}; }

static ignore_value ctx_set(arg_value) { return ignore_value{}; }
static ignore_value ctx_set_as(arg_value) { return ignore_value{}; }

static thisctxlist list(const thisctx&, const std::string_view&) { return thisctxlist{}; }
static ignore_value add_to(const thisctxlist& l, const any_stack &val) {
  stack_element el;
  el.set(val.view());
  l.ctx->lists[l.idx].emplace_back(el);
  return ignore_value{};
}

static bool is_in(const thisctxlist& l, const any_stack& val) {
  const auto& list = l.ctx->lists[l.idx];
  size_t counter = 0;
  for (; counter < list.size() && memcmp(list[counter].mem, val._mem, MAXIMUM_STACK_VAL_SIZE) != 0; ++counter) {}
  return counter < list.size();
}

static ignore_value remove_from(const thisctxlist& l, const any_stack& val) {
  auto& list = l.ctx->lists[l.idx];
  size_t counter = 0;
  for (; counter < list.size() && memcmp(list[counter].mem, val._mem, MAXIMUM_STACK_VAL_SIZE) != 0; ++counter) {}
  list.erase(list.begin() + counter);
  return ignore_value{};
}

static script_float_t chance() { return 1; }
static any_stack randomfn(script_float_t, const element_view&) { return any_stack{}; }

// The command name is passed explicitly by the caller (assert/trace know their own name) — it is no
// longer looked up from a per-command table, which has been removed.
static std::string debug_environment(const context* ctx, const std::string_view fn, const size_t consumed_args) {
  const int64_t scope_index = int64_t(ctx->stack.size()) - int64_t(consumed_args) - 1;
  const auto scope = scope_index >= 0 ? ctx->stack.type(scope_index) : std::string_view();
  return std::format("function '{}', scope '{}'", fn, scope);
}

static int64_t debug_assert(int64_t, context* ctx, const script_container* scr) {
  const auto message = ctx->stack.safe_pop<std::string_view>();
  const bool condition = ctx->stack.safe_pop<bool>();
  if (!condition) scr->error_at(ctx, std::format("assert failed: '{}' ({})", message, debug_environment(ctx, "assert", 2)));
  return -2;
}

static int64_t debug_trace(int64_t, context* ctx, const script_container* scr) {
  const auto message = ctx->stack.safe_pop<std::string_view>();
  const auto loc = scr->loc_at(ctx);
  ctx->trace(std::format("Script trace @ {}:{}: '{}' ({})", loc.line, loc.column, message, debug_environment(ctx, "trace", 1)));
  return -1;
}
// Grow ctx->lists by `arg` fresh empty slots: the sub-script's list frame. Emitted by the execute
// builtin right before the call when the target uses lists; paired with list_frame_exit afterwards.
static int64_t list_frame_enter(int64_t arg, context* ctx, const script_container*) {
  ctx->lists.resize(ctx->lists.size() + size_t(arg));
  return 0;
}

// Drop the top `arg` list slots — the sub-script's list frame — once the call has returned.
static int64_t list_frame_exit(int64_t arg, context* ctx, const script_container*) {
  ctx->lists.resize(ctx->lists.size() - size_t(arg));
  return 0;
}

// In/out list binding (path B), emitted both BEFORE and AFTER the execute opcode (double-swap, O(1)).
// `arg` packs (top_off, caller_index): the sub-script's list slot sits `top_off` entries below the
// end of ctx->lists (independent of any base, since list_frame_enter just appended it), and the
// caller's list is `caller_index + ctx->list_base`. Both swaps run while ctx->list_base is the
// CALLER's base — the execute opcode sets and restores its own list_base internally — so the pre-swap
// moves the caller's contents into the sub's slot and the post-swap moves the sub's mutations back.
static int64_t swap_bound_list(int64_t arg, context* ctx, const script_container*) {
  const auto [top_off, caller_index] = unpack2(arg);
  std::swap(ctx->lists[ctx->lists.size() - size_t(top_off)], ctx->lists[size_t(caller_index) + ctx->list_base]);
  return 0;
}

// Index of the just-returned sub-script's argument frame. The sub's arg slots survive the execute
// opcode (args are frame-local — no snapshot/restore), so an in/out write-back runs AFTER execute
// while ctx->arg_base/current_script are the CALLER's: the sub's frame sits at caller_arg_base +
// caller's arg count. (Bounded by script_arguments_size, guaranteed at parse + by execute's guard.)
static inline size_t returned_sub_arg_base(const context* ctx, const script_container* scr) {
  return ctx->arg_base + scr->args.size();
}

// In/out scalar write-back: copy the sub-script's final value of arg slot `sub_slot` (high half of
// arg) into the caller's saved slot (caller_index + ctx->saved_base). Emitted after execute for a
// `name = ctx:saved:x` binding.
static int64_t writeback_arg_to_saved(int64_t arg, context* ctx, const script_container* scr) {
  const auto [sub_slot, caller_index] = unpack2(arg);
  ctx->set_saved(size_t(caller_index) + ctx->saved_base, ctx->get_arg<any_stack>(returned_sub_arg_base(ctx, scr) + size_t(sub_slot)));
  return 0;
}

// In/out scalar write-back into the caller's arg slot (caller_index + ctx->arg_base). Emitted after
// execute for a `name = ctx:arg:x` binding.
static int64_t writeback_arg_to_arg(int64_t arg, context* ctx, const script_container* scr) {
  const auto [sub_slot, caller_index] = unpack2(arg);
  ctx->set_arg(size_t(caller_index) + ctx->arg_base, ctx->get_arg<any_stack>(returned_sub_arg_base(ctx, scr) + size_t(sub_slot)));
  return 0;
}

// Runs a resolved sub-script in the current context as a script-in-script call. The target sub-script
// pointer is packed directly into `arg` (caller-owned, outlives the call — same lifetime model as the
// resolver). The call frame is just base offsets + a little transient state in C++ locals; nothing is
// copied. The main stack is NOT copied — `ctx->frame_base` is bumped to the live stack top so the
// sub-script's compiled (zero-based, absolute) stack indices resolve above the parent's frame. Args,
// saved values and lists are all frame-local via arg_base/saved_base/list_base, so a sub never
// clobbers the caller's slots and needs no snapshot. The sub-script's arguments (root first when
// present, then named args in slot order) sit on the stack top; here we move them into args_stack.
// List-frame append/shrink and in/out swaps/write-backs are handled by the surrounding opcodes.
static int64_t execute_script(int64_t arg, context* ctx, const script_container* scr) {
  const auto* sub = reinterpret_cast<const script_container*>(static_cast<intptr_t>(arg));
  if (ctx->describing) throw context::description_unavailable{};
  const auto old_index = ctx->current_index;
  const auto old_frame = ctx->frame_base;
  const auto old_arg = ctx->arg_base;
  const auto old_saved = ctx->saved_base;
  const auto old_list = ctx->list_base;
  const auto old_return = ctx->_return_value;
  const auto old_size = ctx->stack.size();
  try {
  const size_t nargs = sub->args.size();

  // Arguments are frame-local: the sub's arg frame sits above the caller's, so the caller's slots are
  // never touched and the sub's final values survive for the in/out write-back opcodes.
  const size_t new_arg_base = ctx->arg_base + scr->args.size();
  if (new_arg_base + nargs > context::script_arguments_size)
    scr->error_at(ctx, std::format("script-in-script argument frame overflow: '{}' needs {} arg slots at base {}, only {} available", sub->get_name(), nargs, new_arg_base, context::script_arguments_size));
  const size_t nsaved = sub->saved.size();
  // Saved values are frame-local: the sub-script's saved frame sits ABOVE the caller's.
  const size_t new_saved_base = ctx->saved_base + scr->saved.size();
  // Bound against the context's ACTUAL saved-stack size (runtime-configurable via context's ctor),
  // not the constexpr default — a larger context legitimately holds deeper saved frames.
  if (new_saved_base + nsaved > ctx->saved_stack.size())
    scr->error_at(ctx, std::format("script-in-script saved-value frame overflow: '{}' needs {} saved slots at base {}, only {} available", sub->get_name(), nsaved, new_saved_base, ctx->saved_stack.size()));
  // Lists are frame-local: the sub's list frame is the top `nlists` slots of ctx->lists, already
  // appended by the preceding list_frame_enter opcode (and dropped by list_frame_exit afterwards).
  const size_t nlists = sub->lists.size();
  const size_t new_list_base = ctx->lists.size() - nlists;
  const size_t saved_list_base = ctx->list_base;

  const size_t saved_index = ctx->current_index;
  const size_t saved_frame_base = ctx->frame_base;
  const size_t saved_arg_base = ctx->arg_base;
  const size_t saved_saved_base = ctx->saved_base;
  const any_stack saved_return = ctx->_return_value;

  // Move the argument values (top `nargs` of the stack, in slot order) into the sub's arg frame.
  const size_t base = ctx->stack._size - nargs;
  for (size_t k = 0; k < nargs; ++k) {
    ctx->args_stack._data[new_arg_base + k] = ctx->stack._data[base + k];
    ctx->args_stack._types[new_arg_base + k] = ctx->stack._types[base + k];
  }
  ctx->stack._size = base;       // consume the arguments; sub-script runs above this point

  ctx->frame_base = base;        // zero-based sub indices resolve relative to the live stack top
  ctx->arg_base = new_arg_base;      // sub's arg slots sit above the caller's frame
  ctx->saved_base = new_saved_base;  // sub's saved slots sit above the caller's frame
  ctx->list_base = new_list_base;    // sub's lists sit above the caller's in ctx->lists
  // The sub-script starts with a fresh saved frame: clear its slot types so a read before write is
  // caught by the usual type check rather than seeing a parent's stale value.
  for (size_t i = 0; i < nsaved; ++i) ctx->saved_stack._types[new_saved_base + i] = std::string_view();
  ctx->current_index = 0;
  sub->process(ctx);             // process() saves/restores current_script itself

  const any_stack result = ctx->_return_value;

  // Restore the parent frame. The sub's arg slots are left as-is (scratch above the caller's frame) so
  // the in/out write-back opcodes that follow can still read them.
  ctx->stack._size = base;
  ctx->frame_base = saved_frame_base;
  ctx->arg_base = saved_arg_base;
  ctx->saved_base = saved_saved_base;
  ctx->list_base = saved_list_base;
  ctx->_return_value = saved_return;
  ctx->current_index = saved_index;

  if (!type_is_void(sub->return_type)) {
    stack_element rel;
    memcpy(rel.mem, result._mem, MAXIMUM_STACK_VAL_SIZE);
    ctx->stack.push(result.type(), rel);
    return int64_t(1) - int64_t(nargs);
  }
  return -int64_t(nargs);
  } catch (...) {
    ctx->current_index = old_index;
    ctx->frame_base = old_frame;
    ctx->arg_base = old_arg;
    ctx->saved_base = old_saved;
    ctx->list_base = old_list;
    ctx->_return_value = old_return;
    ctx->stack._size = old_size - sub->args.size();
    // The compiler emits scalar writebacks, list swaps, then list_frame_exit immediately
    // after execute. Unwind that list frame even if entry validation or the callee failed.
    // Completed list mutations are retained; scalar writebacks only happen on success.
    for (size_t i = old_index + 1; i < scr->cmds.size(); ++i) {
      const auto& cmd = scr->cmds[i];
      if (cmd.fp == &writeback_arg_to_saved || cmd.fp == &writeback_arg_to_arg) continue;
      if (cmd.fp == &swap_bound_list) { swap_bound_list(cmd.arg, ctx, scr); continue; }
      if (cmd.fp == &list_frame_exit) list_frame_exit(cmd.arg, ctx, scr);
      break;
    }
    throw;
  }
}

static any_stack executefn() { return any_stack{}; }
}

template <auto f>
static void add_cmd(const system* sys, container* scr) {
  constexpr function_t fs[] = { &mathfunc_unsafe<f>, &mathfunc<f> };
  scr->cmds.emplace_back(fs[size_t(sys->safety())], INT64_C(0));
}

#define ADD_CMD(fn) add_cmd<fn>

void system::init_basic_functions() {
  builtin_registration_scope builtins(registering_builtins);
  RFI(internal::operator_and)("AND", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    return sys->fold_block(ctx, scr, args, basicf::AND);
  });
  RFI(internal::operator_or)("OR", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    return sys->fold_block(ctx, scr, args, basicf::OR);
  });
  RFI(internal::operator_and)("NAND", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    return sys->fold_block(ctx, scr, args, basicf::NAND);
  });
  RFI(internal::operator_or)("NOR", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    return sys->fold_block(ctx, scr, args, basicf::NOR);
  });
  RFI(internal::rawaddi)("ADD");
  RFI(internal::rawmuli)("MUL");
  RFI(internal::rawadd)("ADD");
  RFI(internal::rawmul)("MUL");

  RFI(internal::ctx_save)("assert", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    size_t offset = 1;
    auto cond = command_block(args, offset);
    if (cond.name() == custom_description_constant) { offset += cond.size(); cond = command_block(args, offset); }
    offset += sys->parse_arg<bool>(ctx, scr, cond, 0, utils::type_name<bool>(), std::string_view(), {});

    auto message = command_block(args, offset);
    if (message.name() == custom_description_constant) { offset += message.size(); message = command_block(args, offset); }
    const auto message_text = sys->static_string_arg(message, args.name());
    sys->push_string(ctx, scr, message_text);
    offset += message.size();
    if (offset < args.size()) sys->raise_error(std::format("Too many arguments for function '{}'", args.name()));

    scr->cmds.emplace_back(&internal::debug_assert, INT64_C(0));
    ctx->pop();
    ctx->pop();
    return args.size();
  });

  RFI(internal::ctx_save)("trace", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    size_t offset = 1;
    auto message = command_block(args, offset);
    if (message.name() == custom_description_constant) { offset += message.size(); message = command_block(args, offset); }
    const auto message_text = sys->static_string_arg(message, args.name());
    sys->push_string(ctx, scr, message_text);
    offset += message.size();
    if (offset < args.size()) sys->raise_error(std::format("Too many arguments for function '{}'", args.name()));

    scr->cmds.emplace_back(&internal::debug_trace, INT64_C(0));
    ctx->pop();
    return args.size();
  });

  // Script-in-script call. Syntax: `execute = { script_name, arg1 = expr, ... }` (or
  // `execute = script_name` with no args). The script name resolves through the system's
  // script_resolver to a pre-compiled, caller-owned sub-script. Named arguments are matched by
  // name (and type) to the sub-script's declared `arg:get` arguments; if the sub-script has a root
  // scope it is fed implicitly from the caller's current scope. The return type is checked against
  // the caller's context. The matched values are emitted in declared-slot order so the runtime
  // handler can read them straight off the stack top.
  RFI(internal::executefn)("execute", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;

    size_t offset = 1;
    auto name_block = command_block(args, offset);
    if (name_block.name() == custom_description_constant) { offset += name_block.size(); name_block = command_block(args, offset); }
    if (name_block.empty() || name_block.args_count() != 0 || name_block.size() != 1)
      sys->raise_error("'execute' expects a script name as its first argument");
    const auto script_name = name_block.name();
    offset += name_block.size();

    const script_container* sub = sys->resolve_script(script_name);
    if (sub == nullptr) sys->raise_error(std::format("'execute' could not resolve script '{}' (is a script_resolver installed?)", script_name));
    if (sub == static_cast<const script_container*>(scr)) sys->raise_error(std::format("'execute' cannot call the script currently being compiled ('{}')", script_name));

    const bool has_root = !sub->args.empty() && sub->args[0].name.count == SIZE_MAX;
    const size_t named_base = has_root ? 1 : 0;

    // The sub-script's root (slot 0) is fed implicitly from the caller's current scope: push a copy
    // of it onto the stack here so it becomes the first argument value the opcode consumes.
    if (has_root) {
      if (ctx->scope_stack.empty())
        sys->raise_error(std::format("'execute' target '{}' needs root scope '{}' but no scope is active", script_name, sub->args[0].type));
      const auto cur_type = ctx->current_scope_type();
      if (cur_type != sub->args[0].type)
        sys->raise_error(std::format("'execute' target '{}' expects root scope '{}', but current scope is '{}'", script_name, sub->args[0].type, cur_type));
      sys->push_basic_function(ctx, scr, basicf::pushthis, ctx->current_scope_index());
      ctx->push(cur_type);
    }

    // Collect provided children. `name = <value>` (args_count 1) is a scalar argument; a bare `name`
    // (args_count 0) is an in/out LIST binding: the caller's list `ctx:list:name` is bound by name to
    // the sub-script's same-named list, and the two are double-swapped around the call at runtime so
    // the sub's mutations land in the caller's list. A sub list left unbound here stays sub-local
    // scratch (path A).
    std::vector<std::pair<std::string_view, command_block>> provided;
    struct list_bind { size_t sub_slot; size_t caller_idx; };
    std::vector<list_bind> list_bindings;
    while (offset < args.size()) {
      auto child = command_block(args, offset);
      offset += child.size();
      if (child.name() == custom_description_constant) continue;

      if (child.args_count() == 0) {
        const auto lname = child.name();
        const size_t sub_slot = sub->find_list(lname);
        if (sub_slot >= sub->lists.size())
          sys->raise_error(std::format("'execute' target '{}' has no list parameter '{}'", script_name, lname));
        for (const auto& b : list_bindings) if (b.sub_slot == sub_slot)
          sys->raise_error(std::format("'execute' list parameter '{}' bound more than once", lname));

        size_t caller_idx = scr->find_list(lname);
        if (caller_idx >= scr->lists.size()) {
          scr->lists.push_back({ sys->store_string(scr, lname), sub->lists[sub_slot].type });
          caller_idx = scr->lists.size() - 1;
        } else if (scr->lists[caller_idx].type.empty()) {
          scr->lists[caller_idx].type = sub->lists[sub_slot].type;
        } else if (!sub->lists[sub_slot].type.empty() && scr->lists[caller_idx].type != sub->lists[sub_slot].type) {
          sys->raise_error(std::format("'execute' list '{}' type mismatch: caller holds '{}', target '{}' expects '{}'",
            lname, scr->lists[caller_idx].type, script_name, sub->lists[sub_slot].type));
        }
        // Forbid binding the list this script is currently iterating (we are inside its pipeline
        // callback): the sub would mutate the live list mid-iteration, and the running pipeline holds
        // a reference/iterators into it. list_index_upvalue is SIZE_MAX outside a callback.
        if (caller_idx == ctx->list_index_upvalue)
          sys->raise_error(std::format("'execute' target '{}' cannot bind list '{}' while it is being iterated", script_name, lname));
        list_bindings.push_back({ sub_slot, caller_idx });
        continue;
      }

      if (child.args_count() != 1)
        sys->raise_error(std::format("'execute' argument '{}' must be of the form '{} = <value>'", child.name(), child.name()));
      provided.emplace_back(child.name(), command_block(child, 1));
    }

    // Detect an in/out scalar: a value that is EXACTLY a bare caller lvalue path `ctx:saved:X` /
    // `ctx:arg:X` (a single scope-path block whose full name is that path). Such a value is still read
    // by-value as the sub's input, but after the call the sub's final arg value is written back into
    // that caller slot. Anything else (a computed expression) is plain by-value.
    const auto inout_target = [](const command_block& v) -> std::optional<std::pair<bool, std::string_view>> {
      if (v.args_count() != 0 || v.size() != 1) return std::nullopt;
      const auto n = v.name();
      constexpr std::string_view sp = "ctx:saved:", ap = "ctx:arg:";
      std::string_view var; bool is_saved;
      if (n.starts_with(sp)) { var = n.substr(sp.size()); is_saved = true; }
      else if (n.starts_with(ap)) { var = n.substr(ap.size()); is_saved = false; }
      else return std::nullopt;
      if (var.empty() || var.find(':') != std::string_view::npos) return std::nullopt;  // direct slot only
      return std::make_pair(is_saved, var);
    };
    struct inout_bind { size_t sub_slot; bool is_saved; size_t caller_idx; };
    std::vector<inout_bind> scalar_inout;

    const size_t nargs = sub->args.size();
    size_t matched = 0;
    for (size_t slot = named_base; slot < nargs; ++slot) {
      const auto arg_name = sub->get_arg_name(slot);
      const auto expected = sub->args[slot].type;

      const command_block* value = nullptr;
      for (auto& [pname, pblock] : provided) if (pname == arg_name) { value = &pblock; break; }
      if (value == nullptr) sys->raise_error(std::format("'execute' target '{}' is missing argument '{}'", script_name, arg_name));
      matched += 1;

      const auto inout = inout_target(*value);

      {
        set_expected_type set(ctx, expected);
        sys->dispatch_node(ctx, scr, *value);
      }
      const auto top = ctx->top();
      if (top != expected) {
        const bool top_num = type_is_bool(top) || type_is_fundamental(top);
        const bool exp_num = type_is_bool(expected) || type_is_fundamental(expected);
        if (top_num && exp_num) {
          // An in/out target must round-trip into the same caller slot, so a silent numeric
          // conversion would corrupt its stored type — require an exact match instead.
          if (inout) sys->raise_error(std::format("'execute' in/out argument '{}' of '{}' must match type '{}' exactly, got '{}'", arg_name, script_name, expected, top));
          if (type_is_bool(expected)) {
            if (type_is_integral(top)) sys->setup_type_conversion<int64_t, bool>(ctx, scr);
            else if (type_is_floating_point(top)) sys->setup_type_conversion<double, bool>(ctx, scr);
          } else if (type_is_integral(expected)) {
            if (type_is_bool(top)) sys->setup_type_conversion<bool, int64_t>(ctx, scr);
            else if (type_is_floating_point(top)) sys->setup_type_conversion<double, int64_t>(ctx, scr);
          } else if (type_is_floating_point(expected)) {
            if (type_is_bool(top)) sys->setup_type_conversion<bool, double>(ctx, scr);
            else if (type_is_integral(top)) sys->setup_type_conversion<int64_t, double>(ctx, scr);
          }
        } else {
          sys->raise_error(std::format("'execute' argument '{}' of '{}' expects type '{}', got '{}'", arg_name, script_name, expected, top));
        }
      }

      if (inout) {
        const auto [is_saved, var] = *inout;
        // The dispatch above read (and for ctx:arg, ensured the existence of) the caller slot.
        const size_t caller_idx = is_saved ? scr->find_saved(var) : scr->find_arg(var);
        scalar_inout.push_back({ slot, is_saved, caller_idx });
      }
    }
    if (matched != provided.size())
      sys->raise_error(std::format("'execute' target '{}' received an unknown argument", script_name));

    const auto ret = sub->return_type;
    if (!type_is_void(ret) && type_is_void(ctx->expected_type))
      sys->raise_error(std::format("'execute' target '{}' returns '{}' but is used in an effect context", script_name, ret));

    // Emit the call as a short opcode sequence. The sub-script pointer is packed straight into the
    // execute opcode's arg (no side table). When the target uses lists, a list_frame_enter/exit pair
    // brackets the call to append/drop the sub's list frame, and each in/out binding emits a
    // swap_bound_list both before and after the execute opcode (double-swap). list_*/swap opcodes have
    // no net stack effect, so the parse-time stack model only accounts for the execute opcode itself.
    const size_t nlists = sub->lists.size();
    const auto swap_arg = [&](const list_bind& b) { return pack2(int32_t(nlists - b.sub_slot), int32_t(b.caller_idx)); };

    if (nlists > 0) scr->cmds.emplace_back(container::command(&internal::list_frame_enter, int64_t(nlists)));
    for (const auto& b : list_bindings) scr->cmds.emplace_back(container::command(&internal::swap_bound_list, swap_arg(b)));
    scr->cmds.emplace_back(container::command(&internal::execute_script, int64_t(reinterpret_cast<intptr_t>(sub))));
    // In/out scalar write-backs: read while the sub's arg frame is still live (before list_frame_exit,
    // which is unrelated, but ordering is harmless either way).
    for (const auto& b : scalar_inout)
      scr->cmds.emplace_back(container::command(
        b.is_saved ? &internal::writeback_arg_to_saved : &internal::writeback_arg_to_arg,
        pack2(int32_t(b.sub_slot), int32_t(b.caller_idx))));
    for (const auto& b : list_bindings) scr->cmds.emplace_back(container::command(&internal::swap_bound_list, swap_arg(b)));
    if (nlists > 0) scr->cmds.emplace_back(container::command(&internal::list_frame_exit, int64_t(nlists)));

    // Account for the sub-script's transient usage stacked above this frame. At runtime the sub runs
    // with frame_base = stack_top - nargs, so its peak operand depth is that base plus sub->max_stack;
    // its saved frame stacks on top of every caller's, tracked as the deepest child for max_saved.
    const size_t exec_base = ctx->stack_types.size() - nargs;
    if (exec_base + sub->max_stack > ctx->max_stack_depth) ctx->max_stack_depth = exec_base + sub->max_stack;
    if (sub->max_saved > ctx->max_child_saved) ctx->max_child_saved = sub->max_saved;
    if (sub->max_lists > ctx->max_child_lists) ctx->max_child_lists = sub->max_lists;

    for (size_t k = 0; k < nargs; ++k) ctx->pop();   // consume root (if any) + named arguments
    if (!type_is_void(ret)) ctx->push(ret);
    else ctx->push<ignore_value>();

    return args.size();
  });

  RFI(internal::value_or)("value_or", {}, [](emitter& e, const command_block& args, const std::vector<std::string> &) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    if (ctx->ftype != function_type::lvalue) sys->raise_error(std::format("'value_or' expected to be lvalue"));
    if (type_is_void(ctx->expected_type)) sys->raise_error(std::format("Could not use 'value_or' in this context, is it effect block?"));


    size_t offset = 1;
    do { 
      auto cb = command_block(args, offset);
      if (cb.name() == custom_description_constant) { offset += cb.size(); cb = command_block(args, offset); }
      offset += sys->parse_arg<0, 0, decltype(&internal::value_or)>(ctx, scr, cb, utils::type_name<bool>(), std::string_view(), {});
    } while (ctx->pop_while_ignore());

    auto else_branch = e.make_label();   // first arg false -> skip the second arg, take the default
    e.jump_to(basicf::condjump, else_branch);

    do {
      auto cb = command_block(args, offset);
      if (cb.name() == custom_description_constant) { offset += cb.size(); cb = command_block(args, offset); }
      offset += sys->parse_arg<1, 1, decltype(&internal::value_or)>(ctx, scr, cb, ctx->expected_type, std::string_view(), {});
    } while (ctx->pop_while_ignore());
    const auto second_arg_value_type = ctx->top();
    auto end = e.make_label();
    e.jump_to(basicf::jump, end);
    e.bind(else_branch);

    ctx->pop();

    do { 
      auto cb = command_block(args, offset);
      if (cb.name() == custom_description_constant) { offset += cb.size(); cb = command_block(args, offset); }
      offset += sys->parse_arg<2, 2, decltype(&internal::value_or)>(ctx, scr, cb, second_arg_value_type, std::string_view(), {});
    } while (ctx->pop_while_ignore());
    const auto third_arg_value_type = ctx->top();
    e.bind(end);

    ctx->pop();

    ctx->push(ctx->expected_type);

    if (!type_is_any_stack(second_arg_value_type) && !type_is_any_object(second_arg_value_type) && second_arg_value_type != third_arg_value_type) {
      if ((type_is_bool(second_arg_value_type) || type_is_fundamental(second_arg_value_type)) !=
        (type_is_bool(third_arg_value_type) || type_is_fundamental(third_arg_value_type)))
        sys->raise_error(std::format("Its expected that 'value_or' receives equal types as input for second and third argument, received: {} {}", second_arg_value_type, third_arg_value_type));

      if (type_is_bool(second_arg_value_type)           && type_is_floating_point(third_arg_value_type)) sys->setup_type_conversion<double, bool>(ctx, scr);
      if (type_is_bool(second_arg_value_type)           && type_is_integral(third_arg_value_type))       sys->setup_type_conversion<int64_t, bool>(ctx, scr);
      if (type_is_floating_point(second_arg_value_type) && type_is_bool(third_arg_value_type))           sys->setup_type_conversion<bool, double>(ctx, scr);
      if (type_is_floating_point(second_arg_value_type) && type_is_integral(third_arg_value_type))       sys->setup_type_conversion<int64_t, double>(ctx, scr);
      if (type_is_integral(second_arg_value_type)       && type_is_bool(third_arg_value_type))           sys->setup_type_conversion<bool, int64_t>(ctx, scr);
      if (type_is_integral(second_arg_value_type)       && type_is_floating_point(third_arg_value_type)) sys->setup_type_conversion<double, int64_t>(ctx, scr);
    }

    return args.size();
  });
  
  RFI(internal::thisfn)("this", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    if (ctx->scope_stack.size() == 0) sys->raise_error(std::format("Function 'this' requires at least 1 element in scope_stack"));
    sys->push_basic_function(ctx, scr, basicf::pushthis, ctx->current_scope_index());
    ctx->push(ctx->current_scope_type());

    if (args.size() == 1) return args.size();

    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->fold_block(ctx, scr, args, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    return args.size();
  });
  
  RFI(internal::prevfn)("prev", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    if (ctx->scope_stack.size() <= 1 + ctx->prev_chaining) sys->raise_error(std::format("Function 'prev' requires at least 2 elements in scope_stack"));

    // `prev_chaining` lets repeated `prev.prev` walk outward through scope_stack.
    const size_t scope_index = ctx->scope_stack[((ctx->scope_stack.size()-1)-ctx->prev_chaining)-1];
    sys->push_basic_function(ctx, scr, basicf::pushprev, scope_index);
    ctx->push(ctx->stack_types[scope_index]);

    if (args.size() == 1) return args.size();

    change_chain_index cci(ctx);
    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->fold_block(ctx, scr, args, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    return args.size();
  });

  // Skip internal ctx/list scopes and return the nearest user-visible outer scope.
  RFI(internal::prevfn)("outer", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    size_t counter = 0;
    size_t fin_index = SIZE_MAX;
    for (auto itr = ctx->scope_stack.rbegin(); itr != ctx->scope_stack.rend(); ++itr) {
      const size_t idx = *itr;
      const auto &curtype = ctx->stack_types[idx];
      if (curtype != utils::type_name<internal::thisarg>() && 
          curtype != utils::type_name<internal::thisctx>() && 
          curtype != utils::type_name<internal::thisctxlist>()
      ) {
        fin_index = idx;
        break;
      }

      counter += 1;
    }

    if (fin_index == SIZE_MAX) throw std::runtime_error(std::format("'outer' could not find any valuable scope in this context"));

    sys->push_basic_function(ctx, scr, basicf::pushprev, fin_index);
    ctx->push(ctx->stack_types[fin_index]);

    if (args.size() == 1) return args.size();

    const size_t prev_value = ctx->prev_chaining;
    ctx->prev_chaining += counter;
    
    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->fold_block(ctx, scr, args, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    ctx->prev_chaining = prev_value;

    return args.size();
  });

  // Equality is type-directed because script values can be bool, number, string, or object handles.
  const auto eqfn = [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto expected = utils::type_name<element_view>();
    size_t offset = 1;
    do { 
      auto cb = command_block(args, offset);
      if (cb.name() == custom_description_constant) { offset += cb.size(); cb = command_block(args, offset); }
      offset += sys->parse_arg<element_view>(ctx, scr, cb, 0, expected, std::string_view(), {});
    } while (ctx->pop_while_ignore());
    const auto first_type = ctx->top();

    do {
      auto cb = command_block(args, offset);
      if (cb.name() == custom_description_constant) { offset += cb.size(); cb = command_block(args, offset); }
      offset += sys->parse_arg<element_view>(ctx, scr, cb, 1, expected, std::string_view(), {});
    } while (ctx->pop_while_ignore());
    const auto second_type = ctx->top();

    if (type_is_bool(first_type) && type_is_bool(second_type)) {
      ADD_CMD(internal::raweqb)(sys, scr);
    } else if (type_is_integral(first_type) && type_is_integral(second_type)) {
      ADD_CMD(internal::raweqi)(sys, scr);
    } else if (type_is_floating_point(first_type) && type_is_floating_point(second_type)) {
      ADD_CMD(internal::raweqd)(sys, scr);
    } else if (type_is_integral(first_type) && type_is_floating_point(second_type)) {
      ADD_CMD(internal::raweqid)(sys, scr);
    } else if (type_is_floating_point(first_type) && type_is_integral(second_type)) {
      ADD_CMD(internal::raweqdi)(sys, scr);
    } else if ((type_is_bool(first_type) || type_is_fundamental(first_type)) && (type_is_bool(second_type) || type_is_fundamental(second_type))) {
      if (type_is_bool(first_type)) sys->setup_type_conversion<bool, double>(ctx, scr);
      if (type_is_integral(first_type)) sys->setup_type_conversion<int64_t, double>(ctx, scr);
      if (type_is_bool(second_type)) sys->setup_type_conversion<bool, double>(ctx, scr);
      if (type_is_integral(second_type)) sys->setup_type_conversion<int64_t, double>(ctx, scr);
      ADD_CMD(internal::raweqd)(sys, scr);
    } else if (type_is_string(first_type) && type_is_string(second_type)) {
      ADD_CMD(internal::raweqs)(sys, scr);
    } else {
      if (first_type != second_type) sys->raise_error(std::format("Could not make an equality check on different types '{}' and '{}'", first_type, second_type));
      ADD_CMD(internal::raweq)(sys, scr);
    }

    ctx->pop();
    ctx->pop();
    ctx->push<bool>();

    return args.size();
  };

  RFI(internal::raweq)("EQ", {}, [eqfn](emitter& e, const command_block& args, const std::vector<std::string>& func_args_names) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    return std::invoke(eqfn, e, args, func_args_names);
  });

  const operator_props op_specs = { 7, command_data::math_ftype::binary, command_data::associativity::left };
  ROI(internal::raweq)("==", op_specs, [eqfn](emitter& e, const command_block& args, const std::vector<std::string>& func_args_names) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    return std::invoke(eqfn, e, args, func_args_names);
  });

  RFI(internal::raweq)("NEQ", {}, [eqfn](emitter& e, const command_block& args, const std::vector<std::string>& func_args_names) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const size_t count = std::invoke(eqfn, e, args, func_args_names);
    e.emit(basicf::notfn, 0);
    return count;
  });

  ROI(internal::raweq)("!=", op_specs, [eqfn](emitter& e, const command_block& args, const std::vector<std::string>& func_args_names) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const size_t count = std::invoke(eqfn, e, args, func_args_names);
    e.emit(basicf::notfn, 0);
    return count;
  });

  // Find the first true condition and evaluate that block; the final block is the else branch.
  RFI(internal::selectfn)("select", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto exp = ctx->expected_type;
    if (type_is_string(exp) || type_is_object(exp)) sys->raise_error(std::format("Current language design makes 'select' meaningless in string and object blocks"));

    const bool requires_at_least_one_value = type_is_bool(exp) || type_is_fundamental(exp);

    auto end = e.make_label();   // a matched clause jumps past all the rest

    const auto kids = args.children();
    for (auto it = kids.begin(); it != kids.end(); ++it) {
      const auto block = *it;
      const bool last_block = it.is_last();

      const auto cond = block.find("condition");
      if (requires_at_least_one_value && !last_block && cond.empty()) sys->raise_error(std::format("Each script block in 'select' except last one requires 'condition'"));
      if (requires_at_least_one_value && last_block && !cond.empty()) sys->raise_error(std::format("Last script block in 'select' must not contain 'condition'"));

      const bool has_cond = !cond.empty();
      if (has_cond) sys->dispatch_node(ctx, scr, cond, "AND");
      e.guarded_clause(end, has_cond, [&]{
        sys->dispatch_node(ctx, scr, block);
        if (!type_is_void(exp) && ctx->is_ignore()) sys->raise_error(std::format("Block in 'select' function returns 'ignore_value'"));
      });

      if (!type_is_void(exp)) ctx->pop();
    }

    if (!type_is_void(exp)) ctx->push(exp);

    e.bind(end);

    return args.size();
  });
  
  // Evaluate blocks while each condition is true; the first false condition exits.
  RFI(internal::selectfn)("sequence", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto exp = ctx->expected_type;
    if (type_is_string(exp) || type_is_object(exp)) sys->raise_error(std::format("Current language design makes 'sequence' meaningless in string and object blocks"));

    const bool exp_is_void = type_is_void(exp);
    const bool exp_is_bool = type_is_bool(exp);
    const bool exp_is_fund = type_is_fundamental(exp);

    if (!exp_is_void) {
      if (exp_is_bool) sys->push_basic_function(ctx, scr, basicf::pushbool, true);
      if (exp_is_fund) sys->push_basic_function(ctx, scr, basicf::pushvalue, std::bit_cast<int64_t>(0.0));
    }

    auto end = e.make_label();   // a failed condition jumps out of the whole sequence

    for (const auto& block : args.children()) {
      const auto cond = block.find("condition");
      if (cond.empty()) sys->raise_error(std::format("Each script block in 'sequence' requires 'condition'"));

      sys->dispatch_node(ctx, scr, cond, "AND"); // condition would generate a description
      e.jump_to(basicf::condjump, end);

      sys->dispatch_node(ctx, scr, block);
      if (!type_is_void(exp) && ctx->is_ignore()) sys->raise_error(std::format("Block in 'sequence' function returns 'ignore_value'"));

      if (exp_is_bool) {
        if (ctx->is<int64_t>()) sys->setup_type_conversion<int64_t, bool>(ctx, scr);
        if (ctx->is<double>()) sys->setup_type_conversion<double, bool>(ctx, scr);
        sys->push_basic_function(ctx, scr, basicf::andbin, 0);
      }

      if (exp_is_fund) {
        if (ctx->is<int64_t>()) sys->setup_type_conversion<int64_t, double>(ctx, scr);
        if (ctx->is<bool>()) sys->setup_type_conversion<bool, double>(ctx, scr);
        sys->push_basic_function(ctx, scr, basicf::sum, 0);
      }
    }

    e.bind(end);

    return args.size();
  });

  RFI(internal::switchfn)("switch", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto exp = ctx->expected_type;
    const bool exp_is_void = type_is_void(exp);

    auto emit_value = [&](const command_block& value_node) {
      if (value_node.empty()) sys->raise_error("'switch' case requires 'value'");
      if (value_node.args_count() != 1) sys->raise_error(std::format("'switch' value '{}' must contain exactly one expression", value_node.name()));
      const auto value_expr = command_block(value_node, 1);
      sys->dispatch_node(ctx, scr, value_expr);
    };

    const auto switch_value_node = args.find("value");
    if (switch_value_node.empty()) sys->raise_error("'switch' requires top-level 'value'");

    size_t switch_index = 0;
    std::string_view switch_type;
    {
      set_expected_type set(ctx, utils::type_name<element_view>());
      const size_t before = ctx->stack_types.size();
      emit_value(switch_value_node);
      if (before >= ctx->stack_types.size()) sys->raise_error("'switch' top-level 'value' does not produce a value");
      switch_index = ctx->stack_types.size() - 1;
      switch_type = ctx->top();
    }
    if (type_is_bool(switch_type) || type_is_void(switch_type) || type_is_ignore(switch_type))
      sys->raise_error(std::format("'switch' cannot compare values of type '{}'", switch_type));

    ctx->scope_stack.push_back(switch_index);
    auto end = e.make_label();
    bool has_case = false;

    size_t offset = 1;
    while (offset < args.size()) {
      const auto curblock = command_block(args, offset);
      offset += curblock.size();
      if (curblock.name() == "value" || curblock.name() == custom_description_constant) continue;
      has_case = true;

      const auto case_value_node = curblock.find("value");
      if (case_value_node.empty()) sys->raise_error("'switch' case block requires 'value'");

      {
        set_expected_type set(ctx, switch_type);
        const size_t before = ctx->stack_types.size();
        emit_value(case_value_node);
        if (before >= ctx->stack_types.size()) sys->raise_error("'switch' case 'value' does not produce a value");
        if (ctx->top() != switch_type)
          sys->raise_error(std::format("'switch' top-level value has type '{}', but case value '{}' returns '{}'", switch_type, case_value_node.name(), ctx->top()));
      }

      const size_t case_index = ctx->stack_types.size() - 1;
      sys->push_basic_function(ctx, scr, basicf::cmpeq2, pack2(int32_t(switch_index), int32_t(case_index)));

      auto next_case = e.make_label();
      e.jump_to(basicf::condjump, next_case);

      sys->push_basic_function(ctx, scr, basicf::erase, case_index);
      ctx->erase(case_index);

      sys->dispatch_node(ctx, scr, curblock);
      if (!exp_is_void) {
        if (ctx->is_ignore()) sys->raise_error(std::format("'switch' case '{}' returns 'ignore_value'", curblock.name()));
        ctx->pop();
      }

      e.jump_to(basicf::jump, end);
      e.bind(next_case);
      sys->push_basic_function(ctx, scr, basicf::erase, case_index);
    }

    if (!has_case) sys->raise_error("'switch' requires at least one case block");

    e.bind(end);
    sys->scope_exit(ctx, scr, 1);
    if (!exp_is_void) ctx->push(exp);

    return args.size();
  });

  RFI(internal::rawmoreeq)("MOREEQ");
  RFI(internal::rawlesseq)("LESSEQ");
  RFI(internal::rawmore)("MORE");
  RFI(internal::rawless)("LESS");

  RFI(internal::chance)("chance", {}, [](emitter& e, const command_block&, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto val = ctx->gen_value();
    sys->push_basic_function(ctx, scr, basicf::chance, std::bit_cast<int64_t>(val));
    return 0;
  });

  RFI(internal::randomfn)("random", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto exp = ctx->expected_type;

    const bool is_void = type_is_void(exp);

    auto end = e.make_label();   // a chosen weighted branch jumps past the rest
    std::vector<size_t> values_indicies;

    const auto val = ctx->gen_value();
    const size_t chance_index = sys->push_basic_function(ctx, scr, basicf::chance, std::bit_cast<int64_t>(val));
    if (chance_index > INT32_MAX) sys->raise_error("wtf");
    const size_t stack_index = ctx->stack_types.size() - 1;
    size_t last_index = stack_index;

    {
      for (const auto& curblock : args.children()) {
        const auto wnode = curblock.find("weight");
        if (wnode.empty()) sys->raise_error(std::format("'random' node requires all of nodes to have node 'weight'"));

        set_expected_type set(ctx, utils::type_name<script_float_t>());
        const size_t start = scr->block_descs.size();
        sys->fold_block(ctx, scr, wnode, basicf::ADD);
        const auto cd = wnode.find(custom_description_constant);
        sys->setup_block_description(ctx, scr, wnode.name(), sys->static_string_arg(cd, custom_description_constant), start);

        if (last_index == ctx->stack_types.size() - 1) sys->raise_error(std::format("No new values on stack after 'weight' node computation?"));
        const size_t weight_index = ctx->stack_types.size() - 1;

        if (values_indicies.size() > 0) {
          sys->push_basic_function(ctx, scr, basicf::sumsetstack, pack2(int32_t(weight_index), int32_t(last_index)));
        }

        values_indicies.push_back(weight_index);
        last_index = weight_index;
      }
    }

    sys->push_basic_function(ctx, scr, basicf::mulsetstack, pack2(int32_t(stack_index), int32_t(last_index)));

    std::string_view value_type = exp;

    size_t counter = 0;
    for (const auto& curblock : args.children()) {
      const size_t arg_index = counter;
      counter += 1;

      const size_t curid = values_indicies[arg_index];

      sys->push_basic_function(ctx, scr, basicf::cmplesseqd2, pack2(int32_t(stack_index), int32_t(curid)));
      e.guarded_clause(end, true, [&]{
        const size_t stack_size = ctx->stack_types.size();
        const size_t start = scr->block_descs.size();
        sys->fold_block(ctx, scr, curblock, basicf::invalid);
        const auto cd = curblock.find(custom_description_constant);
        sys->setup_block_description(ctx, scr, curblock.name(), sys->static_string_arg(cd, custom_description_constant), start);
        if (!is_void) {
          if (stack_size == ctx->stack_types.size()) sys->raise_error(std::format("'{}' produces no value on stack", curblock.name()));
          if (value_type != ctx->top()) sys->raise_error(std::format("'random' node expects all of values to be same type, expected type '{}', but got '{}'", value_type, ctx->top()));
          ctx->pop();
        }
      });
    }

    e.bind(end);

    // Erase temporary weights from the highest stack index down to keep lower indexes valid.
    std::reverse(values_indicies.begin(), values_indicies.end());
    for (const auto id : values_indicies) { sys->push_basic_function(ctx, scr, basicf::erase, id); ctx->erase(id); }
    sys->push_basic_function(ctx, scr, basicf::erase, stack_index);
    ctx->erase(stack_index);

    ctx->push(value_type);

    return args.size();
  });

  RFI(internal::ctx)("ctx", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const size_t ctx_cmd = scr->cmds.size();
    sys->push_basic_function(ctx, scr, basicf::context, 0);
    if (args.size() == 1) return args.size();
    
    const int64_t ctx_slot = int64_t(ctx->stack_types.size()) - 1;
    ctx->scope_stack.push_back(size_t(ctx_slot));
    sys->fold_block(ctx, scr, args, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    // `ctx:saved:x`, `ctx:arg:x` and `ctx:list:x` compile to a single read that takes the context
    // from `context*` directly, so the object this block pushed is never read and neither it nor its
    // unwind has to run. Hand the shape to the peephole rather than skipping the push here: the
    // stream stays executable as emitted, which is what the optimizer is tested against.
    const size_t unwind = scr->cmds.size() - 1;
    if (unwind > ctx_cmd + 1 && find_basicf_by_fp(scr->cmds[unwind].fp) == basicf::erase) {
      size_t reads = 0;
      bool only_reads_and_fallthrough = true;
      for (size_t i = ctx_cmd + 1; i < unwind && only_reads_and_fallthrough; ++i) {
        switch (find_basicf_by_fp(scr->cmds[i].fp)) {
          case basicf::pushctxvalue:
          case basicf::pushargvalue:
          case basicf::pushlist:
            ++reads;
            break;
          // A description placeholder can leave a jump onto the next command between the read and
          // the unwind; it touches no stack slot, so it does not keep the pushed context alive.
          case basicf::jump:
            only_reads_and_fallthrough = size_t(scr->cmds[i].arg) == i + 1;
            break;
          default:
            only_reads_and_fallthrough = false;
            break;
        }
      }
      if (only_reads_and_fallthrough && reads == 1) sys->record_dead_context_push(ctx, ctx_cmd, unwind, ctx_slot);
    }

    return args.size();
  });

  RFI(internal::loadctx)("saved", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto child = command_block(args, 1);
    if (child.empty() || child.args_count() != 0 || child.size() != 1) sys->raise_error(std::format("'ctx:load' expects a string as the only argument"));

    const auto nextblock = command_block(args, 1 + child.size());

    size_t index = scr->find_saved(child.name());
    if (index >= scr->saved.size()) {
      sys->raise_error(std::format("Trying to load unsaved value '{}'", child.name()));
    } else {
      // The slot's type does not have to match the block exactly - a saved integer read in a
      // floating-point block converts like any other value. Requiring equality here used to be
      // harmless only because every numeric literal was a double.
      if (!type_is_any_type(ctx->expected_type) && nextblock.empty() &&
          scr->saved[index].type != ctx->expected_type &&
          !sys->can_convert_implicitly(scr->saved[index].type, ctx->expected_type))
        sys->raise_error(std::format("Saved value type '{}' cannot be used where '{}' is expected", scr->saved[index].type, ctx->expected_type));
    }

    sys->push_basic_function(ctx, scr, basicf::pushctxvalue, index);
    if (nextblock.empty()) return args.size();

    const size_t desc_start = scr->block_descs.size();

    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->fold_block(ctx, scr, nextblock, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    const auto desc_name = sys->static_string_arg(nextblock.find(custom_description_constant), custom_description_constant);
    sys->setup_block_description(ctx, scr, nextblock.name(), desc_name, desc_start);

    return args.size();
  });

  RFI(internal::ctx_save)("ctx_save", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    size_t offset = 1;
    while (offset < args.size()) {
      const auto child = command_block(args, offset);
      offset += child.size();

      if (child.args_count() != 1) sys->raise_error("'ctx:save' requires block with [key] = [value] pairs");

      set_expected_type set(ctx, utils::type_name<element_view>());
      const auto childchild = command_block(child, 1);
      sys->dispatch_node(ctx, scr, childchild);

      const auto top = ctx->top();

      size_t index = scr->find_saved(child.name());
      if (index >= scr->saved.size()) {
        scr->saved.push_back({ sys->store_string(scr, child.name()), top });
        index = scr->saved.size()-1;
      } else {
        scr->saved[index].type = top;
      }

      sys->push_basic_function(ctx, scr, basicf::savectxrvalue, index);
    }

    ctx->push<ignore_value>();

    return args.size();
  });

  RFI(internal::ctx_save_as)("ctx_save_as", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto child = command_block(args, 1);
    if (child.empty() || child.size() > 1) sys->raise_error("'ctx:save_as' expects string as the only argument");

    if (ctx->scope_stack.size() < 1) throw std::runtime_error(std::format("Context does not contain scope to save!"));
    const size_t scope_index = ctx->scope_stack[ctx->scope_stack.size() - 1];
    const auto type = ctx->stack_types[scope_index];
    size_t index = scr->find_saved(child.name());
    if (index >= scr->saved.size()) {
      scr->saved.push_back({ sys->store_string(scr, child.name()), type });
      index = scr->saved.size()-1;
    } else {
        scr->saved[index].type = type;
    }

    sys->push_basic_function(ctx, scr, basicf::savectxlvalue, pack2(scope_index, index));
    ctx->push<ignore_value>();

    return args.size();
  });

  RFI(internal::arg)("arg", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto child = command_block(args, 1);
    if (child.empty() || child.size() > 1) sys->raise_error("'arg:get' expects string as the only argument");

    auto exp_value = ctx->expected_type;

    const auto nextblock = command_block(args, 1 + child.size());
    if (!nextblock.empty()) exp_value = utils::type_name<element_view>();

    size_t index = scr->find_arg(child.name());
    if (index >= scr->args.size()) {
      scr->args.push_back({ sys->store_string(scr, child.name()), exp_value });
      index = scr->args.size()-1;
    } else {
      if (type_is_bool(scr->args[index].type) || type_is_fundamental(scr->args[index].type) || type_is_string(scr->args[index].type)) {
        if (!nextblock.empty()) sys->raise_error(std::format("Could not use argument '{}' as lvalue, type is '{}'", child.name(), scr->args[index].type));
      }

      exp_value = scr->args[index].type;
    }

    if (index >= context::script_arguments_size) sys->raise_error(std::format("Maximum arguments count is {}", context::script_arguments_size));

    sys->push_basic_function(ctx, scr, basicf::pushargvalue, index);
    if (!nextblock.empty()) {
      ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
      sys->dispatch_node(ctx, scr, nextblock);
      sys->scope_exit(ctx, scr, 1);

      if (!ctx->scope_type_upvalue.empty() && exp_value == utils::type_name<element_view>()) {
        scr->args[index].type = ctx->scope_type_upvalue;
      }
    }

    return args.size();
  });

  RFI(internal::ctx_set)("ctx_set", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    size_t offset = 1;
    while (offset < args.size()) {
      const auto child = command_block(args, offset);
      offset += child.size();

      if (child.args_count() != 1) sys->raise_error("'arg:set' requires block with [key] = [value] pairs");

      set_expected_type set(ctx, utils::type_name<element_view>());
      const auto childchild = command_block(child, 1);
      sys->dispatch_node(ctx, scr, childchild);

      const auto top = ctx->top();

      // Match an EXISTING argument by name (find_arg, not find_saved — args and saved are separate
      // tables) so re-setting an arg the script already reads updates that slot instead of creating a
      // duplicate; only push a fresh arg when the name is genuinely new.
      size_t index = scr->find_arg(child.name());
      if (index >= scr->args.size()) {
        scr->args.push_back({ sys->store_string(scr, child.name()), top });
        index = scr->args.size()-1;
      } else {
        scr->args[index].type = top;
      }

      sys->push_basic_function(ctx, scr, basicf::setargrvalue, index);
    }

    ctx->push<ignore_value>();

    return args.size();
  });

  RFI(internal::ctx_set_as)("ctx_set_as", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto child = command_block(args, 1);
    if (child.empty() || child.size() > 1) sys->raise_error("'ctx:save_as' expects string as the only argument");

    if (ctx->scope_stack.size() < 1) throw std::runtime_error(std::format("Context does not contain scope to save!"));
    const size_t scope_index = ctx->scope_stack[ctx->scope_stack.size() - 1];
    const auto type = ctx->stack_types[scope_index];
    size_t index = scr->find_saved(child.name());
    if (index >= scr->args.size()) {
      scr->args.push_back({ sys->store_string(scr, child.name()), type });
      index = scr->args.size() - 1;
    } else {
      scr->args[index].type = type;
    }

    sys->push_basic_function(ctx, scr, basicf::setarglvalue, pack2(scope_index, index));
    ctx->push<ignore_value>();

    return args.size();
  });

  RFI(internal::list)("list", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto child = command_block(args, 1);
    if (child.empty() || child.size() > 1) sys->raise_error("'ctx:list' expects string as the only argument");

    size_t index = scr->find_list(child.name());
    if (index >= scr->lists.size()) {
      scr->lists.push_back({ sys->store_string(scr, child.name()), std::string_view() });
      index = scr->lists.size()-1;
    } else {}

    const size_t start = ctx->stack_types.size();

    sys->push_basic_function(ctx, scr, basicf::pushlist, index);
    if (ctx->ftype != function_type::lvalue) sys->raise_error(std::format("Trying to use function 'list' as rvalue"));

    const auto nextblock = command_block(args, 1 + child.size());

    const auto op_kind = [](const std::string_view name) -> std::optional<container::list_pipeline_kind> {
      using k = container::list_pipeline_kind;
      if (name == "add_to") return k::add_to;
      if (name == "clear") return k::clear;
      if (name == "filter") return k::filter;
      if (name == "map") return k::map;
      if (name == "count") return k::count;
      if (name == "empty") return k::empty;
      if (name == "any") return k::any;
      if (name == "all") return k::all;
      if (name == "none") return k::none;
      if (name == "count_if") return k::count_if;
      if (name == "sum") return k::sum;
      if (name == "min") return k::min;
      if (name == "max") return k::max;
      if (name == "average") return k::average;
      if (name == "first") return k::first;
      if (name == "last") return k::last;
      return std::nullopt;
    };

    bool is_pipeline = false;
    if (!nextblock.empty()) {
      bool has_pipeline_op = false;
      bool has_non_pipeline_op = false;
      for (const auto& op : nextblock.children()) {
        if (op.name() == custom_description_constant) continue;
        const bool is_default = op.name() == "default";
        const bool is_op = op_kind(op.name()).has_value();
        has_pipeline_op = has_pipeline_op || is_op;
        has_non_pipeline_op = has_non_pipeline_op || (!is_op && !is_default);
      }
      is_pipeline = has_pipeline_op && !has_non_pipeline_op;
    }

    if (is_pipeline) {
      const auto requires_default = [](const container::list_pipeline_kind kind) {
        using k = container::list_pipeline_kind;
        return kind == k::first || kind == k::last || kind == k::min || kind == k::max || kind == k::average;
      };
      const auto is_reducer = [](const container::list_pipeline_kind kind) {
        using k = container::list_pipeline_kind;
        return kind == k::count || kind == k::empty || kind == k::any || kind == k::all || kind == k::none ||
               kind == k::count_if || kind == k::sum || kind == k::min || kind == k::max || kind == k::average ||
               kind == k::first || kind == k::last;
      };
      const auto has_value_callback = [](const container::list_pipeline_kind kind) {
        using k = container::list_pipeline_kind;
        return kind == k::filter || kind == k::map || kind == k::any || kind == k::all || kind == k::none ||
               kind == k::count_if || kind == k::sum || kind == k::min || kind == k::max || kind == k::average ||
               kind == k::first || kind == k::last;
      };
      const auto callback_expected = [](const container::list_pipeline_kind kind) {
        using k = container::list_pipeline_kind;
        if (kind == k::map) return utils::type_name<any_stack>();
        if (kind == k::sum || kind == k::min || kind == k::max || kind == k::average) return utils::type_name<script_float_t>();
        return utils::type_name<bool>();
      };

      auto direct_body = [](const command_block& op) {
        return op.size() > 1 ? command_block(op, 1) : command_block();
      };

      auto compile_value_section = [&](const command_block& body, const std::string_view expected) {
        const size_t section_start = scr->cmds.size();
        if (body.empty()) sys->raise_error("List pipeline operation requires a script body");

        const size_t before = ctx->stack_types.size();
        const auto prev_scope_upvalue = ctx->scope_type_upvalue;
        ctx->scope_type_upvalue = std::string_view();
        const auto input_type = scr->lists[index].type.empty() ? utils::type_name<element_view>() : scr->lists[index].type;
        ctx->push(input_type);
        ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
        {
          // Mark this list as "being iterated" for the duration of the per-element callback, so an
          // `execute` inside it can't bind the same list in/out (it would mutate the live list under us).
          push_list_index_upvalue pliu(ctx, index);
          set_expected_type set(ctx, expected);
          sys->dispatch_node(ctx, scr, body);
        }
        sys->scope_exit(ctx, scr, 1);

        if (ctx->stack_types.size() <= before) sys->raise_error(std::format("List pipeline body '{}' produced no value", body.name()));
        const auto result_type = ctx->top();
        if (!type_is_any_type(expected) && result_type != expected) sys->raise_error(std::format("List pipeline body '{}' expected '{}', got '{}'", body.name(), expected, result_type));
        ctx->pop();

        if (scr->lists[index].type.empty() && !ctx->scope_type_upvalue.empty()) scr->lists[index].type = ctx->scope_type_upvalue;
        ctx->scope_type_upvalue = prev_scope_upvalue;
        return std::make_tuple(section_start, scr->cmds.size(), result_type);
      };

      auto compile_default_section = [&](const command_block& body, const std::string_view expected) {
        const size_t section_start = scr->cmds.size();
        if (body.empty()) sys->raise_error("'default' requires a script body");
        bool pushed_outer_scope = false;
        if (!ctx->scope_stack.empty()) {
          const auto& curtype = ctx->current_scope_type();
          if (curtype == utils::type_name<internal::thisarg>() ||
              curtype == utils::type_name<internal::thisctx>() ||
              curtype == utils::type_name<internal::thisctxlist>()) {
            for (auto itr = ctx->scope_stack.rbegin(); itr != ctx->scope_stack.rend(); ++itr) {
              const auto& type = ctx->stack_types[*itr];
              if (type != utils::type_name<internal::thisarg>() &&
                  type != utils::type_name<internal::thisctx>() &&
                  type != utils::type_name<internal::thisctxlist>()) {
                ctx->scope_stack.push_back(*itr);
                pushed_outer_scope = true;
                break;
              }
            }
          }
        }
        {
          set_expected_type set(ctx, expected);
          sys->dispatch_node(ctx, scr, body);
        }
        if (pushed_outer_scope) ctx->scope_stack.pop_back();
        if (ctx->stack_types.size() <= start) sys->raise_error("'default' produced no value");
        const auto result_type = ctx->top();
        if (!type_is_any_type(expected) && result_type != expected) sys->raise_error(std::format("'default' expected '{}', got '{}'", expected, result_type));
        ctx->pop();
        return std::make_tuple(section_start, scr->cmds.size(), result_type);
      };

      bool reduced = false;
      size_t offset = 1;
      while (offset < nextblock.size()) {
        const auto op = command_block(nextblock, offset);
        offset += op.size();
        if (op.name() == custom_description_constant) continue;
        if (op.name() == "default") sys->raise_error("'default' must immediately follow first/last/min/max/average");
        if (reduced) sys->raise_error(std::format("List reducer must be the last significant operation, found '{}'", op.name()));

        const auto maybe_kind = op_kind(op.name());
        if (!maybe_kind) sys->raise_error(std::format("Unknown list pipeline operation '{}'", op.name()));
        const auto kind = *maybe_kind;

        command_block default_body;
        if (requires_default(kind)) {
          if (offset >= nextblock.size()) sys->raise_error(std::format("List operation '{}' requires 'default' immediately after it", op.name()));
          const auto def = command_block(nextblock, offset);
          if (def.name() != "default") sys->raise_error(std::format("List operation '{}' requires 'default' immediately after it", op.name()));
          default_body = direct_body(def);
          offset += def.size();
        }

        // Emit the op + three immediate-data slots; the metadata (callback ranges, resume point,
        // input element type) is backpatched into those slots once the sections are compiled — no
        // side table. Offsets are stored relative to the opcode's own cmd index (op_cmd).
        const auto input_type_at_op = scr->lists[index].type;  // element type fed to this op's callback
        const size_t op_cmd = scr->cmds.size();
        scr->cmds.emplace_back(container::command(&list_pipeline, pack2(int32_t(kind), int32_t(index))));
        scr->cmds.emplace_back(container::command(&list_op_data, int64_t(0)));  // [op+1] value range
        scr->cmds.emplace_back(container::command(&list_op_data, int64_t(0)));  // [op+2] default_start + end
        scr->cmds.emplace_back(container::command(&list_op_data, int64_t(0)));  // [op+3] input_type string ref
        sys->record_data_slots(ctx, op_cmd + 1, 3);
        size_t value_start = 0, value_end = 0, default_start = 0;  // absolute cmd indices; 0 == section absent

        if (kind == container::list_pipeline_kind::add_to) {
          const auto [s, en, result_type] = compile_default_section(direct_body(op), utils::type_name<any_stack>());
          default_start = s;  // default_end is always the resume point `end`
          if (type_is_ignore(result_type) || type_is_void(result_type)) sys->raise_error(std::format("List operation '{}' cannot add '{}'", op.name(), result_type));
          if (scr->lists[index].type.empty()) scr->lists[index].type = result_type;
          else if (scr->lists[index].type != result_type) sys->raise_error(std::format("List '{}' expects '{}', got '{}'", child.name(), scr->lists[index].type, result_type));
        } else if (has_value_callback(kind)) {
          const auto [s, en, result_type] = compile_value_section(direct_body(op), callback_expected(kind));
          value_start = s; value_end = en;
          if (kind == container::list_pipeline_kind::map) {
            if (type_is_ignore(result_type) || type_is_void(result_type)) sys->raise_error(std::format("List operation '{}' cannot map to '{}'", op.name(), result_type));
            scr->lists[index].type = result_type;
          }
        }

        if (requires_default(kind)) {
          const auto expected = (kind == container::list_pipeline_kind::min || kind == container::list_pipeline_kind::max || kind == container::list_pipeline_kind::average)
            ? utils::type_name<script_float_t>()
            : (scr->lists[index].type.empty() ? utils::type_name<any_stack>() : scr->lists[index].type);
          const auto [s, en, result_type] = compile_default_section(default_body, expected);
          default_start = s;  // default_end == end
        }

        const size_t end_abs = scr->cmds.size();
        scr->cmds[op_cmd + 1].arg = pack2(int32_t(value_start ? value_start - op_cmd : 0), int32_t(value_end ? value_end - op_cmd : 0));
        scr->cmds[op_cmd + 2].arg = pack2(int32_t(default_start ? default_start - op_cmd : 0), int32_t(end_abs - op_cmd));
        // The four section bounds are stored relative to the opcode, so the peephole has to know
        // both which half holds them and what they are relative to.
        sys->record_cmd_index(ctx, op_cmd + 1, 1, op_cmd, true);
        sys->record_cmd_index(ctx, op_cmd + 1, 2, op_cmd, true);
        sys->record_cmd_index(ctx, op_cmd + 2, 1, op_cmd, true);
        sys->record_cmd_index(ctx, op_cmd + 2, 2, op_cmd, false);
        if (!input_type_at_op.empty()) {
          const auto ref = sys->store_string(scr, input_type_at_op);
          scr->cmds[op_cmd + 3].arg = packstrid(uint32_t(ref.start), uint32_t(ref.count));
        }

        if (kind == container::list_pipeline_kind::filter || kind == container::list_pipeline_kind::map || kind == container::list_pipeline_kind::clear) {
          // no stack value
        } else if (kind == container::list_pipeline_kind::count || kind == container::list_pipeline_kind::count_if || kind == container::list_pipeline_kind::sum ||
                   kind == container::list_pipeline_kind::min || kind == container::list_pipeline_kind::max || kind == container::list_pipeline_kind::average) {
          ctx->push<double>();
        } else if (kind == container::list_pipeline_kind::empty || kind == container::list_pipeline_kind::any || kind == container::list_pipeline_kind::all || kind == container::list_pipeline_kind::none) {
          ctx->push<bool>();
        } else if (kind == container::list_pipeline_kind::first || kind == container::list_pipeline_kind::last) {
          ctx->push(scr->lists[index].type.empty() ? utils::type_name<any_stack>() : scr->lists[index].type);
        }

        reduced = is_reducer(kind);
      }

      sys->push_basic_function(ctx, scr, basicf::erase, start);
      ctx->erase(start);
      if (!reduced) ctx->push<ignore_value>();
      return args.size();
    }

    push_list_index_upvalue pliu(ctx, index);
    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->dispatch_node(ctx, scr, nextblock);
    sys->scope_exit(ctx, scr, 1);

    size_t counter = start;
    while (counter < ctx->stack_types.size()) {
      if (type_is_ignore(ctx->stack_types[counter])) ctx->stack_types.erase(ctx->stack_types.begin() + counter);
      else counter += 1;
    }

    if (ctx->stack_types.size() == start) ctx->push<ignore_value>();

    return args.size();
  });

  RFI(internal::add_to)("add_to");
  RFI(internal::is_in)("is_in");
  RFI(internal::remove_from)("remove_from");
}

#undef EPSILON

}
