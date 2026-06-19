#include "devils_script/system.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <format>
#include <optional>

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

#define EPSILON 0.000001

namespace internal {
static double rawadd(const double val1, const double val2) noexcept { return val1 + val2; }
static double rawmul(const double val1, const double val2) noexcept { return val1 * val2; }
static double rawsub(const double val1, const double val2) noexcept { return val1 - val2; }
static double rawdiv(const double val1, const double val2) noexcept { return val1 / val2; }
static double rawmod(const double val1, const double val2) noexcept { return std::fmod(val1, val2); }
static double rawpos(const double val1) noexcept { return +val1; }
static double rawneg(const double val1) noexcept { return -val1; }
static bool rawnot(const bool val1) noexcept { return !val1; }

static bool rawmore(const double val1, const double val2) noexcept { return val1 > val2; }
static bool rawless(const double val1, const double val2) noexcept { return val1 < val2; }
static bool rawmoreeq(const double val1, const double val2) noexcept { return val1 >= val2; }
static bool rawlesseq(const double val1, const double val2) noexcept { return val1 <= val2; }

static double rawmax(const double val1, const double val2) noexcept { return std::max(val1, val2); }
static double rawmin(const double val1, const double val2) noexcept { return std::min(val1, val2); }
static double rawabs(const double val1) noexcept { return std::abs(val1); }
static double rawceil(const double val1) noexcept { return std::ceil(val1); }
static double rawfloor(const double val1) noexcept { return std::floor(val1); }
static double rawround(const double val1) noexcept { return std::round(val1); }
static double rawtrunc(const double val1) noexcept { return std::trunc(val1); }
static double rawexp(const double val1) noexcept { return std::exp(val1); }
static double rawsqrt(const double val1) noexcept { return std::sqrt(val1); }
static double rawinversesqrt(const double val1) noexcept { return 1.0 / std::sqrt(val1); }
static double rawsin(const double val1) noexcept { return std::sin(val1); }
static double rawcos(const double val1) noexcept { return std::cos(val1); }
static double rawasin(const double val1) noexcept { return std::asin(val1); }
static double rawacos(const double val1) noexcept { return std::acos(val1); }
static double rawtan(const double val1) noexcept { return std::tan(val1); }
static double rawatan(const double val1) noexcept { return std::atan(val1); }
static double rawinc(const double val1) noexcept { return val1 + 1.0; }
static double rawdec(const double val1) noexcept { return val1 - 1.0; }
static double rawinv(const double val1) noexcept { return 1.0 / val1; }

static bool raweqb(const bool val1, const bool val2) noexcept { return val1 == val2; }
static bool raweqi(const int64_t val1, const int64_t val2) noexcept { return val1 == val2; }
static bool raweqd(const double val1, const double val2) noexcept { return std::abs(val1 - val2) < EPSILON; }
static bool raweqs(const std::string_view& val1, const std::string_view& val2) noexcept { return val1 == val2; }
static bool raweq (const element_view& val1, const element_view& val2) noexcept { 
  return val1 == val2;
}

static bool operator_or(const bool val1, const bool val2) noexcept { return val1 || val2; }
static bool operator_and(const bool val1, const bool val2) noexcept { return val1 && val2; }

static double rawsign(const double v1) noexcept { return v1 > 0.0 ? 1.0 : (v1 < 0.0 ? -1.0 : 0.0); }
// probably needs to use intrinsics 
static double rawfma(const double v1, const double v2, const double v3) noexcept { return v1 * v2 + v3; }
static double rawfract(const double v1) noexcept { return v1 - rawfloor(v1); }
//static double rawtrunc(const double v1) noexcept { return v1 < 0.0 ? rawceil(v1) : rawfloor(v1); }
static double rawmix(const double v1, const double v2, const double v3) noexcept { return v1 * (1.0 - v3) + v2 * v3; }
static double rawsclamp(const double t, const double v1, const double v2) noexcept { return std::clamp(t, v1, v2);  }
static double rawsmoothstep(const double v1, const double v2, const double x) noexcept {
  if (x <= v1) return 0.0;
  if (x >= v2) return 1.0;
  const double t = rawsclamp((x - v1) / (v2 - v1), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}
static double rawstep(const double v1, const double x) noexcept { return x < v1 ? 0.0 : 1.0; }
static double rawrndmix1(const double v1) noexcept { return prng::prng_normalize(prng::mix(std::bit_cast<uint64_t>(v1))); }
static double rawrndmix(const double v1, const double v2) noexcept { return prng::prng_normalize(prng::mix(std::bit_cast<uint64_t>(v1), std::bit_cast<uint64_t>(v2))); }
}
#define RFI(func) register_function<&func>
#define ROI(func) register_operator<&func>

void system::init_math() {
  ROI(internal::rawpos)("unary_plus", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawneg)("unary_minus", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawnot)("not", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawinc)("++", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawdec)("--", { 14, command_data::math_ftype::prefix, command_data::associativity::right });
  ROI(internal::rawmul)("*", { 12, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawdiv)("/", { 12, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmod)("%", { 12, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawadd)("+", { 11, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawsub)("-", { 11, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmoreeq)(">=", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawlesseq)("<=", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawmore)(">", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  ROI(internal::rawless)("<", { 8, command_data::math_ftype::binary, command_data::associativity::left });
  //ROI(raweq)("==", { 7, command_data::math_ftype::binary, command_data::associativity::left }); // why isnt here?
  //ROI(rawneq)("!=", { 7, command_data::math_ftype::binary, command_data::associativity::left });
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
//static ignore_value inc(thisctx, ctx_value) { return ignore_value{}; } // changed how we deal with variables
//static ignore_value dec(thisctx, ctx_value) { return ignore_value{}; }

enum class arg_value {};
static thisarg arg() { return thisarg{}; }
// this way? or custom function?
//static any_stack loadarg(thisctx ctx, arg_value val) {
  //return ctx.ctx->get_arg<any_stack>(static_cast<size_t>(val));
//}

static ignore_value ctx_set(arg_value) { return ignore_value{}; }
static ignore_value ctx_set_as(arg_value) { return ignore_value{}; }

// needs to be slightly rewritten
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

//static ignore_value custom_description(const element_view&) { return ignore_value{}; }
//static any_stack pushcurrent() { return any_stack{}; }
static double chance() { return 1; }
static any_stack randomfn(double, const element_view&) { return any_stack{}; }

static size_t line_for_command(const context* ctx, const container* scr) {
  if (scr == nullptr || ctx->current_index >= scr->descs.size()) return 0;
  const auto name = scr->descs[ctx->current_index].name;
  if (name.count == SIZE_MAX || scr->globals.empty()) return 0;
  size_t line = 1;
  const size_t end = std::min(name.start, scr->globals[0].size());
  for (size_t i = 0; i < end; ++i) line += size_t(scr->globals[0][i] == '\n');
  return line;
}

static std::string debug_environment(const context* ctx, const container* scr, const size_t consumed_args) {
  const auto fn = scr != nullptr && ctx->current_index < scr->descs.size()
    ? scr->get_string(scr->descs[ctx->current_index].name)
    : std::string_view();
  const int64_t scope_index = int64_t(ctx->stack.size()) - int64_t(consumed_args) - 1;
  const auto scope = scope_index >= 0 ? ctx->stack.type(scope_index) : std::string_view();
  return std::format("line {}, function '{}', scope '{}'", line_for_command(ctx, scr), fn, scope);
}

static int64_t debug_assert(int64_t, context* ctx, const container* scr) {
  const auto message = ctx->stack.safe_pop<std::string_view>();
  const bool condition = ctx->stack.safe_pop<bool>();
  if (!condition) {
    throw std::runtime_error(std::format("Script assert failed: '{}' ({})", message, debug_environment(ctx, scr, 2)));
  }
  return -2;
}

static int64_t debug_trace(int64_t, context* ctx, const container* scr) {
  const auto message = ctx->stack.safe_pop<std::string_view>();
  if (ctx->trace) ctx->trace(std::format("Script trace: '{}' ({})", message, debug_environment(ctx, scr, 1)));
  return -1;
}
}

template <auto f>
static void add_cmd(const system* sys, container* scr) {
  using F = decltype(f);
  constexpr function_t fs[] = { &mathfunc_unsafe<f>, &mathfunc<f> };
  scr->cmds.emplace_back(fs[size_t(sys->safety())], INT64_C(0));
}

#define ADD_CMD(fn) add_cmd<fn>

void system::init_basic_functions() {
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
    offset += sys->parse_arg<std::string_view>(ctx, scr, message, 1, utils::type_name<std::string_view>(), std::string_view(), {});
    if (offset < args.size()) sys->raise_error(std::format("Too many arguments for function '{}'", args.name()));

    scr->cmds.emplace_back(&internal::debug_assert, INT64_C(0));
    sys->setup_description<&internal::ctx_save, void, is_valid_t<void>(nullptr)>(ctx, scr, args.name());
    ctx->pop();
    ctx->pop();
    return args.size();
  });

  RFI(internal::ctx_save)("trace", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    size_t offset = 1;
    auto message = command_block(args, offset);
    if (message.name() == custom_description_constant) { offset += message.size(); message = command_block(args, offset); }
    offset += sys->parse_arg<std::string_view>(ctx, scr, message, 0, utils::type_name<std::string_view>(), std::string_view(), {});
    if (offset < args.size()) sys->raise_error(std::format("Too many arguments for function '{}'", args.name()));

    scr->cmds.emplace_back(&internal::debug_trace, INT64_C(0));
    sys->setup_description<&internal::ctx_save, void, is_valid_t<void>(nullptr)>(ctx, scr, args.name());
    ctx->pop();
    return args.size();
  });

  // is 'while (ctx->pop_while_ignore())' an overkill for this situations?
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

    // sys->fold_block(ctx, scr, args, basicf::invalid); - doesnt produce description
    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->fold_block(ctx, scr, args, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    return args.size();
  });
  
  RFI(internal::prevfn)("prev", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    if (ctx->scope_stack.size() <= 1 + ctx->prev_chaining) sys->raise_error(std::format("Function 'prev' requires at least 2 elements in scope_stack"));

    // mess =(
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

  // jump out of the script internal ctxs
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
    ctx->prev_chaining += counter; // ???
    
    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->fold_block(ctx, scr, args, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    ctx->prev_chaining = prev_value;

    return args.size();
  });

  // making equality for every possible combinations...
  const auto eqfn = [](emitter& e, const command_block& args, const std::vector<std::string>&) -> size_t {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto expected = utils::type_name<element_view>();
    size_t offset = 1;
    do { 
      auto cb = command_block(args, offset);
      if (cb.name() == custom_description_constant) { offset += cb.size(); cb = command_block(args, offset); }
      offset += sys->parse_arg<element_view>(ctx, scr, cb, 0, expected, std::string_view(), {});
    } while (ctx->pop_while_ignore()); // pop_while_ignore is overkill?
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

    sys->setup_description<&internal::raweq, void, is_valid_t<void>(nullptr)>(ctx, scr, args.name());

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
    // override function to "EQ" ???
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

  // find first 'condition' that true and compute a block
  // simple "if then else"
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
        if (!type_is_void(exp) && ctx->is_ignore()) sys->raise_error(std::format("Block in 'select' function returns 'ignore_value'???"));
      });

      if (!type_is_void(exp)) ctx->pop();
    }

    if (!type_is_void(exp)) ctx->push(exp);

    e.bind(end);

    return args.size();
  });
  
  // while 'condition' block is true, do command in a block
  // when false, jump out
  RFI(internal::selectfn)("sequence", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    // what to do when no value? now i try to push default one
    // but maybe better to push ignore_value?

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
      if (!type_is_void(exp) && ctx->is_ignore()) sys->raise_error(std::format("Block in 'sequence' function returns 'ignore_value'???"));

      if (exp_is_bool) {
        if (ctx->is<int64_t>()) sys->setup_type_conversion<int64_t, bool>(ctx, scr);
        if (ctx->is<double>()) sys->setup_type_conversion<double, bool>(ctx, scr);
        // boolean optimization?
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
    sys->push_basic_function(ctx, scr, basicf::chance, std::bit_cast<int64_t>(val)); // push
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

        set_expected_type set(ctx, utils::type_name<double>());
        const size_t start = scr->block_descs.size();
        sys->fold_block(ctx, scr, wnode, basicf::ADD);
        const auto cd = wnode.find(custom_description_constant);
        sys->setup_block_description(ctx, scr, wnode.name(), command_block(cd, 1).name(), start);

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

      sys->push_basic_function(ctx, scr, basicf::cmplesseqd2, pack2(int32_t(stack_index), int32_t(curid))); // push
      e.guarded_clause(end, true, [&]{
        const size_t stack_size = ctx->stack_types.size();
        const size_t start = scr->block_descs.size();
        sys->fold_block(ctx, scr, curblock, basicf::invalid);
        const auto cd = curblock.find(custom_description_constant);
        sys->setup_block_description(ctx, scr, curblock.name(), command_block(cd, 1).name(), start);
        if (!is_void) {
          if (stack_size == ctx->stack_types.size()) sys->raise_error(std::format("'{}' produces no value on stack ???", curblock.name()));
          if (value_type != ctx->top()) sys->raise_error(std::format("'random' node expects all of values to be same type, expected type '{}', but got '{}'", value_type, ctx->top()));
          ctx->pop();
        }
      });
    }

    e.bind(end);

    // erase needs to be in OPPOSITE order
    std::reverse(values_indicies.begin(), values_indicies.end());
    for (const auto id : values_indicies) { sys->push_basic_function(ctx, scr, basicf::erase, id); ctx->erase(id); }
    sys->push_basic_function(ctx, scr, basicf::erase, stack_index);
    ctx->erase(stack_index);

    ctx->push(value_type);

    return args.size();
  });

  RFI(internal::ctx)("ctx", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    sys->push_basic_function(ctx, scr, basicf::context, 0);
    if (args.size() == 1) return args.size();
    
    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->fold_block(ctx, scr, args, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    return args.size();
  });

  RFI(internal::loadctx)("saved", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto child = command_block(args, 1);
    if (child.empty() || child.args_count() != 0 || child.size() != 1) sys->raise_error(std::format("'ctx:load' expects a string as the only argument"));

    //if (!check_is_str_part_of(scr->globals[0], child.name())) sys->raise_error(std::format("'{}' is not a part of original script string", child.name()));

    const auto nextblock = command_block(args, 1 + child.size());

    size_t index = scr->find_saved(child.name());
    if (index >= scr->saved.size()) {
      sys->raise_error(std::format("Trying to load unsaved value '{}'", child.name()));
    } else {
      if (!type_is_any_type(ctx->expected_type) && nextblock.empty() && scr->saved[index].type != ctx->expected_type) 
        sys->raise_error(std::format("Saved value type '{}' is not equals to expected '{}'", scr->saved[index].type, ctx->expected_type));
    }

    sys->push_basic_function(ctx, scr, basicf::pushctxvalue, index); // push
    if (nextblock.empty()) return args.size();

    const size_t desc_start = scr->block_descs.size();

    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->fold_block(ctx, scr, nextblock, basicf::invalid);
    sys->scope_exit(ctx, scr, 1);

    const auto desc_name = nextblock.find(custom_description_constant).name();
    sys->setup_block_description(ctx, scr, nextblock.name(), desc_name, desc_start);

    return args.size();
  });

  RFI(internal::ctx_save)("ctx_save", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    size_t offset = 1;
    while (offset < args.size()) {
      const auto child = command_block(args, offset);
      offset += child.size();

      // ???
      if (child.args_count() != 1) sys->raise_error("'ctx:save' requires block with [key] = [value] pairs");

      set_expected_type set(ctx, utils::type_name<element_view>());
      const auto childchild = command_block(child, 1);
      sys->dispatch_node(ctx, scr, childchild);

      const auto top = ctx->top();

      size_t index = scr->find_saved(child.name());
      if (index >= scr->saved.size()) {
        if (!check_is_str_part_of(scr->globals[0], child.name())) sys->raise_error(std::format("'{}' is not a part of original script string", child.name()));

        const size_t fname_start = child.name().data() - scr->globals[0].data();
        const size_t fname_size = child.name().size();

        scr->saved.push_back({ { fname_start, fname_size }, top });
        index = scr->saved.size()-1;
      } else {
        // can be oversaved? i think yes
        //if (scr->args[index].type != ctx->current_scope_type()) sys->raise_error(std::format("Argument type '{}' is not equals to expected '{}'", scr->args[index].type, ctx->expected_type));
        scr->saved[index].type = top;
      }

      sys->push_basic_function(ctx, scr, basicf::savectxrvalue, index); // pop
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
      if (!check_is_str_part_of(scr->globals[0], child.name())) sys->raise_error(std::format("'{}' is not a part of original script string", child.name()));

      const size_t fname_start = child.name().data() - scr->globals[0].data();
      const size_t fname_size = child.name().size();

      scr->saved.push_back({ { fname_start, fname_size }, type });
      index = scr->saved.size()-1;
    } else {
      // can be oversaved? i think yes
      //if (scr->args[index].type != ctx->current_scope_type()) sys->raise_error(std::format("Argument type '{}' is not equals to expected '{}'", scr->args[index].type, ctx->expected_type));
      scr->saved[index].type = type;
    }

    sys->push_basic_function(ctx, scr, basicf::savectxlvalue, pack2(scope_index, index)); // no pop
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
      if (!check_is_str_part_of(scr->globals[0], child.name())) sys->raise_error(std::format("'{}' is not a part of original script string", child.name()));

      const size_t fname_start = child.name().data() - scr->globals[0].data();
      const size_t fname_size = child.name().size();

      scr->args.push_back({ { fname_start, fname_size }, exp_value });
      index = scr->args.size()-1;
    } else {
      //if (scr->args[index].type != exp_value) sys->raise_error(std::format("Argument type '{}' is not equals to expected '{}'", scr->args[index].type, ctx->expected_type));
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

      // ???
      if (child.args_count() != 1) sys->raise_error("'arg:set' requires block with [key] = [value] pairs");

      set_expected_type set(ctx, utils::type_name<element_view>());
      const auto childchild = command_block(child, 1);
      sys->dispatch_node(ctx, scr, childchild);

      const auto top = ctx->top();

      size_t index = scr->find_saved(child.name());
      if (index >= scr->args.size()) {
        if (!check_is_str_part_of(scr->globals[0], child.name())) sys->raise_error(std::format("'{}' is not a part of original script string", child.name()));

        const size_t fname_start = child.name().data() - scr->globals[0].data();
        const size_t fname_size = child.name().size();

        scr->args.push_back({ { fname_start, fname_size }, top });
        index = scr->args.size()-1;
      } else {
        // can be oversaved? i think yes
        //if (scr->args[index].type != ctx->current_scope_type()) sys->raise_error(std::format("Argument type '{}' is not equals to expected '{}'", scr->args[index].type, ctx->expected_type));
        scr->args[index].type = top;
      }

      sys->push_basic_function(ctx, scr, basicf::setargrvalue, index); // pop
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
      if (!check_is_str_part_of(scr->globals[0], child.name())) sys->raise_error(std::format("'{}' is not a part of original script string", child.name()));

      const size_t fname_start = child.name().data() - scr->globals[0].data();
      const size_t fname_size = child.name().size();

      scr->args.push_back({ { fname_start, fname_size }, type });
      index = scr->args.size() - 1;
    } else {
      // can be oversaved? i think yes
      //if (scr->args[index].type != ctx->current_scope_type()) sys->raise_error(std::format("Argument type '{}' is not equals to expected '{}'", scr->args[index].type, ctx->expected_type));
      scr->args[index].type = type;
    }

    sys->push_basic_function(ctx, scr, basicf::setarglvalue, pack2(scope_index, index)); // no pop
    ctx->push<ignore_value>();

    return args.size();
  });

  RFI(internal::list)("list", {}, [](emitter& e, const command_block& args, const std::vector<std::string>&) {
    [[maybe_unused]] const auto sys = e.sys; [[maybe_unused]] const auto ctx = e.ctx; [[maybe_unused]] const auto scr = e.scr;
    const auto child = command_block(args, 1);
    if (child.empty() || child.size() > 1) sys->raise_error("'ctx:list' expects string as the only argument");

    size_t index = scr->find_list(child.name());
    if (index >= scr->lists.size()) {
      if (!check_is_str_part_of(scr->globals[0], child.name())) sys->raise_error(std::format("'{}' is not a part of original script string", child.name()));

      const size_t fname_start = child.name().data() - scr->globals[0].data();
      const size_t fname_size = child.name().size();

      scr->lists.push_back({ { fname_start, fname_size }, std::string_view() });
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
        if (kind == k::sum || kind == k::min || kind == k::max || kind == k::average) return utils::type_name<double>();
        return utils::type_name<bool>();
      };

      auto direct_body = [](const command_block& op) {
        return op.size() > 1 ? command_block(op, 1) : command_block();
      };

      auto emit_desc = [&](const command_block& op, const bool has_return) {
        using sv_t = container::command_description::global_string_view;
        sv_t name{ static_cast<size_t>(basicf::invalid), SIZE_MAX };
        if (check_is_str_part_of(scr->globals[0], op.name())) name = { size_t(op.name().data() - scr->globals[0].data()), op.name().size() };
        scr->descs.emplace_back(name, 1, true, true, has_return, false, ctx->nest_level, SIZE_MAX);
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

        container::list_pipeline_op meta{ kind, index, scr->lists[index].type, 0, 0, 0, 0, 0 };
        const size_t meta_index = scr->list_pipeline_ops.size();
        scr->list_pipeline_ops.push_back(meta);
        scr->cmds.emplace_back(container::command(&list_pipeline, int64_t(meta_index)));
        emit_desc(op, is_reducer(kind));

        if (kind == container::list_pipeline_kind::add_to) {
          const auto [s, en, result_type] = compile_default_section(direct_body(op), utils::type_name<any_stack>());
          scr->list_pipeline_ops[meta_index].default_start = s;
          scr->list_pipeline_ops[meta_index].default_end = en;
          if (type_is_ignore(result_type) || type_is_void(result_type)) sys->raise_error(std::format("List operation '{}' cannot add '{}'", op.name(), result_type));
          if (scr->lists[index].type.empty()) scr->lists[index].type = result_type;
          else if (scr->lists[index].type != result_type) sys->raise_error(std::format("List '{}' expects '{}', got '{}'", child.name(), scr->lists[index].type, result_type));
        } else if (has_value_callback(kind)) {
          const auto [s, en, result_type] = compile_value_section(direct_body(op), callback_expected(kind));
          scr->list_pipeline_ops[meta_index].value_start = s;
          scr->list_pipeline_ops[meta_index].value_end = en;
          if (kind == container::list_pipeline_kind::map) {
            if (type_is_ignore(result_type) || type_is_void(result_type)) sys->raise_error(std::format("List operation '{}' cannot map to '{}'", op.name(), result_type));
            scr->lists[index].type = result_type;
          }
        }

        if (requires_default(kind)) {
          const auto expected = (kind == container::list_pipeline_kind::min || kind == container::list_pipeline_kind::max || kind == container::list_pipeline_kind::average)
            ? utils::type_name<double>()
            : (scr->lists[index].type.empty() ? utils::type_name<any_stack>() : scr->lists[index].type);
          const auto [s, en, result_type] = compile_default_section(default_body, expected);
          scr->list_pipeline_ops[meta_index].default_start = s;
          scr->list_pipeline_ops[meta_index].default_end = en;
        }

        scr->list_pipeline_ops[meta_index].end = scr->cmds.size();
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

    // if this block has 1 arg with str, then next child would be __empty_lvalue
    // description?
    //const size_t desc_start = scr->block_descs.size();

    push_list_index_upvalue pliu(ctx, index);
    ctx->scope_stack.push_back(ctx->stack_types.size() - 1);
    sys->dispatch_node(ctx, scr, nextblock);
    sys->scope_exit(ctx, scr, 1);

    //const auto desc_name = nextblock.find(custom_description_constant).name();
    //sys->setup_block_description(ctx, scr, nextblock.name(), desc_name, desc_start);

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

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
