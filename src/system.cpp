#include "devils_script/system.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cassert>
#include <optional>
#include <cstring>
#include <vector>
#include "devils_script/context.h"
#include "devils_script/string-utils.hpp"

namespace devils_script {

namespace {
class parse_rejected final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};
}

bool type_is_ignore(const std::string_view& type) noexcept { return type == utils::type_name<ignore_value>(); }
bool type_is_void(const std::string_view& type) noexcept { return type == utils::type_name<void>() || type == utils::type_name<utils::void_t>(); }
bool type_is_bool(const std::string_view& type) noexcept { return type == utils::type_name<bool>(); }
bool type_is_integral(const std::string_view& type) noexcept { return type == utils::type_name<script_int_t>(); }
bool type_is_floating_point(const std::string_view& type) noexcept { return type == utils::type_name<script_float_t>(); }
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
bool stack_element::view::valid() const { return _mem && !_type.empty() && memcmp(_mem, invalid_memory, MAXIMUM_STACK_VAL_SIZE) != 0; }
std::string_view stack_element::view::type() const { return _type; }
bool operator==(const stack_element::view& v1, const stack_element::view& v2) {
  if (!v1._mem || !v2._mem) return v1._mem == v2._mem && v1._type == v2._type;
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

void context::create_lists(const script_container* scr) {
  lists.clear();
  // Reserve the script's whole-subtree peak so nested `execute` list-frames (list_frame_enter) never
  // reallocate ctx->lists mid-run — a list-pipeline callback holds a reference into this vector across
  // any sub-script call, so a reallocation there would dangle it (crash on map/filter write-back).
  lists.reserve(std::max(scr->max_lists, scr->lists.size()));
  lists.resize(scr->lists.size());
}

static size_t rpn_block_direct_child_count(const system::rpn_conversion_ctx::block* data, const size_t size) {
  if (data == nullptr || size == 0) return 0;

  using kind = system::rpn_conversion_ctx::block_kind;
  switch (data[0].kind) {
    case kind::scope_path: return 0;
    case kind::scope_path_call: return size > 1 ? 1 : 0;
    case kind::node:
    case kind::string_literal:
    case kind::nullable_call:
    case kind::braced_call:
    case kind::nullable_braced_call:
      break;
  }

  size_t count = 0;
  for (size_t i = 1; i < size; i += data[i].size) count += 1;
  return count;
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

namespace text {

bool is_bool(const std::string_view& str) noexcept {
  return str == "true" || str == "false";
}

bool as_bool(const std::string_view& str) noexcept {
  return str == "true";
}

bool is_number(const std::string_view& str, script_float_t& val) noexcept {
  const auto last = str.data() + str.size();
  const auto [ptr, ec] = std::from_chars(str.data(), last, val);
  return ec == std::errc() && ptr == last;
}

bool is_integer(const std::string_view& str, script_int_t& val) noexcept {
  const auto last = str.data() + str.size();
  const auto [ptr, ec] = std::from_chars(str.data(), last, val);
  return ec == std::errc() && ptr == last;
}

constexpr std::string_view engalpha = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz_";
constexpr std::string_view specials = "!@#$%^&*-+=<>?/;\\|`~";
constexpr std::string_view numbers = "1234567890";
constexpr std::string_view reserved_tokens[] = {
  "condition", "custom_description", "value", "weight",
  "__empty_lvalue", "__effect_block", "__string_block", "__object_block"
};
constexpr std::string_view tokens_ignore_list[] = {
  "condition", "custom_description", "value", "weight",
};
constexpr std::string_view reserved_operators[] = { "=", "?=", "//", "/*", "*/" };

constexpr bool is_common_english(const char c) {
  return std::any_of(engalpha.begin(), engalpha.end(), [c](const char ac) { return ac == c; });
}

constexpr bool is_special(const char c) {
  return std::any_of(specials.begin(), specials.end(), [c](const char ac) { return ac == c; });
}

constexpr bool is_number_char(const char c) {
  return std::any_of(numbers.begin(), numbers.end(), [c](const char ac) { return ac == c; });
}

constexpr bool is_reserved_word(const std::string_view& str) {
  return std::any_of(std::begin(reserved_tokens), std::end(reserved_tokens), [&str](const std::string_view& reserv) { return reserv == str; });
}

constexpr bool is_reserved_operator(const std::string_view& str) {
  return std::any_of(std::begin(reserved_operators), std::end(reserved_operators), [&str](const std::string_view& reserv) { return reserv == str; });
}

bool is_valid_function_name(const std::string_view& str) noexcept {
  if (str.empty()) return false;
  if (is_number_char(str[0])) return false;
  if (is_reserved_word(str)) return false;

  return std::all_of(str.begin(), str.end(), [](const char c) {
    return is_common_english(c) || is_number_char(c);
  });
}

bool is_valid_operator_name(const std::string_view& str) noexcept {
  if (str.empty()) return false;
  if (is_reserved_word(str)) return false;
  if (is_reserved_operator(str)) return false;
  return std::all_of(str.begin(), str.end(), [](const char c) {
    return is_common_english(c) || is_number_char(c) || is_special(c);
  });
}

bool is_special_operator(const std::string_view& str) noexcept {
  return std::all_of(str.begin(), str.end(), [](const char c) {
    return is_special(c);
  });
}

bool has_special_symbols(const std::string_view& str) noexcept {
  return std::any_of(str.begin(), str.end(), [](const char c) {
    return is_special(c);
  });
}

bool has_common_symbols(const std::string_view& str) noexcept {
  return std::any_of(str.begin(), str.end(), [](const char c) {
    return is_common_english(c) || is_number_char(c);
  });
}

bool is_in_ignore_list(const std::string_view& str) noexcept {
  return std::any_of(std::begin(tokens_ignore_list), std::end(tokens_ignore_list), [&str](const std::string_view& reserv) { return reserv == str; });
}

}

#define EPSILON 0.000001


system::nest_level_changer::nest_level_changer(parse_ctx* ctx) noexcept : ctx(ctx) { ctx->nest_level += 1; }
system::nest_level_changer::~nest_level_changer() noexcept { ctx->nest_level -= 1; }

system::source_position_changer::source_position_changer(parse_ctx* ctx, const size_t line, const size_t column) noexcept :
  ctx(ctx), prev_line(ctx->source_line), prev_column(ctx->source_column) { ctx->source_line = line; ctx->source_column = column; }
system::source_position_changer::~source_position_changer() noexcept { ctx->source_line = prev_line; ctx->source_column = prev_column; }

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

system::description_placeholder::description_placeholder(parse_ctx* ctx) noexcept : ctx(ctx) { ctx->description_placeholder_depth += 1; }
system::description_placeholder::~description_placeholder() noexcept { ctx->description_placeholder_depth -= 1; }



using p_t = prng::xoshiro256starstar;
system::options::options() noexcept : seed(1), safety(safety::safe), optimize(true), error([](const std::string& msg) { throw std::runtime_error(msg); }), warning([](const std::string& msg) { std::cout << "WARN: " << msg << "\n"; }) {}
system::system(const options& opts) noexcept : seed(opts.seed), safet(opts.safety), optimize(opts.optimize), error(opts.error), warning(opts.warning) {
  register_arithmetic_type<script_int_t>("ADD", 10);
  register_arithmetic_type<script_float_t>("ADD", 20);
  register_implicit_conversion<script_int_t, script_float_t>(1);
}

void system::toggle_safety() { this->safet = static_cast<enum safety>(!static_cast<bool>(this->safet)); }
void system::toggle_optimizations() { this->optimize = !this->optimize; }
bool system::optimizations() const { return this->optimize; }
bool system::safety() const { return static_cast<bool>(this->safet); }
[[noreturn]] void system::raise_error(const std::string& msg) const { throw parse_rejected(msg); }
void system::raise_warning(const std::string& msg) const { warning(msg); }
uint64_t system::get_seed() const { return seed; }
void system::reseed(const uint64_t val) { seed = val; }
void system::set_script_resolver(script_resolver_t resolver) { script_resolver = std::move(resolver); }
const script_container* system::resolve_script(const std::string_view& name) const {
  return script_resolver ? script_resolver(name) : nullptr;
}
uint64_t system::parse_context::gen_value() { prng_s = p_t::next(prng_s); return p_t::value(prng_s); }

static std::string_view command_data_ftype_name(const system::command_data::ftype t) noexcept {
  using ftype = system::command_data::ftype;
  switch (t) {
    case ftype::operator_t: return "operator";
    case ftype::function_t: return "function";
    case ftype::invalid: return "invalid";
  }
  return "invalid";
}

static std::string_view command_data_assoc_name(const system::command_data::associativity a) noexcept {
  using associativity = system::command_data::associativity;
  switch (a) {
    case associativity::left: return "left";
    case associativity::right: return "right";
  }
  return "right";
}

static std::string_view description_kind_name(const container::description_node_kind k) noexcept {
  using kind = container::description_node_kind;
  switch (k) {
    case kind::unknown: return "unknown";
    case kind::literal: return "literal";
    case kind::function: return "function";
    case kind::effect: return "effect";
    case kind::operator_t: return "operator";
    case kind::iterator: return "iterator";
    case kind::block: return "block";
    case kind::argument: return "argument";
    case kind::scope: return "scope";
    case kind::control_flow: return "control_flow";
    case kind::conversion: return "conversion";
    case kind::instruction: return "instruction";
  }
  return "unknown";
}

std::string system::dump_registered_functions() const {
  std::vector<const command_data*> entries;
  for (const auto& [name, overloads] : mfuncs) {
    (void)name;
    for (const auto& data : overloads) entries.push_back(&data);
  }

  std::sort(entries.begin(), entries.end(), [](const command_data* lhs, const command_data* rhs) {
    if (lhs->name != rhs->name) return lhs->name < rhs->name;
    if (lhs->expected_scope != rhs->expected_scope) return lhs->expected_scope < rhs->expected_scope;
    if (lhs->type != rhs->type) return lhs->type < rhs->type;
    return lhs->return_type < rhs->return_type;
  });

  std::string out;
  out += std::format("registered functions: {}\n", entries.size());
  for (const command_data* data : entries) {
    out += std::format(
      "{} '{}' scope='{}' returns='{}' args={} priority={} assoc={} kind={} signature='{}'\n",
      command_data_ftype_name(data->type),
      data->name,
      data->expected_scope,
      data->return_type,
      data->arg_count,
      data->priority,
      command_data_assoc_name(data->assoc),
      description_kind_name(data->description_kind),
      data->function_signature
    );
  }

  return out;
}

bool system::is_arithmetic_type(const std::string_view& type) const noexcept {
  return arithmetic_types.find(std::string(type)) != arithmetic_types.end();
}

std::string_view system::arithmetic_block_for(const std::string_view& type) const noexcept {
  const auto itr = arithmetic_types.find(std::string(type));
  if (itr == arithmetic_types.end()) return std::string_view();
  return itr->second.block_name;
}

std::optional<system::conversion_path> system::find_conversion_path(std::string_view from, std::string_view to) const {
  struct node { std::string_view type; int64_t cost; std::vector<const conversion_data*> edges; };
  std::vector<node> open{{from, 0, {}}};
  std::unordered_map<std::string_view, int64_t> best{{from, 0}};
  while (!open.empty()) {
    // Stable minimum selection: equal-cost alternatives keep registration/discovery order.
    auto it = std::min_element(open.begin(), open.end(), [](const node& a, const node& b) { return a.cost < b.cost; });
    node cur = std::move(*it);
    open.erase(it);
    if (cur.cost != best.at(cur.type)) continue;
    if (cur.type == to) {
      if (cur.cost > INT32_MAX) raise_error("Implicit conversion path cost exceeds INT32_MAX");
      return conversion_path{int32_t(cur.cost), std::move(cur.edges)};
    }
    const auto next = implicit_conversions.find(std::string(cur.type));
    if (next == implicit_conversions.end()) continue;
    for (const auto& edge : next->second) {
      // A path already beyond the public cost range need only propagate an overflow sentinel.
      const int64_t cost = std::min(int64_t(INT32_MAX) + 1, cur.cost + edge.cost);
      auto [known, inserted] = best.emplace(edge.to, cost);
      if (!inserted && known->second <= cost) continue;
      known->second = cost;
      auto path = cur.edges;
      path.push_back(&edge);
      open.push_back({edge.to, cost, std::move(path)});
    }
  }
  return std::nullopt;
}

std::optional<int32_t> system::implicit_conversion_cost(const std::string_view& from, const std::string_view& to) const {
  auto path = find_conversion_path(from, to);
  return path ? std::optional<int32_t>(path->cost) : std::nullopt;
}

bool system::can_convert_implicitly(const std::string_view& from, const std::string_view& to) const {
  return find_conversion_path(from, to).has_value();
}

void system::setup_type_conversion(parse_ctx* ctx, container* scr, const std::string_view& from, const std::string_view& to) const {
  if (from == to) return;
  const auto path = find_conversion_path(from, to);
  if (!path) raise_error(std::format("Could not convert from '{}' to '{}'", from, to));
  if (ctx->stack_types.empty() || ctx->stack_types.back() != from)
    raise_error(std::format("Wrong FROM type '{}' - stack last type is '{}'", from, ctx->stack_types.empty() ? std::string_view("empty stack") : ctx->stack_types.back()));
  for (const auto* edge : path->edges) {
    scr->cmds.emplace_back(safety() ? edge->safe : edge->unsafe, INT64_C(0));
    ctx->stack_types.back() = edge->to;
  }
  ctx->conversion_cost += path->cost;
}

const system::command_data* system::resolve_function(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view& name) const {
  const auto itr = mfuncs.find(std::string(name));
  if (itr == mfuncs.end()) raise_error(std::format("Could not find function '{}'", name));

  const auto current_scope = ctx->current_scope_type();
  const auto void_scope = scope_type_name<void>();
  bool has_exact_scope = false;
  for (const auto& data : itr->second) {
    if (data.expected_scope == current_scope) {
      has_exact_scope = true;
      break;
    }
  }

  struct candidate_result {
    const command_data* data = nullptr;
    parse_ctx ctx;
    container scr;
    int64_t cost = 0;
    int64_t rank = 0;
    bool exact_return = false;
    size_t count = 0;
  };

  std::optional<candidate_result> best;
  bool ambiguous = false;
  std::vector<std::string> errors;

  for (const auto& data : itr->second) {
    if (has_exact_scope) {
      if (data.expected_scope != current_scope) continue;
    } else if (data.expected_scope != void_scope) {
      continue;
    }

    int64_t return_conversion_cost = 0;
    const bool boolean_block = data.name == "AND" || data.name == "OR" || data.name == "NAND" || data.name == "NOR";
    const bool return_is_scalar = !boolean_block && (type_is_bool(data.return_type) || type_is_fundamental(data.return_type) || is_arithmetic_type(data.return_type) || type_is_string(data.return_type));
    if (return_is_scalar && !type_is_void(ctx->expected_type) && !type_is_any_type(ctx->expected_type) && !type_is_element_view(ctx->expected_type) && data.return_type != ctx->expected_type) {
      const auto ret_cost = implicit_conversion_cost(data.return_type, ctx->expected_type);
      if (!ret_cost.has_value()) {
        errors.push_back(std::format(
          "candidate '{}' returns '{}', which cannot be converted to expected type '{}'",
          data.function_signature, data.return_type, ctx->expected_type
        ));
        continue;
      }
      return_conversion_cost = *ret_cost;
    }

    parse_ctx test_ctx = *ctx;
    container test_scr = *scr;
    test_ctx.conversion_cost = 0;
    emitter e{ this, &test_ctx, &test_scr };

    try {
      const size_t count = std::invoke(data.init, e, block);
      const int64_t cost = test_ctx.conversion_cost + return_conversion_cost;
      int64_t rank = 0;
      for (const auto arg_type : data.argument_types) {
        const auto arith = arithmetic_types.find(std::string(arg_type));
        rank += arith == arithmetic_types.end() ? 1000 : arith->second.priority;
      }
      // `rank` prefers the narrower arithmetic type on a tie, which is the right default when every
      // operand's type is known. It is the wrong one as soon as an operand is dynamically typed
      // (`any_stack` out of value_or, a list element, a script argument): picking the narrow overload
      // there does not just choose an implementation, it asserts a runtime type nothing established.
      // So a candidate that already returns what this block expects wins the tie first - it needs no
      // conversion and imposes nothing.
      const bool exact_return = !type_is_void(ctx->expected_type) && data.return_type == ctx->expected_type;
      const auto better = [&] {
        if (cost != best->cost) return cost < best->cost;
        if (exact_return != best->exact_return) return exact_return;
        return rank < best->rank;
      };
      if (!best.has_value() || better()) {
        best = candidate_result{ &data, std::move(test_ctx), std::move(test_scr), cost, rank, exact_return, count };
        ambiguous = false;
      } else if (cost == best->cost && exact_return == best->exact_return && rank == best->rank) {
        ambiguous = true;
      }
    } catch (const std::exception& ex) {
      errors.push_back(std::format("candidate '{}': {}", data.function_signature, ex.what()));
    }
  }

  if (!best.has_value()) {
    if (errors.empty()) raise_error(std::format("Could not find function '{}' for scope type '{}'", name, current_scope));
    std::string reasons;
    for (const auto& reason : errors) {
      if (!reasons.empty()) reasons += "; ";
      reasons += reason;
    }
    raise_error(std::format("Could not resolve function '{}' for scope type '{}': {}", name, current_scope, reasons));
  }

  if (ambiguous) {
    raise_error(std::format("Ambiguous overload for function '{}' in scope type '{}'", name, current_scope));
  }

  // Active command blocks and token views refer to the caller's parser storage.
  // Preserve its buffers when committing a candidate; copying them would free the
  // original token allocation below and leave those views dangling.
  best->ctx.rpn_ctx = std::move(ctx->rpn_ctx);
  best->ctx.script_ast_ctx = std::move(ctx->script_ast_ctx);
  best->ctx.script_ast_nodes = std::move(ctx->script_ast_nodes);
  *ctx = std::move(best->ctx);
  *scr = std::move(best->scr);
  return best->data;
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

  auto itr = mfuncs.find(data.name);
  if (itr == mfuncs.end()) {
    itr = mfuncs.emplace(std::make_pair(data.name, std::vector<command_data>{})).first;
  } else {
    if (itr->second.empty()) raise_error(std::format("'{}' empty?", data.name));
    if (data.type != itr->second.front().type) 
      raise_error(std::format("Cannot register several functions '{}' with defferent types", data.name));
    if (data.type == command_data::ftype::operator_t) {
      const auto& another = itr->second.front();
      const bool invalid_state = 
        data.priority != another.priority || 
        data.arg_count != another.arg_count ||
        data.assoc != another.assoc;

      if (invalid_state) 
        raise_error(std::format("Operators props must be the same for all overloaded operators ({},{},{}) != ({},{},{})", data.priority, data.arg_count, static_cast<uint32_t>(data.assoc), another.priority, another.arg_count, static_cast<uint32_t>(another.assoc)));
    }
  }

  for (const auto& overload : itr->second) {
    if (overload.expected_scope == data.expected_scope && overload.argument_types == data.argument_types) {
      raise_error(std::format("'{}' is already registered for scope '{}' and signature '{}'", data.name, data.expected_scope, data.function_signature));
    }
  }

  data.builtin = registering_builtins;
  itr->second.push_back(std::move(data));
}

bool system::is_builtin_function(const std::string_view& name) const {
  const auto itr = mfuncs.find(std::string(name));
  if (itr == mfuncs.end()) return false;
  for (const auto& overload : itr->second) {
    if (!overload.builtin) return false;
  }
  return true;
}

void system::reserve_from_hint(container* scr, const size_t block_count, const size_t token_bytes) const {
  // ~one description node per block; cmds/locs run a bit higher (combinator folds, scope-chain
  // erases, type conversions emit extra opcodes), so reserve ~1.5x. string_pool grows by at most
  // the token-text bytes (the pool is deduplicated, so this is an upper bound).
  scr->block_descs.reserve(scr->block_descs.size() + block_count);
  const size_t cmd_hint = scr->cmds.size() + block_count + block_count / 2;
  scr->cmds.reserve(cmd_hint);
  scr->locs.reserve(cmd_hint);
  scr->string_pool.reserve(scr->string_pool.size() + token_bytes);
}

void system::setup_block_description(
  parse_ctx* ctx,
  container* scr,
  const std::string_view& token,
  const std::string_view& custom_desc,
  const size_t start,
  const size_t initial_cmd_start,
  const container::description_node_kind explicit_kind
) const {
  using sv_t = script_container::string_ref;
  sv_t tok{};
  sv_t cd{};
  auto kind = explicit_kind;
  bool effect = false;   // set for void functions/iterators via the registry lookup below

  if (token == "__empty_lvalue") {
    tok = { SIZE_MAX, SIZE_MAX };
  } else {
    basicf id = basicf::invalid;
    if (token == "__object_block") id = basicf::object_block;
    else if (token == "__string_block") id = basicf::string_block;
    else if (token == "__effect_block") id = basicf::effect_block;
    else id = find_basicf(token);
    if (id == basicf::invalid) tok = store_string(scr, token);
    else tok = { static_cast<size_t>(id), SIZE_MAX };
  }

  if (custom_desc.empty()) {
    cd = { SIZE_MAX, SIZE_MAX };
  } else {
    const basicf id = find_basicf(custom_desc);
    if (id == basicf::invalid) cd = store_string(scr, custom_desc);
    else cd = { static_cast<size_t>(id), SIZE_MAX };
  }

  const int64_t scope_index = ctx->scope_stack.empty() ? -1 : ctx->scope_stack.back();
  const size_t size = scr->block_descs.size() - start + 1;
  const size_t index = scr->cmds.size()-1;
  size_t cmd_start = initial_cmd_start == SIZE_MAX ? index : initial_cmd_start;
  if (kind == container::description_node_kind::unknown) {
    script_float_t number = 0;
    if (text::is_bool(token) || text::is_number(token, number)) {
      kind = container::description_node_kind::literal;
    } else if (token == "condition" || token == "value" || token == "weight") {
      kind = container::description_node_kind::argument;
    } else if (token == "select" || token == "sequence" || token == "switch" || token == "random" || token == "value_or") {
      kind = container::description_node_kind::control_flow;
    } else if (
      token == "AND" || token == "OR" || token == "NAND" || token == "NOR" ||
      token == "__effect_block" || token == "__string_block" || token == "__object_block" ||
      token == to_string(basicf::AND) || token == to_string(basicf::OR) || token == to_string(basicf::ADD) ||
      token == to_string(basicf::MUL) || token == to_string(basicf::effect_block) ||
      token == to_string(basicf::string_block) || token == to_string(basicf::object_block)
    ) {
      kind = container::description_node_kind::block;
    } else {
      const auto func = mfuncs.find(std::string(token));
      if (func != mfuncs.end()) {
        const command_data* found = nullptr;
        for (const auto& data : func->second) {
          if (data.expected_scope == ctx->current_scope_type()) { found = &data; break; }
          if (found == nullptr && data.expected_scope == scope_type_name<void>()) found = &data;
        }
        if (found != nullptr) {
          kind = found->description_kind;
          effect = type_is_void(found->return_type);
        }
      } else if (!token.empty()) {
        kind = container::description_node_kind::literal;
      }
    }
  }

  scr->block_descs.push_back({
    tok, cd, size, 0, index, cmd_start, scr->cmds.size(), scope_index, ctx->description_placeholder_depth > 0, effect, kind
  });

  const size_t current_index = scr->block_descs.size() - 1;
  size_t offset = 1;
  size_t counter = 0;
  while (offset < size) {
    const auto& desc = scr->block_descs[current_index - offset];
    cmd_start = std::min(cmd_start, desc.cmd_start);
    offset += desc.size;

    counter += 1;
  }

  scr->block_descs[current_index].args_count = counter;
  scr->block_descs[current_index].cmd_start = cmd_start;

  // Stamp source positions for any commands emitted by this node that are not yet covered.
  // Children describe themselves first, so this fills only the node's own trailing commands;
  // build_description_index pads any stragglers (e.g. the final pushreturn) afterwards.
  const script_container::src_loc loc{ static_cast<uint32_t>(ctx->source_line), static_cast<uint32_t>(ctx->source_column) };
  if (scr->locs.size() < scr->cmds.size()) scr->locs.resize(scr->cmds.size(), loc);
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
  bool      has_return = false;  // currently unused (was a command_description flag; left for the table's completeness)
  uint8_t   arg_count = 0;       // currently unused (was a command_description flag)
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
  row(basicf::mul,           &mul,             &mul_unsafe,            2,    pk::b_double,    true,  2);
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
  row(basicf::erase_range,   &erase_range,     &erase_range,           0,    pk::none,        false, 0);
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

basicf find_basicf_by_fp(function_t fp) noexcept {
  if (fp == nullptr) return basicf::invalid;
  for (size_t k = 0; k < basicf_count; ++k) {
    if (insn_table[k].safe == fp || insn_table[k].unsafe == fp) return static_cast<basicf>(k);
  }
  // `jumpinvalid` is emitted for nullable object-block children but labelled as a condjump.
  if (fp == &jumpinvalid) return basicf::condjump;
  return basicf::invalid;
}

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

  return scr->cmds.size()-1;
}

size_t system::emitter::emit(const basicf op, const int64_t arg) const { return sys->push_basic_function(ctx, scr, op, arg); }
size_t system::emitter::emit_string(const std::string_view& str) const { return sys->push_string(ctx, scr, str); }
void system::emitter::warn(std::string msg) const { ctx->warnings.push_back(std::move(msg)); }
system::emitter::label system::emitter::make_label() const { return label{}; }
void system::emitter::jump_to(const basicf op, label& l) const { l.sites.push_back(emit(op, 0)); }
void system::emitter::mark(label& l, const size_t cmd_index) const { l.sites.push_back(cmd_index); }
void system::emitter::bind(label& l) const {
  const size_t target = scr->cmds.size();
  for (const size_t site : l.sites) {
    scr->cmds[site].arg = target;
    sys->record_cmd_index(ctx, site);
  }
}

size_t system::push_string(parse_ctx* ctx, container* scr, const std::string_view& str) const {
  const auto stored = store_string(scr, str);
  push_basic_function(ctx, scr, basicf::pushstring, packstrid(uint32_t(stored.start), uint32_t(stored.count)));
  return 1;
}

auto system::store_string(container* scr, const std::string_view& str) const -> script_container::string_ref {
  using sv_t = script_container::string_ref;
  if (str.empty()) return sv_t{ 0, 0 };

  // The container keeps a single contiguous, deduplicated string pool (`scr->string_pool`). Reuse
  // an existing byte range when the text already appears in the pool, otherwise append it; the
  // result is always an (offset, size) reference into the pool.
  // NOTE: dedup is a linear `find` per call (O(pool * str)); fine for typical scripts, but a
  // hash index keyed by text -> offset would cut it to O(str) if it ever shows up in profiles.
  if (!check_value(str.size(), packed_size_bit_size))
    raise_error(std::format("String '{}' size in script string cannot be packed in {} bits", str, packed_size_bit_size));

  size_t pos = scr->string_pool.find(str.data(), 0, str.size());
  if (pos == std::string::npos) {
    pos = scr->string_pool.size();
    if (!check_value(pos, packed_pos_bit_size)) raise_error(std::format("String '{}' position in script string pool cannot be packed in {} bits", str, packed_pos_bit_size));
    scr->string_pool.append(str);
  } else if (!check_value(pos, packed_pos_bit_size)) {
    raise_error(std::format("String '{}' position in script string pool cannot be packed in {} bits", str, packed_pos_bit_size));
  }
  return sv_t{ pos, str.size() };
}

void system::compact_source_storage(container* scr) const {
  if (scr == nullptr) return;

  // `store_string` builds `scr->string_pool` as a deduplicated, append-only pool, so every
  // reference already points at the minimal set of live bytes — there is nothing to intern away.
  // Just release the spare capacity left by reservation/append growth.
  scr->string_pool.shrink_to_fit();
}

std::string_view system::static_string_arg(const command_block& block, const std::string_view& name) const {
  if (block.empty()) return std::string_view();
  if (block.braced_args()) {
    raise_error(std::format("'{}' expects a static string token, not a script block", name));
  }
  if (block.args_count() == 0 && block.size() == 1 && !block.nullable()) return block.name();
  if (block.args_count() != 1 || block.size() != 2) {
    raise_error(std::format("'{}' expects exactly one static string token", name));
  }

  const auto arg = command_block(block, 1);
  if (arg.empty() || arg.args_count() != 0 || arg.size() != 1 || arg.nullable()) {
    raise_error(std::format("'{}' expects a static string token, not a script block", name));
  }

  return arg.name();
}

std::optional<int64_t> system::resolve_enum(const std::string_view& enum_type, const std::string_view& value) const {
  const auto itr = enums.find(std::string(enum_type));
  if (itr == enums.end()) return std::nullopt;
  return std::invoke(itr->second, value);
}

std::optional<int64_t> system::resolve_enum(const std::string_view& value) const {
  std::optional<int64_t> result;
  for (const auto& [_, fn] : enums) {
    const auto val = std::invoke(fn, value);
    if (!val.has_value()) continue;
    if (result.has_value() && *result != *val) raise_error(std::format("Enum literal '{}' is ambiguous", value));
    result = val;
  }
  return result;
}

size_t system::push_enum_literal(parse_ctx* ctx, container* scr, const std::string_view& enum_type, const std::string_view& value) const {
  const auto val = enum_type.empty() ? resolve_enum(value) : resolve_enum(enum_type, value);
  if (!val.has_value()) {
    if (enum_type.empty()) raise_error(std::format("Could not find enum value '{}'", value));
    raise_error(std::format("Could not find value '{}' in registered enum type '{}'", value, enum_type));
  }

  push_basic_function(ctx, scr, basicf::pushint, *val);
  return 1;
}

namespace {

// A folded compile-time constant. The three kinds are kept strictly apart: the emitted command
// stream has no implicit conversion of its own, so the folder must not invent one either. Anything
// that would need a conversion the compiler did not choose, or an overload the folder cannot see, is
// left to normal codegen.
struct const_value {
  enum class kind { boolean, integer, number };
  kind type;
  bool b = false;
  script_int_t i = 0;
  script_float_t n = 0;

  static const_value boolean(const bool v) noexcept { return { kind::boolean, v, 0, 0 }; }
  static const_value integer(const script_int_t v) noexcept { return { kind::integer, false, v, 0 }; }
  static const_value number(const script_float_t v) noexcept { return { kind::number, false, 0, v }; }

  bool is_bool() const noexcept { return type == kind::boolean; }
  bool is_integer() const noexcept { return type == kind::integer; }
  bool is_number() const noexcept { return type == kind::number; }
  bool is_numeric() const noexcept { return type != kind::boolean; }
  // Mixed integer/floating operands make the compiler convert the integer side, so the folder reads
  // the same value the emitted `convert<int64_t, double>` would have produced.
  script_float_t as_float() const noexcept { return type == kind::integer ? script_float_t(i) : n; }
};

// Integer arithmetic wraps, matching rawaddi/rawsubi/rawmuli at runtime. Going through the unsigned
// type keeps the fold itself free of the undefined behaviour it is modelling.
using script_uint_t = std::make_unsigned_t<script_int_t>;
script_int_t wrap_add(const script_int_t a, const script_int_t b) noexcept { return script_int_t(script_uint_t(a) + script_uint_t(b)); }
script_int_t wrap_sub(const script_int_t a, const script_int_t b) noexcept { return script_int_t(script_uint_t(a) - script_uint_t(b)); }
script_int_t wrap_mul(const script_int_t a, const script_int_t b) noexcept { return script_int_t(script_uint_t(a) * script_uint_t(b)); }

// Evaluates a fully constant sub-expression, or nothing when it cannot be folded without changing
// what the script would have done. Folding happens before overload resolution, so it is restricted
// to built-in operators over literal bools, integers and doubles; every other case falls through.
std::optional<const_value> try_eval_const(const system& sys, const system::command_block& block) {
  if (block.empty()) return std::nullopt;

  const auto name = block.name();
  if (block.args_count() == 0 && block.size() == 1) {
    if (block.string_literal()) return std::nullopt;
    if (text::is_bool(name)) return const_value::boolean(text::as_bool(name));
    // Same order as the literal lowering in dispatch_node: written without a fraction or an
    // exponent means integer, and only then is it read as a double.
    if (script_int_t v = 0; text::is_integer(name, v)) return const_value::integer(v);
    if (script_float_t v = 0; text::is_number(name, v)) return const_value::number(v);
    return std::nullopt;
  }

  // A consumer overload registered under this name may compute something else entirely, or have
  // effects. Only names owned exclusively by init_math()/init_basic_functions() are foldable.
  if (!sys.is_builtin_function(name)) return std::nullopt;

  std::vector<const_value> args;
  args.reserve(block.args_count());
  for (const auto& child : block.children()) {
    auto v = try_eval_const(sys, child);
    if (!v) return std::nullopt;
    args.push_back(*v);
  }
  if (args.empty()) return std::nullopt;

  const bool all_numeric = std::all_of(args.begin(), args.end(), [](const const_value& v) { return v.is_numeric(); });
  const bool all_integer = std::all_of(args.begin(), args.end(), [](const const_value& v) { return v.is_integer(); });
  const bool all_bool = std::all_of(args.begin(), args.end(), [](const const_value& v) { return v.is_bool(); });

  // n-ary arithmetic blocks. The accumulator starts at the first operand rather than at 0/1 so the
  // fold associates exactly like the emitted chain of ADD/MUL opcodes - a leading identity would
  // turn `ADD = { -0.0 }` into +0.0.
  if (name == "ADD" || name == "MUL") {
    if (!all_numeric) return std::nullopt;
    const bool product = name == "MUL";
    if (all_integer) {
      script_int_t r = args[0].i;
      for (size_t k = 1; k < args.size(); ++k) r = product ? wrap_mul(r, args[k].i) : wrap_add(r, args[k].i);
      return const_value::integer(r);
    }
    script_float_t r = args[0].as_float();
    for (size_t k = 1; k < args.size(); ++k) r = product ? r * args[k].as_float() : r + args[k].as_float();
    return const_value::number(r);
  }

  if (name == "AND" || name == "NAND" || name == "OR" || name == "NOR") {
    if (!all_bool) return std::nullopt;
    const bool disjunction = name == "OR" || name == "NOR";
    const bool negated = name == "NAND" || name == "NOR";
    bool r = args[0].b;
    for (size_t k = 1; k < args.size(); ++k) r = disjunction ? (r || args[k].b) : (r && args[k].b);
    return const_value::boolean(negated ? !r : r);
  }

  if (args.size() == 1) {
    if (args[0].is_integer()) {
      if (name == "unary_plus") return const_value::integer(args[0].i);
      if (name == "unary_minus") return const_value::integer(wrap_sub(0, args[0].i));
    }
    if (args[0].is_number()) {
      if (name == "unary_plus") return const_value::number(+args[0].n);
      if (name == "unary_minus") return const_value::number(-args[0].n);
    }
    if (args[0].is_bool() && name == "not") return const_value::boolean(!args[0].b);
    return std::nullopt;
  }

  if (args.size() != 2) return std::nullopt;

  if (all_integer) {
    const auto l = args[0].i;
    const auto r = args[1].i;
    if (name == "+") return const_value::integer(wrap_add(l, r));
    if (name == "-") return const_value::integer(wrap_sub(l, r));
    if (name == "*") return const_value::integer(wrap_mul(l, r));
    // Both cases raise at runtime, so folding them would turn a script error into a compile error.
    if (name == "%") return r == 0 ? std::nullopt : std::optional(const_value::integer(r == -1 ? 0 : l % r));
    if (name == ">") return const_value::boolean(l > r);
    if (name == "<") return const_value::boolean(l < r);
    if (name == ">=") return const_value::boolean(l >= r);
    if (name == "<=") return const_value::boolean(l <= r);
    if (name == "==") return const_value::boolean(l == r);
    if (name == "!=") return const_value::boolean(l != r);
    // `/` has no integer overload: fall through to the floating-point rules below.
    if (name != "/") return std::nullopt;
  }

  if (all_numeric) {
    const auto l = args[0].as_float();
    const auto r = args[1].as_float();
    if (name == "+") return const_value::number(l + r);
    if (name == "-") return const_value::number(l - r);
    if (name == "*") return const_value::number(l * r);
    if (name == "/") return const_value::number(l / r);
    if (name == "%") return const_value::number(std::fmod(l, r));
    if (name == ">") return const_value::boolean(l > r);
    if (name == "<") return const_value::boolean(l < r);
    if (name == ">=") return const_value::boolean(l >= r);
    if (name == "<=") return const_value::boolean(l <= r);
    // Matches the runtime double comparison (raweqd), which is tolerance-based.
    if (name == "==") return const_value::boolean(std::abs(l - r) < EPSILON);
    if (name == "!=") return const_value::boolean(std::abs(l - r) >= EPSILON);
    return std::nullopt;
  }

  if (all_bool) {
    if (name == "and") return const_value::boolean(args[0].b && args[1].b);
    if (name == "or") return const_value::boolean(args[0].b || args[1].b);
    if (name == "==") return const_value::boolean(args[0].b == args[1].b);
    if (name == "!=") return const_value::boolean(args[0].b != args[1].b);
  }

  return std::nullopt;
}

}  // namespace

size_t system::dispatch_node(parse_ctx* ctx, container* scr, const command_block& block, const std::string_view& override_lvalue) const {
  if (block.empty()) return 0;

  source_position_changer spc(ctx, block.line(), block.column());

  const auto exp_t = ctx->expected_type;
  const bool any_type_expected = type_is_any_type(exp_t);

  if (override_lvalue.empty()) {
    if (auto v = try_eval_const(*this, block)) {
      // The constant stands in for a call whose result type the caller already checked, so it may
      // only be emitted where a value of its own kind is what codegen would have produced. An
      // integer in a floating-point block is the one crossing allowed, because there codegen would
      // have emitted `pushint` followed by `convert<int64_t, double>` - the same static_cast.
      const bool as_float = v->is_integer() && !any_type_expected && type_is_floating_point(exp_t);
      const bool fits =
        v->is_bool()    ? any_type_expected || type_is_bool(exp_t) :
        v->is_integer() ? any_type_expected || type_is_integral(exp_t) || as_float :
                          any_type_expected || type_is_floating_point(exp_t);
      if (fits) {
        set_function_type sft(ctx, function_type::rvalue);
        if (v->is_bool()) push_basic_function(ctx, scr, basicf::pushbool, v->b);
        else if (v->is_integer() && !as_float) push_basic_function(ctx, scr, basicf::pushint, v->i);
        else push_basic_function(ctx, scr, basicf::pushvalue, pack_float(as_float ? script_float_t(v->i) : v->n));
        setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
        return block.size();
      }
    }
  }
  
  auto funcname = block.name();
  if (block.args_count() == 0 && block.size() == 1) { // rvalue
    if (funcname == "condition") raise_error(std::format("'condition' is not allowed here"));

    if (block.string_literal()) {
      set_function_type sft(ctx, function_type::rvalue);
      push_string(ctx, scr, block.name());
      setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
      return 1;
    }

    if (text::is_bool(funcname)) {
      set_function_type sft(ctx, function_type::rvalue);
      push_basic_function(ctx, scr, basicf::pushbool, text::as_bool(funcname));
      setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
      return 1;
    }

    // A literal written without a fraction or an exponent is an integer, whatever the surrounding
    // block happens to expect. Overload resolution and the registered int64 -> double conversion
    // take it from there. Reading it as a double because the context is floating-point is what used
    // to make `9007199254740992 == 9007199254740993` true.
    if (script_int_t val; text::is_integer(funcname, val)) {
      set_function_type sft(ctx, function_type::rvalue);
      push_basic_function(ctx, scr, basicf::pushint, val);
      setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
      return 1;
    }

    if (script_float_t val; text::is_number(funcname, val)) {
      set_function_type sft(ctx, function_type::rvalue);
      push_basic_function(ctx, scr, basicf::pushvalue, pack_float(val));
      setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
      return 1;
    }

    {
      set_function_type sft(ctx, function_type::lvalue);

      std::array<rpn_conversion_ctx::block, 16 * 3+1> arr;
      auto [local_fname, count] = ctx->rpn_ctx.convert_scope(funcname, arr.data(), arr.size(), ctx->source_line, ctx->source_column);
      funcname = ctx->rpn_ctx.token_text(local_fname);

      if (count == 0) {
        const auto& itr = mfuncs.find(std::string(block.name()));
        if (
          itr == mfuncs.end() &&
          !type_is_string(exp_t) &&
          (type_is_fundamental(exp_t) || any_type_expected) &&
          resolve_enum(block.name()).has_value()
        ) {
          set_function_type sft(ctx, function_type::rvalue);
          push_enum_literal(ctx, scr, std::string_view(), block.name());
          setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
          return 1;
        } else if (itr == mfuncs.end() && exp_t == utils::type_name<std::string_view>()) {
          set_function_type sft(ctx, function_type::rvalue);
          push_string(ctx, scr, block.name());
          setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
          return 1;
        } else if (itr == mfuncs.end() && any_type_expected) {
          set_function_type sft(ctx, function_type::rvalue);
          push_string(ctx, scr, block.name());
          setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size());
          return 1;
        } else if (itr == mfuncs.end()) raise_error(std::format("Could not find function '{}'", block.name()));

        set_function_type sft(ctx, function_type::rvalue);
        function_name_changer fnc(ctx, block.name());

        const size_t cmd_start = scr->cmds.size();
        const command_data* data = resolve_function(ctx, scr, block, block.name());
        (void)data;
        const size_t count = block.size();
        setup_block_description(ctx, scr, block.name(), std::string_view(), scr->block_descs.size(), cmd_start);
        return count;
      }

      if (local_fname.offset != SIZE_MAX) {
        for (size_t i = 0; i < count; i += rpn_block_direct_child_count(arr.data() + i, count - i) + 1) {
          arr[i].size += 1;
        }

        arr[count] = rpn_conversion_ctx::block{ local_fname, 1 };
        count += 1;
      }
      
      bool has_invalid_func_name = false;
      command_block cb(std::span(arr.data(), count), &ctx->rpn_ctx.token_storage);
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

  const bool is_not_overriden = override_lvalue.empty();
  const bool is_condition = block.name() == "condition";
  const bool is_subblock = funcname == "__empty_lvalue";
  if (is_subblock) {
    auto curid = basicf::object_block;
    if (type_is_bool(exp_t)) curid = basicf::AND;
    else if (type_is_fundamental(exp_t) || is_arithmetic_type(exp_t)) curid = basicf::ADD;
    else if (type_is_string(exp_t)) curid = basicf::string_block;
    else if (type_is_void(exp_t)) curid = basicf::effect_block;

    const size_t desc_start = scr->block_descs.size();
    const size_t cmd_start = scr->cmds.size();
    const size_t count = fold_block(ctx, scr, block, curid);
    const auto cd = block.find(custom_description_constant);
    setup_block_description(ctx, scr, to_string(curid), static_string_arg(cd, custom_description_constant), desc_start, cmd_start);
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
    const size_t cmd_start = scr->cmds.size();
    const size_t count = fold_block(ctx, scr, block, curid);
    const auto cd = block.find(custom_description_constant);
    setup_block_description(ctx, scr, funcname, static_string_arg(cd, custom_description_constant), desc_start, cmd_start);
    return count;
  }

  const size_t desc_start = scr->block_descs.size();
  const size_t cmd_start = scr->cmds.size();
  const std::string desc_name(prevname);
  const auto cd_before_resolve = block.find(custom_description_constant);
  const std::string custom_desc_before_resolve(static_string_arg(cd_before_resolve, custom_description_constant));

  function_name_changer fnc(ctx, prevname);
  set_function_type sft(ctx, function_type::lvalue);
  resolve_function(ctx, scr, block, funcname);

  if (is_subblock || is_condition || is_not_overriden) {
    setup_block_description(ctx, scr, desc_name, custom_desc_before_resolve, desc_start, cmd_start);
  }

  return block.size();
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
    else if (type_is_fundamental(exp_t) || is_arithmetic_type(exp_t)) curid = basicf::ADD;
    else if (type_is_string(exp_t)) curid = basicf::string_block;
    else curid = basicf::object_block;
  }

  if (curid == basicf::object_block) {
    emitter e{this, ctx, scr};
    auto end = e.make_label();
    const auto exp_t = ctx->expected_type;
    const bool any_object_expected = type_is_any_type(exp_t) || type_is_any_type_object(exp_t) || type_is_element_view(exp_t);
    std::string_view result_type = any_object_expected ? std::string_view() : exp_t;
    bool has_value = false;

    size_t offset = 1;
    while (offset < block.size()) {
      const auto child = command_block(block, offset);
      offset += child.size();
      if (text::is_in_ignore_list(child.name())) continue;

      auto skip_child = e.make_label();
      const auto cond_block = child.find("condition");
      const bool has_condition = !cond_block.empty();
      if (has_condition) {
        dispatch_node(ctx, scr, cond_block, "AND");
        if (!ctx->is<bool>()) raise_error(std::format("Object block condition '{}' must return bool, got '{}'", cond_block.name(), ctx->top()));
        e.jump_to(basicf::condjump, skip_child);
      }

      const size_t before = ctx->stack_types.size();
      dispatch_node(ctx, scr, child);
      if (before >= ctx->stack_types.size()) raise_error(std::format("Object block child '{}' does not push any value", child.name()));
      if (ctx->is<ignore_value>()) raise_error(std::format("Object block child '{}' returns 'ignore_value'", child.name()));

      const auto branch_type = ctx->top();
      if (!any_object_expected && branch_type != exp_t)
        raise_error(std::format("Object block expects '{}', but child '{}' returns '{}'", exp_t, child.name(), branch_type));
      if (any_object_expected && result_type.empty()) result_type = branch_type;
      ctx->pop();
      has_value = true;

      if (child.nullable()) {
        scr->cmds.push_back(container::command(&jumpinvalid, INT64_C(0)));
        e.mark(skip_child, scr->cmds.size() - 1);
      }

      e.jump_to(basicf::jump, end);
      if (has_condition || child.nullable()) e.bind(skip_child);
    }

    e.bind(end);
    if (has_value) ctx->push(result_type.empty() ? exp_t : result_type);
    else ctx->push<ignore_value>();

    return block.size();
  }

  if (curid == basicf::string_block) {
    emitter e{this, ctx, scr};
    auto end = e.make_label();
    bool has_value = false;

    size_t offset = 1;
    while (offset < block.size()) {
      const auto child = command_block(block, offset);
      offset += child.size();
      if (text::is_in_ignore_list(child.name())) continue;

      auto skip_child = e.make_label();
      const auto cond_block = child.find("condition");
      const bool has_condition = !cond_block.empty();
      if (has_condition) {
        dispatch_node(ctx, scr, cond_block, "AND");
        if (!ctx->is<bool>()) raise_error(std::format("String block condition '{}' must return bool, got '{}'", cond_block.name(), ctx->top()));
        e.jump_to(basicf::condjump, skip_child);
      }

      const size_t before = ctx->stack_types.size();
      dispatch_node(ctx, scr, child);
      if (before >= ctx->stack_types.size()) raise_error(std::format("String block child '{}' does not push any value", child.name()));
      if (ctx->is<ignore_value>()) { ctx->pop(); continue; }
      if (!ctx->is<std::string_view>()) raise_error(std::format("String block expects '{}', but child '{}' returns '{}'", utils::type_name<std::string_view>(), child.name(), ctx->top()));
      ctx->pop();
      has_value = true;

      if (child.nullable()) {
        scr->cmds.push_back(container::command(&jumpinvalid, INT64_C(0)));
        e.mark(skip_child, scr->cmds.size() - 1);
      }

      e.jump_to(basicf::jump, end);
      if (has_condition || child.nullable()) e.bind(skip_child);
    }

    e.bind(end);
    if (has_value) ctx->push<std::string_view>();
    else ctx->push<ignore_value>();
    return block.size();
  }

  bool boolean_and_block = false;
  bool boolean_or_block = false;
  basicf opcode = basicf::invalid; // the actual fold opcode emitted per clause; resolved from the combinator id
  switch (curid) {
    case basicf::ADD:  opcode = basicf::sum;     break;
    case basicf::MUL:  opcode = basicf::mul;     break;
    case basicf::AND:  opcode = basicf::andjump; boolean_and_block = true; break;
    case basicf::OR:   opcode = basicf::orjump;  boolean_or_block  = true; break;
    case basicf::NAND: opcode = basicf::andjump; boolean_and_block = true; break;
    case basicf::NOR:  opcode = basicf::orjump;  boolean_or_block  = true; break;

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
  bool conditional_arithmetic = false;
  if (curid == basicf::ADD || curid == basicf::MUL) {
    size_t scan_offset = 1;
    while (scan_offset < block.size() && !conditional_arithmetic) {
      const auto child = command_block(block, scan_offset);
      scan_offset += child.size();
      conditional_arithmetic = !text::is_in_ignore_list(child.name()) && !child.find("condition").empty();
    }
  }

  if (conditional_arithmetic) {
    const auto exp_t = ctx->expected_type;
    const int64_t identity = curid == basicf::MUL ? INT64_C(1) : INT64_C(0);
    if (type_is_integral(exp_t)) {
      push_basic_function(ctx, scr, basicf::pushint, identity);
    } else {
      push_basic_function(ctx, scr, basicf::pushvalue, std::bit_cast<int64_t>(double(identity)));
      if (!type_is_floating_point(exp_t) && can_convert_implicitly(ctx->top(), exp_t)) {
        setup_type_conversion(ctx, scr, ctx->top(), exp_t);
      }
    }
  }

  size_t counter = conditional_arithmetic ? 1 : 0;
  size_t offset = 1;
  while (offset < block.size()) {
    const auto child = command_block(block, offset);
    offset += child.size();
    auto skip_child = e.make_label();
    bool skip_child_used = false;

    if (curid == basicf::effect_block || curid == basicf::string_subblock || curid == basicf::object_subblock) {
      if (const auto cond_block = child.find("condition"); !cond_block.empty()) {
        dispatch_node(ctx, scr, cond_block, "AND");
        if (current_stack_size >= ctx->stack_types.size()) raise_error(std::format("Block '{}' does not push any value", cond_block.name()));
        e.jump_to(basicf::condjump, end);
      }
    }

    if (text::is_in_ignore_list(child.name())) continue;

    if (conditional_arithmetic) {
      if (const auto cond_block = child.find("condition"); !cond_block.empty()) {
        dispatch_node(ctx, scr, cond_block, "AND");
        if (!ctx->is<bool>()) raise_error(std::format("Arithmetic block condition '{}' must return bool, got '{}'", cond_block.name(), ctx->top()));
        e.jump_to(basicf::condjump, skip_child);
        skip_child_used = true;
      }
    }

    const size_t before_child = ctx->stack_types.size();
    dispatch_node(ctx, scr, child);
    if (curid == basicf::effect_block) continue;
    if (before_child >= ctx->stack_types.size()) raise_error(std::format("Block '{}' does not push any value", child.name()));
    if (ctx->is<ignore_value>()) {
      ctx->pop();
      if (skip_child_used) e.bind(skip_child);
      continue;
    }
    if (child.nullable() && (curid == basicf::string_block || curid == basicf::string_subblock || curid == basicf::object_subblock)) {
      auto skip_invalid = e.make_label();
      scr->cmds.push_back(container::command(&jumpinvalid, INT64_C(0)));
      e.mark(skip_invalid, scr->cmds.size() - 1);
      e.bind(skip_invalid);
    }
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

    // The fold opcode is a regular insn_table instruction: emit() is the single source of its
    // stack effect (pops 2, pushes bool/double) + description + the cmds/descs consistency check.
    const size_t op_index = e.emit(opcode, 0);
    if (boolean_and_block || boolean_or_block) e.mark(end, op_index); // andjump / orjump is itself a short-circuit site
    if (skip_child_used) e.bind(skip_child);
  }

  e.bind(end);
  if (current_stack_size == ctx->stack_types.size()) ctx->push<ignore_value>(); // no value

  return block.size();
}

void system::configure_parser(tavl::parser& p) const {
  p.clear_operators();
  // structural call operators — not registered in mfuncs; lowest precedence, right-assoc.
  // `abc = {...}` / `abc ?= {...}` lower than any math so the rhs expression binds first.
  p.add_operator("=",  tavl::op_fixity::binary, 1, tavl::op_assoc::right);
  p.add_operator("?=", tavl::op_fixity::binary, 1, tavl::op_assoc::right);

  for (const auto& [name, overloads] : mfuncs) {
    if (overloads.empty()) continue;
    const auto& cd = overloads.front();        // operator syntax metadata is shared by all overloads
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

void system::emit_command_name(parse_ctx* ctx, container* scr, const std::string_view& name) const {
  const auto ref = store_string(scr, name);
  scr->cmds.push_back(container::command(&push_command_name, packstrid(uint32_t(ref.start), uint32_t(ref.count))));
  ctx->push<std::string_view>();
}

void system::finalize_resource_usage(parse_context& ctx, container& c) const {
  // Own-frame peak is whatever push() observed; the saved frame is this script's declared slots plus
  // the deepest saved frame any executed sub-script needs stacked on top (see container::max_saved).
  c.max_stack = ctx.max_stack_depth;
  c.max_saved = c.saved.size() + ctx.max_child_saved;
  // Peak ctx->lists size = own lists + deepest nested execute list-frame; create_lists reserves it.
  c.max_lists = c.lists.size() + ctx.max_child_lists;
  if (c.max_stack > ctx.max_stack_limit)
    raise_error(std::format("Script '{}' needs a stack of {}, exceeding the limit of {}", c.get_name(), c.max_stack, ctx.max_stack_limit));
  if (c.max_saved > ctx.max_saved_limit)
    raise_error(std::format("Script '{}' needs {} saved-value slots, exceeding the limit of {}", c.get_name(), c.max_saved, ctx.max_saved_limit));
}

std::tuple<tavl::event, tavl::error> system::parse(std::string_view name, tavl::parser& p, parse_context& ctx, container& c) const {
  if (!ctx.initialized) raise_error("parse_context is not initialized");
  if (c.name.count == 0 && c.name.start == 0) c.name = store_string(&c, name);

  auto [ev, err] = make_script_ast(p, ctx.script_ast_ctx, ctx.script_ast_nodes);
  if (ev.type == tavl::event_type::not_enought_data || ctx.script_ast_nodes.empty()) return {ev, err};
  if (!err.no_error()) return {ev, err};

  // Reconstruct this batch's raw source into a local (padding keeps tavl spans at absolute
  // offsets). It feeds normalize only; the container's `c.string_pool` is the compact token pool
  // that `store_string` accumulates across calls, never the raw text.
  const size_t storage_end = p.storage.size();
  const size_t live_size = p.storage.buffer_size();
  const size_t released_size = storage_end - live_size;
  const auto live_src = p.content(tavl::source_span{released_size, live_size, 1, 1});
  std::string raw_src(released_size, ' ');
  raw_src.append(live_src);

  try {
    ctx.rpn_ctx.token_storage.reserve(raw_src.size() + 4096);
    ctx.rpn_ctx.normalize(ctx.script_ast_nodes, std::string_view(raw_src));
    auto output = ctx.rpn_ctx.output;
    ctx.script_ast_nodes.clear();

    const std::string_view root_block = !ctx.root_block_name.empty() ? ctx.root_block_name : std::string_view("__effect_block");
    output.emplace(output.begin(), rpn_conversion_ctx::block{ ctx.rpn_ctx.store_token(root_block), output.size() + 1 });

    reserve_from_hint(&c, output.size(), ctx.rpn_ctx.token_storage.size());

    command_block script_cmds{std::span<rpn_conversion_ctx::block>(output), &ctx.rpn_ctx.token_storage};
    {
      set_expected_type set(&ctx, ctx.return_type);
      dispatch_node(&ctx, &c, script_cmds);
    }
    if (const auto cd = static_string_arg(script_cmds.find(custom_description_constant), custom_description_constant); !cd.empty() && !c.block_descs.empty()) {
      c.block_descs.back().custom_description = store_string(&c, cd);
    }
    ctx.rpn_ctx.clear();

    while (ctx.pop_while_ignore()) {}

    if (!ctx.root_type.empty()) {
      if (ctx.scope_stack.size() != 1) raise_error(std::format("There is not closed scope of type '{}' on stack", ctx.stack_types[ctx.scope_stack.back()]));
      scope_exit(&ctx, &c, 1);
    }

    if (!type_is_void(ctx.return_type)) {
      if (!ctx.stack_types.empty() && ctx.stack_types.back() != ctx.return_type && can_convert_implicitly(ctx.stack_types.back(), ctx.return_type)) {
        setup_type_conversion(&ctx, &c, ctx.stack_types.back(), ctx.return_type);
      }
      if (ctx.stack_types.empty() || ctx.stack_types.back() != ctx.return_type) {
        const auto actual = ctx.stack_types.empty() ? std::string_view("empty stack") : ctx.stack_types.back();
        raise_error(std::format("Invalid return type '{}' expected '{}', stack size {}", actual, ctx.return_type, ctx.stack_types.size()));
      }
      push_basic_function(&ctx, &c, basicf::pushreturn, 0);
    }

    if (ctx.stack_types.size() != 0) raise_error(std::format("Script is not properly ended, {} values on stack", ctx.stack_types.size()));

    finalize_resource_usage(ctx, c);
    optimize_commands(ctx, c);
  } catch (const std::exception& ex) {
    ctx.rpn_ctx.clear();
    ctx.script_ast_nodes.clear();
    ctx.warnings.clear();
    // Keep this overload's established error-return contract even if the configured callback
    // reports by throwing (as the default callback does).
    try { error(ex.what()); } catch (...) {}
    return {ev, tavl::error{tavl::error_type::err_misplaced_operator, ev.token.span}};
  }

  for (const auto& msg : ctx.warnings) raise_warning(msg);
  ctx.warnings.clear();
  c.build_description_index();
  return {ev, err};
}

system::command_data::ftype system::get_token_type(const std::string_view& name) const {
  const auto itr = mfuncs.find(std::string(name));
  if (itr == mfuncs.end()) return command_data::ftype::invalid;
  if (itr->second.empty()) return command_data::ftype::invalid;
  return itr->second.front().type;
}

std::tuple<int32_t, int32_t, system::command_data::associativity, system::command_data::ftype> system::get_token_caps(const std::string_view& name) const {
  const auto itr = mfuncs.find(std::string(name));
  if (itr == mfuncs.end()) return std::make_tuple(0, 0, system::command_data::associativity::left, command_data::ftype::invalid);
  if (itr->second.empty()) return std::make_tuple(0, 0, system::command_data::associativity::left, command_data::ftype::invalid);

  int32_t args_count = 0;
  for (auto it = itr->second.begin(); it != itr->second.end(); ++it) {
    args_count = std::max(args_count, it->arg_count);
  }

  auto it = itr->second.begin();
  return std::make_tuple(it->priority, args_count, it->assoc, it->type);
}

system::parse_context::parse_context() noexcept :
  ftype(function_type::lvalue), nest_level(0), source_line(0), source_column(0), unlimited_func_index(SIZE_MAX), list_index_upvalue(SIZE_MAX), prev_chaining(0), description_placeholder_depth(0),
  max_stack_depth(0), max_child_saved(0), max_child_lists(0), max_stack_limit(context::stack_size), max_saved_limit(context::local_vars_size), conversion_cost(0), initialized(false)
{}

bool system::parse_context::is_func_subblock() const {
  if (function_names.size() < 2) return false;
  return function_names[function_names.size() - 2] == function_names.back();
}

void system::parse_context::push_func(const std::string_view& name) {
  function_names.push_back(name);
}

void system::parse_context::pop_func() {
  function_names.pop_back();
}

size_t system::parse_context::current_scope_index() const {
  if (scope_stack.empty()) return SIZE_MAX;
  return scope_stack.back();
}

std::string_view system::parse_context::current_scope_type() const {
  if (scope_stack.empty()) return std::string_view();
  return stack_types[scope_stack.back()];
}

bool system::parse_context::is_ignore() const { return type_is_ignore(top()); }
bool system::parse_context::is_bool() const { return type_is_bool(top()); }
bool system::parse_context::is_integral() const { return type_is_integral(top()); }
bool system::parse_context::is_number() const { return type_is_floating_point(top()); }
bool system::parse_context::is_fundamental() const { return type_is_fundamental(top()); }
bool system::parse_context::is_string() const { return type_is_string(top()); }
bool system::parse_context::is_object() const { return type_is_object(top()); }
bool system::parse_context::pop_while_ignore() { if (!stack_types.empty() && is_ignore()) { pop(); return true; } return false; }
void system::parse_context::push(const std::string_view& type) {
  stack_types.push_back(type);
  if (stack_types.size() > max_stack_depth) max_stack_depth = stack_types.size();
}
void system::parse_context::pop() {
  if (stack_types.empty()) throw std::runtime_error(std::format("Trying to remove value from empty stack, current function '{}'", function_names.back()));
  stack_types.pop_back();
}

void system::parse_context::erase(const size_t index) {
  if (index >= stack_types.size()) throw std::runtime_error(std::format("Trying to remove value #{} from stack with {} values, current function '{}'", index, stack_types.size(), function_names.back()));
  stack_types.erase(stack_types.begin() + index);
}

std::string_view system::parse_context::top() const { return stack_types.back(); }

}
