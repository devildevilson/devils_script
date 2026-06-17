#include "devils_script/system.h"

#include <cmath>
#include <cassert>
//#include <iostream>
//#include <iomanip>
#include <cstring>
#include "devils_script/string-utils.hpp"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

bool type_is_ignore(const std::string_view& type) noexcept { return type == utils::type_name<ignore_value>(); }
bool type_is_void(const std::string_view& type) noexcept { return type == utils::type_name<void>() || type == utils::type_name<utils::void_t>(); }
bool type_is_bool(const std::string_view& type) noexcept { return type == utils::type_name<bool>(); }
bool type_is_integral(const std::string_view& type) noexcept { return type == utils::type_name<int64_t>(); }
bool type_is_floating_point(const std::string_view& type) noexcept { return type == utils::type_name<double>(); }
bool type_is_fundamental(const std::string_view& type) noexcept { return type_is_integral(type) || type_is_floating_point(type); }
bool type_is_string(const std::string_view& type) noexcept { return type == utils::type_name<std::string_view>(); }
bool type_is_object(const std::string_view& type) noexcept { return !type_is_ignore(type) && !type_is_void(type) && !type_is_bool(type) && !type_is_fundamental(type) && !type_is_string(type); }
bool type_is_element_view(const std::string_view& type) noexcept { return type == utils::type_name<element_view>(); }
bool type_is_object_view(const std::string_view& type) noexcept { return type == utils::type_name<object_view>(); }
bool type_is_any_stack(const std::string_view& type) noexcept { return type == utils::type_name<any_stack>(); }
bool type_is_any_object(const std::string_view& type) noexcept { return type == utils::type_name<any_object>(); }
bool type_is_any_type_object(const std::string_view& type) noexcept { return type_is_object_view(type) || type_is_any_object(type);  }
bool type_is_any_type(const std::string_view& type) noexcept { return type_is_any_stack(type) || type_is_element_view(type) || type_is_any_type_object(type); }

static constexpr char invalid_memory[MAXIMUM_STACK_VAL_SIZE] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; // 0xFF, ...

stack_element::view::view() noexcept : _mem(nullptr) {}
stack_element::view::view(const char* _mem, const std::string_view& _type) noexcept : _mem(_mem), _type(_type) {}
bool stack_element::view::valid() const { return !_type.empty() && memcmp(_mem, invalid_memory, MAXIMUM_STACK_VAL_SIZE) != 0; }
std::string_view stack_element::view::type() const { return _type; }
bool operator==(const stack_element::view& v1, const stack_element::view& v2) {
  return memcmp(v1._mem, v2._mem, MAXIMUM_STACK_VAL_SIZE) == 0 && v1._type == v2._type;
}
bool operator!=(const stack_element::view& v1, const stack_element::view& v2) {
  return !(v1 == v2);
}
void stack_element::invalidate() { memset(mem, -1, MAXIMUM_STACK_VAL_SIZE); }
bool stack_element::invalid() const { return memcmp(mem, invalid_memory, MAXIMUM_STACK_VAL_SIZE) == 0; }
any_stack::any_stack() noexcept { memset(_mem, 0, MAXIMUM_STACK_VAL_SIZE); }
any_stack::any_stack(const char* mem, const std::string_view& _type) noexcept : _type(_type) { memcpy(_mem, mem, MAXIMUM_STACK_VAL_SIZE); }
void any_stack::invalidate() { memset(_mem, -1, MAXIMUM_STACK_VAL_SIZE); }
bool any_stack::invalid() const { return memcmp(_mem, invalid_memory, MAXIMUM_STACK_VAL_SIZE) == 0; }
std::string_view any_stack::type() const { return _type; }
stack_element::view any_stack::view() const { return stack_element::view(_mem, type()); }

void context::create_lists(const container* scr) {
  lists.clear();
  lists.resize(scr->lists.size());
}

const std::string_view basicf_names[] = {
  "none",
#define X(name) #name,
  DEVILS_SCRIPT_BASIC_FUNCTIONS_LIST
#undef X
  "invalid"
};
const size_t basicf_names_size = sizeof(basicf_names) / sizeof(basicf_names[0]);

std::string_view to_string(const basicf val) noexcept {
  if (static_cast<size_t>(val) >= basicf_names_size) return std::string_view();
  return basicf_names[static_cast<size_t>(val)];
}

basicf find_basicf(const std::string_view& str) noexcept {
  for (size_t i = 0; i < basicf_names_size; ++i) {
    if (str == basicf_names[i]) return static_cast<basicf>(i);
  }
  return basicf::invalid;
}

template <typename T1, typename T2> requires(is_typeless_v<T1> && is_typeless_v<T2>)
bool operator==(const T1& s1, const T2& s2) noexcept {
  return memcmp(s1._mem, s2._mem, MAXIMUM_STACK_VAL_SIZE) == 0 && s1.type() == s2.type();
}

template <typename T1, typename T2> requires(is_typeless_v<T1> && is_typeless_v<T2>)
bool operator!=(const T1& s1, const T2& s2) noexcept {
  return !(operator==(s1, s2));
}

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

system::nest_level_changer::nest_level_changer(parse_ctx* ctx) noexcept : ctx(ctx) { ctx->nest_level += 1; }
system::nest_level_changer::~nest_level_changer() noexcept { ctx->nest_level -= 1; }

system::function_name_changer::function_name_changer(parse_ctx* ctx, const std::string_view& str) noexcept : ctx(ctx) { ctx->push_func(str);  }
system::function_name_changer::~function_name_changer() noexcept { ctx->pop_func(); }

system::set_expected_type::set_expected_type(parse_ctx* ctx, const std::string_view& expected) noexcept : ctx(ctx), expected(ctx->expected_type) { ctx->expected_type = expected; }
system::set_expected_type::~set_expected_type() noexcept { ctx->expected_type = expected; }

system::set_function_type::set_function_type(parse_ctx* ctx, const function_type t) noexcept : ctx(ctx), t(ctx->ftype) { ctx->ftype = t; }
system::set_function_type::~set_function_type() noexcept { ctx->ftype = t; }

system::push_list_index_upvalue::push_list_index_upvalue(parse_ctx* ctx, const size_t id) noexcept :
  ctx(ctx), prev_id(ctx->list_index_upvalue)
{ ctx->list_index_upvalue = id; }

system::push_list_index_upvalue::~push_list_index_upvalue() noexcept { ctx->list_index_upvalue = prev_id; }

system::change_chain_index::change_chain_index(parse_ctx* ctx) noexcept : ctx(ctx) { ctx->prev_chaining += 2; }
system::change_chain_index::~change_chain_index() noexcept { ctx->prev_chaining -= 2; }


std::tuple<std::string_view, size_t> system::rpn_conversion_ctx::convert_scope(const std::string_view& expr, block* arr, const size_t max_size) const {
  size_t counter = 0;
  std::array<std::string_view, 16> dot_arr;
  const size_t count = utils::string::split(expr, ".", dot_arr.data(), dot_arr.size());
  if (count == SIZE_MAX) throw std::runtime_error(std::format("Could not parse expr '{}', too many dots", expr));

  size_t size = 0;
  std::string_view lfn;
  for (size_t i = 0; i < count; ++i) {
    if (!lfn.empty()) throw std::runtime_error(std::format("Could not parse expr '{}', found scope call stack after left value function", expr));
    if (counter >= max_size) throw std::runtime_error(std::format("Could not parse expr '{}', not enough 'block' memory ({})", expr, max_size));

    std::array<std::string_view, 3> colon_arr;
    const size_t colon_count = utils::string::split(dot_arr[i], ":", colon_arr.data(), colon_arr.size());
    if (colon_count == SIZE_MAX) throw std::runtime_error(std::format("Could not parse expr '{}' in lvalue '{}', too many colons", dot_arr[i], expr));

    arr[counter] = { colon_arr[0], 0, 1 }; counter += 1;
    size += 1;
    if (colon_count > 2) {
      arr[counter] = { colon_arr[1], 1, 2 }; counter += 1;
      arr[counter] = { colon_arr[2], 0, 1 }; counter += 1;
      size += 2;
    } else if (colon_count == 2) {
      lfn = colon_arr[1];
    }
  }

  if (lfn.empty() && counter == 1) return std::make_tuple(arr[0].token, 0);

  for (size_t i = 0; i < counter; i += arr[i].args_count+1) {
    const size_t cursize = arr[i].size;
    arr[i].size = size;
    size -= cursize;
  }

  return std::make_tuple(lfn, counter);
}

namespace {
// text of a tavl token via its source span (empty for synthetic/empty tokens, e.g. call operator)
std::string_view node_text(const tavl::node* n, std::string_view src) {
  const auto& sp = n->token.span;
  return sp.offset == SIZE_MAX ? std::string_view() : src.substr(sp.offset, sp.size);
}
// direct children of a flat-prefix node (step by child_count+1 over its footprint)
std::vector<const tavl::node*> node_children(const tavl::node* n) {
  std::vector<const tavl::node*> r;
  for (size_t i = 1; i < n->child_count + 1; i += n[i].child_count + 1) r.push_back(n + i);
  return r;
}
}

size_t system::rpn_conversion_ctx::normalize(const std::vector<tavl::node>& tree, std::string_view src) {
  if (tree.empty()) return 0;
  const tavl::node* root = tree.data();
  // mirror convert_block's remove_brackets: if the whole input was a single braced block, descend
  // one level so its elements become the root rows (else we'd get an extra wrapping block).
  const auto rk = node_children(root);
  if (rk.size() == 1 && rk[0]->type == tavl::node_type::object) root = rk[0];
  return normalize_block(root, src);
}

size_t system::rpn_conversion_ctx::normalize_block(const tavl::node* block, std::string_view src) {
  size_t count = 0;
  for (const auto* row : node_children(block)) { normalize_row(row, src); count += 1; }
  return count;
}

// Port of convert_block's per-token body (system.cpp convert_block), sourcing (lvalue, rhs) from a
// tavl row node instead of text::parse_token. Same size/args bookkeeping; reuses convert_scope.
void system::rpn_conversion_ctx::normalize_row(const tavl::node* row, std::string_view src) {
  const size_t cur_size = output.size();
  std::string_view lfn = "__empty_lvalue";
  std::array<block, 16 * 3 + 1> arr;
  size_t lvalue_tokens_count = 0;

  std::string_view lvalue;
  const tavl::node* rhs = row;
  if (row->type == tavl::node_type::pair) {
    const auto op = node_text(row, src);
    if (op == "=" || op == "?=") {            // call/assignment: lhs = function, rhs = its argument
      const auto ch = node_children(row);
      lvalue = node_text(ch[0], src);          // lhs is a single token (scope paths arrive whole)
      rhs = ch[1];
    }
  }

  if (!lvalue.empty()) {
    const auto [func_name, count] = convert_scope(lvalue, arr.data(), arr.size());
    lvalue_tokens_count = count;
    if (!func_name.empty()) lfn = func_name;
  }

  for (size_t i = 0; i < lvalue_tokens_count; i += arr[i].args_count + 1) {
    output.push_back({ arr[i].token, arr[i].args_count, arr[i].size });
    for (size_t j = i + 1; j < i + arr[i].args_count + 1; ++j)
      output.push_back({ arr[j].token, arr[j].args_count, arr[j].size });
  }

  size_t lfn_index = SIZE_MAX;
  const bool lvalue_is_empty = lfn == "__empty_lvalue";
  const bool rhs_is_block = rhs->type == tavl::node_type::object;
  if (lvalue_is_empty && rhs_is_block) { output.push_back({ lfn, 0, 1 }); lfn_index = output.size() - 1; }
  if (!lvalue_is_empty) { output.push_back({ lfn, 0, 1 }); lfn_index = output.size() - 1; }

  const size_t block_size = output.size();
  size_t arguments_count = 0;
  if (rhs_is_block) arguments_count = normalize_block(rhs, src);
  else { normalize_expr(rhs, src); arguments_count = 1; }

  const size_t size = output.size() - block_size;
  for (size_t i = cur_size; i < cur_size + lvalue_tokens_count; i += output[i].args_count + 1)
    output[i].size += size + 1;
  if (lfn_index != SIZE_MAX) {
    output[lfn_index].args_count = arguments_count;
    output[lfn_index].size += size;
  }
}

// Expression (rvalue) -> prefix blocks, mirroring convert()'s output but reading tavl's already
// precedence-correct tree. Returns the footprint (1 + descendants).
size_t system::rpn_conversion_ctx::normalize_expr(const tavl::node* n, std::string_view src) {
  if (n->type == tavl::node_type::token) {
    output.push_back({ node_text(n, src), 0, 1 });   // leaf (incl. whole rvalue scope paths)
    return 1;
  }

  if (n->type == tavl::node_type::pair) {
    const auto ch = node_children(n);
    const auto op = node_text(n, src);

    if (op.empty()) {                                 // call f(...) / f{...}: pair with empty op token
      const size_t idx = output.size();
      output.push_back({ node_text(ch[0], src), 0, 1 });
      size_t s = 0, argn = 0;
      for (const auto* a : node_children(ch[1])) { s += normalize_expr(a, src); argn += 1; }
      output[idx].args_count = argn;
      output[idx].size += s;
      return 1 + s;
    }

    if (ch.size() == 1) {                             // unary prefix (rpn names: unary_minus/unary_plus)
      std::string_view o = op == "-" ? "unary_minus" : (op == "+" ? "unary_plus" : op);
      const size_t idx = output.size();
      output.push_back({ o, 1, 1 });
      const size_t s = normalize_expr(ch[0], src);
      output[idx].size += s;
      return 1 + s;
    }

    const size_t idx = output.size();                 // binary
    output.push_back({ op, 2, 1 });
    const size_t s = normalize_expr(ch[0], src) + normalize_expr(ch[1], src);
    output[idx].size += s;
    return 1 + s;
  }

  if (n->type == tavl::node_type::object) {           // anonymous block at expr position
    const size_t idx = output.size();
    output.push_back({ "__empty_lvalue", 0, 1 });
    const size_t cnt = normalize_block(n, src);
    output[idx].args_count = cnt;
    output[idx].size = output.size() - idx;
    return output.size() - idx;
  }

  // tuple / array group: emit elements in sequence (best-effort; not exercised by golden)
  size_t s = 0;
  for (const auto* e : node_children(n)) s += normalize_expr(e, src);
  return s;
}

void system::rpn_conversion_ctx::clear() {
  output.clear();
}

system::command_block::command_block() noexcept {}
system::command_block::command_block(const std::span<rpn_conversion_ctx::block>& data) noexcept : data(data) {}
system::command_block::command_block(const command_block& block, const size_t index) noexcept {
  if (index < block.size()) {
    data = std::span(&block.data[index], block.data[index].size);
  }
}

system::command_block system::command_block::find(const std::string_view& name) const {
  size_t counter = 1;
  while (counter < data.size()) {
    if (data[counter].token == name) return command_block(*this, counter);
    counter += data[counter].size;
  }
  return command_block();
}

system::command_block system::command_block::at(const size_t index) const {
  size_t curindex = 0;
  size_t counter = 1;
  while (counter < data.size()) {
    if (curindex == index) return command_block(*this, counter);
    counter += data[counter].size;
    curindex += 1;
  }
  return command_block();
}

std::string_view system::command_block::name() const { return !data.empty() ? data[0].token : std::string_view(); }
size_t system::command_block::args_count() const { return !data.empty() ? data[0].args_count : 0; }
size_t system::command_block::size() const { return data.size(); }
bool system::command_block::empty() const { return data.empty(); }

#define RFI(func) register_function<&func>
#define ROI(func) register_operator<&func>

using p_t = prng::xoshiro256starstar;
system::options::options() noexcept : seed(1), safety(safety::safe), error([](const std::string& msg) { throw std::runtime_error(msg); }), warning([](const std::string& msg) { std::cout << "WARN: " << msg << "\n"; }) {}
system::system(const options& opts) noexcept : seed(opts.seed), safet(opts.safety), error(opts.error), warning(opts.warning) {}

void system::toggle_safety() { this->safet = static_cast<enum safety>(!static_cast<bool>(this->safet)); }
bool system::safety() const { return static_cast<bool>(this->safet); }
void system::raise_error(const std::string& msg) const { error(msg); }
void system::raise_warning(const std::string& msg) const { warning(msg); }
uint64_t system::get_seed() const { return seed; }
void system::reseed(const uint64_t val) { seed = val; }
uint64_t system::parse_ctx::gen_value() { prng_s = p_t::next(prng_s); return p_t::value(prng_s); }

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
}

template <typename F, F f>
static void add_cmd(const system* sys, container* scr) {
  constexpr function_t fs[] = { &mathfunc_unsafe<F, f>, &mathfunc<F, f> };
  scr->cmds.emplace_back(fs[size_t(sys->safety())], INT64_C(0));
}

#define ADD_CMD(fn) add_cmd<decltype(&fn), fn>

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

    using fn_t = decltype(&internal::raweq);
    sys->setup_description<fn_t, &internal::raweq, void, is_valid_t<void>(nullptr)>(ctx, scr, args.name());

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
    //const auto exp = ctx->expected_type;

    auto end = e.make_label();   // a matched case jumps past the rest
    std::vector<size_t> values_indicies;

    const size_t prev_index = ctx->stack_types.size()-1;

    size_t stack_index = 0;
    {
      set_expected_type set(ctx, utils::type_name<any_object>());
      // сначала нужно вычислить рвалуе у value
      const auto valnode = args.find("value");
      sys->dispatch_node(ctx, scr, valnode);
      stack_index = ctx->stack_types.size() - 1;
      if (prev_index == stack_index) sys->raise_error(std::format("'value' node in 'switch' does not produce a value"));
      ctx->scope_stack.push_back(stack_index);
    }

    // other children

    size_t last_index = stack_index;
    {
      size_t offset = 1;
      while (offset < args.size()) {
        const auto curblock = command_block(args, offset);
        offset += curblock.size();

        //if (text::is_in_ignore_list(curblock.name())) continue;
        if (curblock.name() == custom_description_constant) continue;

        set_expected_type set(ctx, utils::type_name<any_object>());

        const size_t start = scr->block_descs.size();
        const auto valblock = curblock.find("value");
        sys->dispatch_node(ctx, scr, valblock, "__object_block");
        const auto cd = valblock.find(custom_description_constant); // value ??
        sys->setup_block_description(ctx, scr, valblock.name(), command_block(cd, 1).name(), start);

        if (last_index == ctx->stack_types.size() - 1) sys->raise_error(std::format("No new values on stack after 'value' node computation?"));
        last_index = ctx->stack_types.size() - 1;
        values_indicies.push_back(last_index);
      }
    }
    
    size_t counter = 0;
    size_t offset = 1;
    while (offset < args.size()) {
      const auto curblock = command_block(args, offset);
      offset += curblock.size();
      const size_t arg_index = counter;
      counter += 1;

      const size_t curid = values_indicies[arg_index];

      sys->push_basic_function(ctx, scr, basicf::cmpeq2, pack2(int32_t(stack_index), int32_t(curid))); // push
      auto next_case = e.make_label();
      e.jump_to(basicf::condjump, next_case);

      sys->dispatch_node(ctx, scr, curblock);

      e.bind(next_case);

      e.jump_to(basicf::jump, end);
    }

    e.bind(end);

    // чистим за собой ненужное
    sys->push_basic_function(ctx, scr, basicf::erase, stack_index);
    for (const auto id : values_indicies) { sys->push_basic_function(ctx, scr, basicf::erase, id); }

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

    // if this block has 1 arg with str, then next child would be __empty_lvalue
    // description?
    //const size_t desc_start = scr->block_descs.size();

    push_list_index_upvalue pliu(ctx, index);
    const auto nextblock = command_block(args, 1 + child.size());
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

void system::scope_exit(parse_ctx* ctx, container* scr, const size_t count) const {
  for (size_t i = 0; i < count; ++i) {
    const int64_t index = ctx->scope_stack.back();
    ctx->scope_stack.pop_back();
    push_basic_function(ctx, scr, basicf::erase, index);
    ctx->erase(index);
  }
}

void system::register_function(command_data data) {
  if (data.type == command_data::ftype::invalid) raise_error(std::format("Cannot register '{}' with function type 'invalid'", data.name));

  if (data.type == command_data::ftype::function_t) {
    if (!text::is_valid_function_name(data.name)) raise_error(std::format("'{}' is not valid function name", data.name));
  } else {
    if (!text::is_valid_operator_name(data.name)) raise_error(std::format("'{}' is not valid function name", data.name));
    if (text::has_common_symbols(data.name) && text::has_special_symbols(data.name))
      raise_error(std::format("Do not mix math operator symbols and common function name"));
  }

  using cont_t = std::unordered_map<std::string, command_data>;
  auto itr = mfuncs.find(data.name);
  if (itr == mfuncs.end()) {
    itr = mfuncs.emplace(std::make_pair(data.name, cont_t{})).first;
  } else {
    if (itr->second.empty()) raise_error(std::format("'{}' empty?", data.name));
    if (data.type != itr->second.begin()->second.type) 
      raise_error(std::format("Cannot register several functions '{}' with defferent types", data.name));
    if (data.type == command_data::ftype::operator_t) {
      const auto& another = itr->second.begin()->second;
      const bool invalid_state = 
        data.priority != another.priority || 
        data.arg_count != another.arg_count ||
        data.assoc != another.assoc;

      if (invalid_state) 
        raise_error(std::format("Operators props must be the same for all overloaded operators ({},{},{}) != ({},{},{})", data.priority, data.arg_count, static_cast<uint32_t>(data.assoc), another.priority, another.arg_count, static_cast<uint32_t>(another.assoc)));
    }
  }

  const auto scope_itr = itr->second.find(std::string(data.expected_scope));
  if (scope_itr != itr->second.end()) raise_error(std::format("'{}' is already registered for scope '{}'", data.name, data.expected_scope));

  itr->second[std::string(data.expected_scope)] = std::move(data);
}

void system::setup_block_description(parse_ctx* ctx, container* scr, const std::string_view& token, const std::string_view& custom_desc, const size_t start) const {
  using sv_t = container::command_description::global_string_view;
  sv_t tok{};
  sv_t cd{};

  if (token == "__empty_lvalue") {
    tok = { SIZE_MAX, SIZE_MAX };
  } else if (!check_is_str_part_of(scr->globals[0], token)) {
    basicf id = basicf::invalid;
    if (token == "__object_block") id = basicf::object_block;
    else if (token == "__string_block") id = basicf::string_block;
    else if (token == "__effect_block") id = basicf::effect_block;
    else id = find_basicf(token);
    // warning
    if (id == basicf::invalid) raise_warning(std::format("Function name '{}' could not easily store at description struct, use another method", token));
    tok = { static_cast<size_t>(id), SIZE_MAX };
  } else {
    tok = { size_t(token.data() - scr->globals[0].data()), token.size() };
  }

  if (custom_desc.empty()) {
    cd = { SIZE_MAX, SIZE_MAX };
  } else if (!check_is_str_part_of(scr->globals[0], custom_desc)) {
    const basicf id = find_basicf(custom_desc);
    if (id == basicf::invalid) raise_warning(std::format("Function name '{}' could not easily store at description struct, use another method", custom_desc));
    cd = { static_cast<size_t>(id), SIZE_MAX };
  } else {
    cd = { size_t(custom_desc.data() - scr->globals[0].data()), custom_desc.size() };
  }

  const int64_t scope_index = ctx->scope_stack.empty() ? -1 : ctx->scope_stack.back();
  const size_t size = scr->block_descs.size() - start + 1;
  const size_t index = scr->cmds.size()-1;
  scr->block_descs.push_back({
    tok, cd, size, 0, index, scope_index
  });

  const size_t current_index = scr->block_descs.size() - 1;
  size_t offset = 1;
  size_t counter = 0;
  while (offset < size) {
    const auto& desc = scr->block_descs[current_index - offset];
    offset += desc.size;

    counter += 1;
  }

  scr->block_descs[current_index].args_count = counter;
}

namespace {

// How a basic instruction affects the compile-time type-stack on push.
// Most ops push a static type; a few push a type derived from the script's args/saved/stack.
enum class push_kind : uint8_t {
  none, b_bool, b_double, b_int64, b_string,
  root,           // scr->args[0].type   (pushroot)
  arg_indexed,    // scr->args[arg].type  (pusharg / pushargvalue)
  saved_indexed,  // scr->saved[arg].type (pushctxvalue)
  same_as_top,    // duplicate the current stack top (current)
  thisarg, thisctx, thisctxlist,
};

// One row per basic instruction — the single source of truth for its stack effect & description.
// safe/unsafe are the two execution function pointers (equal when the op has no unsafe variant);
// safe == nullptr marks a basicf value that is NOT directly emittable (blocks/combinators).
struct insn_info {
  function_t safe = nullptr;
  function_t unsafe = nullptr;
  uint8_t   pops = 0;            // type-slots popped before the push
  push_kind pushes = push_kind::none;
  bool      has_return = false;  // command_description flag (independent of the stack push)
  uint8_t   arg_count = 0;       // command_description flag
};

constexpr size_t basicf_count = static_cast<size_t>(basicf::invalid) + 1;

constexpr std::array<insn_info, basicf_count> make_insn_table() {
  std::array<insn_info, basicf_count> t{};
  using pk = push_kind;
  auto row = [&](basicf id, function_t s, function_t u, uint8_t pops, pk pushes, bool ret, uint8_t ac) {
    t[static_cast<size_t>(id)] = insn_info{ s, u, pops, pushes, ret, ac };
  };
  //  basicf                  safe              unsafe                 pops  pushes           ret    args
  row(basicf::jump,          &jump,            &jump,                  0,    pk::none,        false, 0);
  row(basicf::andbin,        &andbin,          &andbin_unsafe,         2,    pk::b_bool,      true,  2);
  row(basicf::sum,           &sum,             &sum_unsafe,            2,    pk::b_double,    true,  2);
  row(basicf::sumsetstack,   &sumsetstack,     &sumsetstack_unsafe,    0,    pk::none,        false, 0);
  row(basicf::mulsetstack,   &mulsetstack,     &mulsetstack_unsafe,    0,    pk::none,        false, 0);
  row(basicf::cmpeq2,        &cmpeq2,          &cmpeq2_unsafe,         0,    pk::b_bool,      true,  0);
  row(basicf::cmplesseqd2,   &cmplesseqd2,     &cmplesseqd2_unsafe,    0,    pk::b_bool,      true,  0);
  row(basicf::notfn,         &invb,            &invb_unsafe,           1,    pk::b_bool,      true,  1);
  row(basicf::unary_plus,    &pos,             &pos_unsafe,            1,    pk::b_double,    true,  1);
  row(basicf::unary_minus,   &neg,             &neg_unsafe,            1,    pk::b_double,    true,  1);
  row(basicf::andjump,       &andjump,         &andjump_unsafe,        2,    pk::b_bool,      true,  2);
  row(basicf::orjump,        &orjump,          &orjump_unsafe,         2,    pk::b_bool,      false, 2);
  row(basicf::condjump,      &condjump,        &condjump_unsafe,       1,    pk::none,        false, 1);
  row(basicf::condjump_get,  &condjump_get,    &condjump_get_unsafe,   0,    pk::none,        false, 0);
  row(basicf::condjumpt_get, &condjumpt_get,   &condjumpt_get_unsafe,  0,    pk::none,        false, 0);
  row(basicf::pushbool,      &pushbool,        &pushbool,              0,    pk::b_bool,      true,  0);
  row(basicf::pushvalue,     &pushvalue,       &pushvalue,             0,    pk::b_double,    true,  0);
  row(basicf::chance,        &pushchance,      &pushchance,            0,    pk::b_double,    true,  0);
  row(basicf::pushint,       &pushint,         &pushint,               0,    pk::b_int64,     true,  0);
  row(basicf::pushstring,    &pushstring,      &pushstring,            0,    pk::b_string,    true,  0);
  row(basicf::pushroot,      &pushroot,        &pushroot,              0,    pk::root,        true,  0);
  row(basicf::pushthis,      &pushthis,        &pushthis,              0,    pk::none,        true,  0); // pushes outside this fn
  row(basicf::pushprev,      &pushprev,        &pushprev,              0,    pk::none,        true,  0); // pushes outside this fn
  row(basicf::pushreturn,    &pushreturn,      &pushreturn,            1,    pk::none,        false, 1);
  row(basicf::pusharg,       &pusharg,         &pusharg,               0,    pk::arg_indexed, true,  0);
  row(basicf::pushinvalid,   &pushinvalid,     &pushinvalid,           0,    pk::none,        true,  0);
  row(basicf::argcontext,    &pushargcontext,  &pushargcontext,        0,    pk::thisarg,     true,  0);
  row(basicf::context,       &pushcontext,     &pushcontext,           0,    pk::thisctx,     true,  0);
  row(basicf::erase,         &erase,           &erase,                 0,    pk::none,        false, 0);
  row(basicf::current,       &pushcurrent,     &pushcurrent,           0,    pk::same_as_top, true,  0);
  row(basicf::pushargvalue,  &pushargvalue,    &pushargvalue,          0,    pk::arg_indexed, true,  0);
  row(basicf::setargrvalue,  &setargrvalue,    &setargrvalue,          1,    pk::none,        false, 0);
  row(basicf::setarglvalue,  &setarglvalue,    &setarglvalue,          0,    pk::none,        false, 0);
  row(basicf::pushctxvalue,  &pushctxvalue,    &pushctxvalue,          0,    pk::saved_indexed,true,0);
  row(basicf::savectxrvalue, &savectxrvalue,   &savectxrvalue,         1,    pk::none,        false, 0);
  row(basicf::savectxlvalue, &savectxlvalue,   &savectxlvalue,         0,    pk::none,        false, 0);
  row(basicf::pushlist,      &pushlist,        &pushlist,              0,    pk::thisctxlist, true,  0);
  return t;
}

constexpr auto insn_table = make_insn_table();

}  // namespace

size_t system::push_basic_function(parse_ctx* ctx, container* scr, const basicf id, const int64_t arg) const {
  function_name_changer fnc(ctx, to_string(id));

  const auto& info = insn_table[static_cast<size_t>(id)];
  if (info.safe == nullptr) raise_error(std::format("'{}' is not supported here", to_string(id)));

  scr->cmds.push_back(container::command(safety() ? info.safe : info.unsafe, arg));

  for (uint8_t i = 0; i < info.pops; ++i) ctx->pop();
  switch (info.pushes) {
    case push_kind::none:                                              break;
    case push_kind::b_bool:        ctx->push<bool>();                  break;
    case push_kind::b_double:      ctx->push<double>();                break;
    case push_kind::b_int64:       ctx->push<int64_t>();               break;
    case push_kind::b_string:      ctx->push<std::string_view>();      break;
    case push_kind::root:          ctx->push(scr->args[0].type);       break;
    case push_kind::arg_indexed:   ctx->push(scr->args[arg].type);     break;
    case push_kind::saved_indexed: ctx->push(scr->saved[arg].type);    break;
    case push_kind::same_as_top:
      if (ctx->stack_types.empty()) raise_error(std::format("Trying to use 'pushcurrent' function on an empty stack"));
      ctx->push(ctx->stack_types.back());
      break;
    case push_kind::thisarg:       ctx->push<internal::thisarg>();     break;
    case push_kind::thisctx:       ctx->push<internal::thisctx>();     break;
    case push_kind::thisctxlist:   ctx->push<internal::thisctxlist>(); break;
  }

  container::command_description desc(
    { static_cast<size_t>(id), SIZE_MAX }, info.arg_count,
    false, true, info.has_return, false, ctx->nest_level, SIZE_MAX
  );
  scr->descs.emplace_back(desc);

  if (scr->cmds.size() != scr->descs.size()) raise_error(std::format("Unconsistent descriptions {} != {}", scr->cmds.size(), scr->descs.size()));

  return scr->cmds.size()-1;
}

size_t system::emitter::emit(const basicf op, const int64_t arg) const { return sys->push_basic_function(ctx, scr, op, arg); }
size_t system::emitter::emit_string(const std::string_view& str) const { return sys->push_string(ctx, scr, str); }
system::emitter::label system::emitter::make_label() const { return label{}; }
void system::emitter::jump_to(const basicf op, label& l) const { l.sites.push_back(emit(op, 0)); }
void system::emitter::mark(label& l, const size_t cmd_index) const { l.sites.push_back(cmd_index); }
void system::emitter::bind(label& l) const {
  const size_t target = scr->cmds.size();
  for (const size_t site : l.sites) scr->cmds[site].arg = target;
}

// we wanna save the position of the string in the global
bool check_is_str_part_of(const std::string_view& big_str, const std::string_view& small_str) noexcept {
  return (big_str.data() <= small_str.data()) && (big_str.data() + big_str.size() >= small_str.data() + small_str.size());
}

size_t system::push_string(parse_ctx* ctx, container* scr, const std::string_view& str) const {
  if (check_is_str_part_of(scr->globals[0], str)) {
    const int64_t pos = str.data() - scr->globals[0].data();
    if (!check_value(pos, packed_pos_bit_size)) raise_error(std::format("String '{}' position in script string cannot be packed in {} bit ???", str, packed_pos_bit_size));
    if (!check_value(str.size(), packed_size_bit_size)) raise_error(std::format("String '{}' size in script string cannot be packed in {} bit ???", str, packed_size_bit_size));
    push_basic_function(ctx, scr, basicf::pushstring, packstrid(0, pos, str.size()));
    return 1;
  }

  // variant 2
  size_t index = SIZE_MAX;
  for (size_t i = 0; i < scr->globals.size(); ++i) {
    const size_t pos = scr->globals[i].find(str);
    if (pos == std::string::npos) continue;
    index = i;
    break;
  }

  if (index == SIZE_MAX) {
    if (scr->globals.size() >= UINT8_MAX) raise_error(std::format("Script would store global string index in 8bit value, too many global strings"));
    scr->globals.emplace_back(std::string(str));
    index = scr->globals.size() - 1;
  }

  const size_t start = scr->globals[index].find(str);
  if (!check_value(index, sizeof(uint8_t) * CHAR_BIT)) raise_error(std::format("Global string index {} cannot be packed in {} bit ???", index, sizeof(uint8_t) * CHAR_BIT));
  if (!check_value(start, packed_pos_bit_size)) raise_error(std::format("String '{}' position in script string cannot be packed in {} bit ???", str, packed_pos_bit_size));
  if (!check_value(str.size(), packed_size_bit_size)) raise_error(std::format("String '{}' size in script string cannot be packed in {} bit ???", str, packed_size_bit_size));
  push_basic_function(ctx, scr, basicf::pushstring, packstrid(index, start, str.size()));
  return 1;
}

size_t system::dispatch_node(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view& override_lvalue) const {
  if (block.empty()) return 0;

  const auto exp_t = ctx->expected_type;
  const bool any_type_expected = type_is_any_type(exp_t);
  
  auto funcname = block.name();
  if (block.args_count() == 0 && block.size() == 1) { // rvalue
    if (funcname == "condition") raise_error(std::format("'condition' is not allowed here"));

    if (text::is_bool(funcname)) {
      set_function_type sft(ctx, function_type::rvalue);
      push_basic_function(ctx, scr, basicf::pushbool, text::as_bool(funcname));
      setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
      return 1;
    }

    // int???

    if (double val; text::is_number(funcname, val)) {
      set_function_type sft(ctx, function_type::rvalue);
      push_basic_function(ctx, scr, basicf::pushvalue, std::bit_cast<int64_t>(val));
      setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
      return 1;
    }

    if (exp_t == utils::type_name<std::string_view>()) {
      set_function_type sft(ctx, function_type::rvalue);
      push_string(ctx, scr, block.data[0].token);
      setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
      return 1;
    }

    {
      set_function_type sft(ctx, function_type::lvalue);

      std::array<rpn_conversion_ctx::block, 16 * 3+1> arr;
      auto [local_fname, count] = ctx->rpn_ctx.convert_scope(funcname, arr.data(), arr.size());
      funcname = local_fname;

      if (count == 0) {
        const auto& itr = mfuncs.find(std::string(block.name()));
        if (itr == mfuncs.end() && any_type_expected) {
          set_function_type sft(ctx, function_type::rvalue);
          push_string(ctx, scr, block.name());
          setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
          return 1;
        } else if (itr == mfuncs.end()) raise_error(std::format("Could not find function '{}'", block.name()));

        set_function_type sft(ctx, function_type::rvalue);
        function_name_changer fnc(ctx, block.name());

        auto scope_itr = itr->second.find(std::string(ctx->current_scope_type()));
        if (scope_itr == itr->second.end()) { scope_itr = itr->second.find(std::string(scope_type_name<void>())); }
        if (scope_itr == itr->second.end()) raise_error(std::format("Could not find function '{}' for scope type '{}'", block.name(), ctx->current_scope_type()));

        emitter e{this, ctx, scr};
        const size_t count = std::invoke(scope_itr->second.init, e, block);
        setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
        return count;
      }

      if (!local_fname.empty()) {
        for (size_t i = 0; i < count; i += arr[i].args_count+1) {
          arr[i].size += 1;
        }

        arr[count] = rpn_conversion_ctx::block{ local_fname, 0, 1 };
        count += 1;
      }
      
      bool has_invalid_func_name = false;
      command_block cb(std::span(arr.data(), count));
      if (any_type_expected) {
        size_t curindex = 1;
        while (curindex < cb.size() && !has_invalid_func_name) {
          const auto block = command_block(cb, curindex);
          curindex += block.size();
          const auto& itr = mfuncs.find(std::string(block.name()));
          has_invalid_func_name = itr == mfuncs.end();
        }
      }

      if (has_invalid_func_name) {
        set_function_type sft(ctx, function_type::rvalue);
        push_string(ctx, scr, block.name());
        setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
        return 1;
      } else {
        return dispatch_node(ctx, scr, cb);
      }
    }
  }

  auto prevname = funcname;
  if (!override_lvalue.empty()) funcname = override_lvalue;

  // how to check named arg?
  const bool is_not_overriden = override_lvalue.empty();
  const bool is_condition = block.name() == "condition";
  const bool is_subblock = funcname == "__empty_lvalue";
  if (is_subblock) {
    auto curid = basicf::object_block;
    if (type_is_bool(exp_t)) curid = basicf::AND;
    else if (type_is_fundamental(exp_t)) curid = basicf::ADD;
    else if (type_is_string(exp_t)) curid = basicf::string_block;
    else if (type_is_void(exp_t)) curid = basicf::effect_block;

    const size_t desc_start = scr->block_descs.size();
    const size_t count = fold_block(ctx, scr, block, curid);
    const auto cd = block.find(custom_description_constant);
    setup_block_description(ctx, scr, to_string(curid), command_block(cd, 1).name(), desc_start);
    return count;
  }

  const bool is_effect_block = funcname == "__effect_block";
  const bool is_string_block = funcname == "__string_block";
  const bool is_object_block = funcname == "__object_block";
  const bool is_special_case = is_effect_block || is_string_block || is_object_block;
  if (is_special_case) {
    auto curid = basicf::effect_block;
    if (is_string_block) curid = basicf::string_block;
    if (is_object_block) curid = basicf::object_block;

    const size_t desc_start = scr->block_descs.size();
    const size_t count = fold_block(ctx, scr, block, curid);
    const auto cd = block.find(custom_description_constant);
    setup_block_description(ctx, scr, funcname, command_block(cd, 1).name(), desc_start);
    return count;
  }

  const size_t desc_start = scr->block_descs.size();

  function_name_changer fnc(ctx, prevname);
  set_function_type sft(ctx, function_type::lvalue); // probably function_type is meaningless
  const auto& itr = mfuncs.find(std::string(funcname));
  if (itr == mfuncs.end()) raise_error(std::format("Could not find function '{}'", funcname));
  auto scope_itr = itr->second.find(std::string(ctx->current_scope_type()));
  if (scope_itr == itr->second.end()) { scope_itr = itr->second.find(std::string(scope_type_name<void>())); }
  if (scope_itr == itr->second.end()) raise_error(std::format("Could not find function '{}' for scope type '{}'", block.name(), ctx->current_scope_type()));

  { emitter e{this, ctx, scr}; std::invoke(scope_itr->second.init, e, block); }

  if (is_subblock || is_condition || is_not_overriden) {
    const auto cd = block.find(custom_description_constant);
    setup_block_description(ctx, scr, prevname, command_block(cd, 1).name(), desc_start);
  }

  return block.size(); // ?
}

size_t system::fold_block(parse_ctx* ctx, container* scr, const command_block& block, const basicf id) const {
  if (block.args_count() == 0 && block.size() == 1) return dispatch_node(ctx, scr, block);

  if (id == basicf::NAND) { 
    const size_t size = fold_block(ctx, scr, block, basicf::AND);
    push_basic_function(ctx, scr, basicf::notfn, 0);
    return size;
  }

  if (id == basicf::NOR) {
    const size_t size = fold_block(ctx, scr, block, basicf::OR);
    push_basic_function(ctx, scr, basicf::notfn, 0);
    return size;
  }

  auto curid = id;
  if (curid == basicf::invalid) {
    const auto exp_t = ctx->expected_type;
    if (type_is_void(exp_t)) curid = basicf::effect_block;
    else if (type_is_bool(exp_t)) curid = basicf::AND;
    else if (type_is_fundamental(exp_t)) curid = basicf::ADD;
    else if (type_is_string(exp_t)) curid = basicf::string_block;
    else curid = basicf::object_block;
  }

  bool boolean_and_block = false;
  bool boolean_or_block = false;
  container::command cmd;
  switch (curid) {
    case basicf::ADD: {
      cmd = container::command(&mathfunc<decltype(&internal::rawadd), &internal::rawadd>, INT64_C(0));
      break;
    }

    case basicf::MUL: {
      cmd = container::command(&mathfunc<decltype(&internal::rawmul), &internal::rawmul>, INT64_C(0));
      break;
    }

    case basicf::AND: {
      cmd = container::command(&andjump, INT64_C(0));
      boolean_and_block = true;
      break;
    }

    case basicf::OR: {
      cmd = container::command(&orjump, INT64_C(0));
      boolean_or_block = true;
      break;
    }

    case basicf::NAND: {
      cmd = container::command(&andjump, INT64_C(0));
      boolean_and_block = true;
      break;
    }

    case basicf::NOR: {
      cmd = container::command(&orjump, INT64_C(0));
      boolean_or_block = true;
      break;
    }

    // объекты? строки? 
    case basicf::effect_block: { break; }
    case basicf::string_block: { break; }
    case basicf::object_block: { break; }
    case basicf::string_subblock: { break; }
    case basicf::object_subblock: { break; }

    default: raise_error(std::format("Wrong place for '{}'", to_string(id)));
  }

  emitter e{this, ctx, scr};
  auto end = e.make_label();   // every short-circuit / cond jump in this block resolves to the block end

  size_t current_stack_size = ctx->stack_types.size();

  size_t counter = 0;
  size_t offset = 1;
  while (offset < block.size()) {
    const auto child = command_block(block, offset);
    offset += child.size();

    if (curid == basicf::effect_block || curid == basicf::string_subblock || curid == basicf::object_subblock) {
      if (const auto cond_block = child.find("condition"); !cond_block.empty()) {
        dispatch_node(ctx, scr, cond_block, "AND");
        if (current_stack_size >= ctx->stack_types.size()) raise_error(std::format("Block '{}' does not push any value?", cond_block.name()));
        e.jump_to(basicf::condjump, end);
      }
    }

    if (text::is_in_ignore_list(child.name())) continue;

    dispatch_node(ctx, scr, child);
    if (curid == basicf::effect_block) continue;
    if (current_stack_size >= ctx->stack_types.size()) raise_error(std::format("Block '{}' does not push any value?", child.name()));
    if (ctx->is<ignore_value>()) { ctx->pop(); continue; }
    if (curid == basicf::string_block) continue;
    if (curid == basicf::object_block) continue;

    const size_t curarg = counter;
    counter += 1;

    if (curarg == 0) {
      if (boolean_and_block) e.jump_to(basicf::condjump_get, end);
      else if (boolean_or_block) e.jump_to(basicf::condjumpt_get, end);

      continue;
    }

    if (ctx->is<double>() && (boolean_and_block || boolean_or_block)) {
      setup_type_conversion<double, bool>(ctx, scr);
    } else if (ctx->is<int64_t>() && (boolean_and_block || boolean_or_block)) {
      setup_type_conversion<int64_t, bool>(ctx, scr);
    } else if (ctx->is<bool>() && !(boolean_and_block || boolean_or_block)) {
      setup_type_conversion<bool, double>(ctx, scr);
    }

    scr->cmds.push_back(cmd);
    container::command_description desc(
      { static_cast<size_t>(curid), SIZE_MAX }, 2, 
      false, true, curid != basicf::effect_block, curid != basicf::effect_block, ctx->nest_level, SIZE_MAX
    );
    scr->descs.emplace_back(desc);

    ctx->pop();
    ctx->pop();

    if (boolean_and_block || boolean_or_block) {
      ctx->push<bool>();
      e.mark(end, scr->cmds.size()-1); // andjump / orjump is itself a short-circuit site
    } else {
      ctx->push<double>();
    }

    if (scr->cmds.size() != scr->descs.size()) raise_error(std::format("Unconsistent descriptions {} != {}, after block '{}'", scr->cmds.size(), scr->descs.size(), child.name()));
  }

  e.bind(end);
  if (current_stack_size == ctx->stack_types.size()) ctx->push<ignore_value>(); // no value

  return block.size();
}

void system::check_is_str_part_of_and_throw(const std::string_view& big_str, const std::string_view& small_str) const {
  if (!(big_str.data() <= small_str.data() && big_str.data() + big_str.size() >= small_str.data() + small_str.size())) 
    raise_error(std::format("'{}' is not part of script string? ( {} {} | {} {} )", small_str, std::bit_cast<size_t>(big_str.data()), std::bit_cast<size_t>(big_str.data() + big_str.size()), std::bit_cast<size_t>(small_str.data()), std::bit_cast<size_t>(small_str.data() + small_str.size())));
}

void system::configure_parser(tavl::parser& p) const {
  p.clear_operators();
  // structural call operators — not registered in mfuncs; lowest precedence, right-assoc.
  // `abc = {...}` / `abc ?= {...}` lower than any math so the rhs expression binds first.
  p.add_operator("=",  tavl::op_fixity::binary, 1, tavl::op_assoc::right);
  p.add_operator("?=", tavl::op_fixity::binary, 1, tavl::op_assoc::right);

  for (const auto& [name, scopes] : mfuncs) {
    if (scopes.empty()) continue;
    const auto& cd = scopes.begin()->second;        // operators don't vary by scope
    if (cd.type != command_data::ftype::operator_t) continue;

    // `unary_plus`/`unary_minus` are rpn-only aliases (see convert()); the source symbol is +/-,
    // distinguished from the binary form by tavl op_fixity instead of a separate name.
    std::string_view sym = name;
    if (name == "unary_minus") sym = "-";
    else if (name == "unary_plus") sym = "+";

    tavl::op_fixity fixity;
    switch (static_cast<command_data::math_ftype>(cd.arg_count)) {
      case command_data::math_ftype::prefix:  fixity = tavl::op_fixity::prefix;  break;
      case command_data::math_ftype::postfix: fixity = tavl::op_fixity::postfix; break;
      default:                                fixity = tavl::op_fixity::binary;  break;
    }
    const auto assoc = (cd.assoc == command_data::associativity::right)
      ? tavl::op_assoc::right : tavl::op_assoc::left;

    if (text::is_special_operator(sym)) p.add_operator(sym, fixity, cd.priority, assoc);
    else p.add_litteral_operator(sym, fixity, cd.priority, assoc);
  }
}

system::command_data::ftype system::get_token_type(const std::string_view& name) const {
  const auto itr = mfuncs.find(std::string(name)); // such a pain
  if (itr == mfuncs.end()) return command_data::ftype::invalid;
  if (itr->second.empty()) return command_data::ftype::invalid;
  return itr->second.begin()->second.type;
}

std::tuple<int32_t, int32_t, system::command_data::associativity, system::command_data::ftype> system::get_token_caps(const std::string_view& name) const {
  const auto itr = mfuncs.find(std::string(name)); // such a pain
  if (itr == mfuncs.end()) return std::make_tuple(0, 0, system::command_data::associativity::left, command_data::ftype::invalid);
  if (itr->second.empty()) return std::make_tuple(0, 0, system::command_data::associativity::left, command_data::ftype::invalid);

  int32_t args_count = 0;
  for (auto it = itr->second.begin(); it != itr->second.end(); ++it) {
    args_count = std::max(args_count, it->second.arg_count);
  }

  auto it = itr->second.begin();
  return std::make_tuple(it->second.priority, args_count, it->second.assoc, it->second.type);
}

size_t system::patch_prev_functions_descriptions(container* scr, const size_t start) const {
  if (scr->descs.size() == 0) return 0;

  size_t counter = 0;
  const size_t last_index = scr->cmds.size()-1;
  for (size_t i = start; i < scr->descs.size()-1; ++i) {
    if (scr->descs[i].parent == SIZE_MAX) {
      scr->descs[i].parent = last_index;
      counter += 1;
    }
  }

  return counter;
}

system::parse_ctx::parse_ctx() noexcept :
  ftype(function_type::lvalue), nest_level(0), unlimited_func_index(SIZE_MAX), list_index_upvalue(SIZE_MAX), prev_chaining(0)
{}

bool system::parse_ctx::is_func_subblock() const {
  if (function_names.size() < 2) return false;
  return function_names[function_names.size() - 2] == function_names.back();
}

void system::parse_ctx::push_func(const std::string_view& name) {
  function_names.push_back(name);
}

void system::parse_ctx::pop_func() {
  function_names.pop_back();
}

size_t system::parse_ctx::current_scope_index() const {
  if (scope_stack.empty()) return SIZE_MAX;
  return scope_stack.back();
}

std::string_view system::parse_ctx::current_scope_type() const {
  if (scope_stack.empty()) return std::string_view();
  return stack_types[scope_stack.back()];
}

bool system::parse_ctx::is_ignore() const { return type_is_ignore(top()); }
bool system::parse_ctx::is_bool() const { return type_is_bool(top()); }
bool system::parse_ctx::is_integral() const { return type_is_integral(top()); }
bool system::parse_ctx::is_number() const { return type_is_floating_point(top()); }
bool system::parse_ctx::is_fundamental() const { return type_is_fundamental(top()); }
bool system::parse_ctx::is_string() const { return type_is_string(top()); }
bool system::parse_ctx::is_object() const { return type_is_object(top()); }
bool system::parse_ctx::pop_while_ignore() { if (!stack_types.empty() && is_ignore()) { pop(); return true; } return false; }
void system::parse_ctx::push(const std::string_view& type) {
  stack_types.push_back(type);
}
void system::parse_ctx::pop() {
  if (stack_types.empty()) throw std::runtime_error(std::format("Trying to remove value from empty stack, current function '{}'", function_names.back()));
  stack_types.pop_back();
}

void system::parse_ctx::erase(const size_t index) {
  if (index >= stack_types.size()) throw std::runtime_error(std::format("Trying to remove value #{} from stack with {} values, current function '{}'", index, stack_types.size(), function_names.back()));
  stack_types.erase(stack_types.begin() + index);
}

std::string_view system::parse_ctx::top() const { return stack_types.back(); }

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}


// better to reduce this file, i can place rpn_context to another file
