#include "devils_script/script_ast.h"
#include "devils_script/common.h"

#include <algorithm>
#include <format>
#include <stdexcept>

// devils_script AST builder over tavl's event stream. See script_ast.h for the language model.
// Implementation mirrors tavl's make_math_ast (Pratt) but with devils_script bracket semantics:
//   - `{}` is ALWAYS an object node (no single-child unwrap, unlike make_math_ast);
//   - `=` / `?=` are binary call-operators (registered lowest precedence) so `fn = {...}` is a pair
//     carrying its op token; a juxtaposed call `f(...)` / `f{...}` is a pair with an EMPTY op token.
// Each parse_* returns a subtree in PREFIX order, so a parent's child_count == sum of child sizes.

namespace devils_script {

namespace {

using subtree = std::vector<tavl::node>;

// How deep one row's expression may nest. tavl bounds bracket nesting, but not operator chains:
// `- - - ... 1` recursed here, and `a + b + ...` is built in a loop yet yields a tree as deep as the
// chain, which normalization and the semantic pass then walk recursively. Thousands of levels
// overflowed the C++ stack; 256 also keeps such a row within a small thread stack.
constexpr size_t max_expression_depth = 256;

std::string format_syntax_error(const tavl::source_span& span, std::string_view what) {
  return std::format("Syntax error @ {}:{}: {}", span.line, span.column, what);
}

// A malformed expression found by the Pratt pass. `err` is what the streaming API returns for the
// row; what() is the full message.
class script_syntax_error final : public std::runtime_error {
public:
  script_syntax_error(const tavl::error_type type, const tavl::source_span& span, std::string_view what) :
    std::runtime_error(format_syntax_error(span, what)), err{type, span} {}
  tavl::error err;
};

struct ev_cursor {
  const std::vector<tavl::event>& evs;
  const tavl::parser& p;
  size_t i = 0;
  size_t depth = 0;   // recursion depth of the parse_* functions

  const tavl::event& peek() const {
    static const tavl::event none{};
    return i < evs.size() ? evs[i] : none;
  }
  tavl::event_type type() const { return peek().type; }
  const tavl::token& tok() const { return peek().token; }
  bool is_op() const { return tok().type == tavl::token_type::op; }
  std::string_view content() const { return p.content(tok().span); }
  void next() { ++i; }
  // Only these may follow a complete expression; tavl closes every row before its block.
  bool at_row_end() const {
    const auto t = type();
    return t == tavl::event_type::row_end || t == tavl::event_type::invalid ||
           t == tavl::event_type::object_end || t == tavl::event_type::tuple_end ||
           t == tavl::event_type::array_end;
  }
};

bool is_begin_block(tavl::event_type t) {
  return t == tavl::event_type::object_begin || t == tavl::event_type::tuple_begin ||
         t == tavl::event_type::array_begin;
}
bool is_end_block(tavl::event_type t) {
  return t == tavl::event_type::object_end || t == tavl::event_type::tuple_end ||
         t == tavl::event_type::array_end;
}
tavl::node_type block_node_type(tavl::event_type t) {
  switch (t) {
    case tavl::event_type::object_begin: return tavl::node_type::object;
    case tavl::event_type::tuple_begin:  return tavl::node_type::tuple;
    case tavl::event_type::array_begin:  return tavl::node_type::array;
    default:                             return tavl::node_type::invalid;
  }
}

subtree make_leaf(const tavl::token& t) { return { tavl::node{tavl::node_type::token, t, 0} }; }

// prepend a parent node over already-built child subtrees (concatenated)
subtree wrap(tavl::node_type nt, const tavl::token& t, const std::vector<subtree>& kids) {
  size_t footprint = 0;
  for (const auto& k : kids) footprint += k.size();
  subtree out;
  out.reserve(1 + footprint);
  out.push_back(tavl::node{nt, t, footprint});
  for (const auto& k : kids) out.insert(out.end(), k.begin(), k.end());
  return out;
}

subtree parse_expr(ev_cursor& c, int min_prec);

struct depth_guard {
  ev_cursor& c;
  explicit depth_guard(ev_cursor& c) : c(c) {
    if (++c.depth > max_expression_depth)
      throw script_syntax_error(tavl::error_type::err_nesting_too_deep, c.tok().span,
                                std::format("expression nests deeper than {} levels", max_expression_depth));
  }
  ~depth_guard() { --c.depth; }
};

// Depth of a flat prefix tree (node.child_count = footprint).
size_t tree_depth(const subtree& t) {
  size_t max_depth = 0;
  std::vector<size_t> ends;   // one past the last node of each open ancestor
  for (size_t i = 0; i < t.size(); ++i) {
    while (!ends.empty() && i >= ends.back()) ends.pop_back();
    ends.push_back(i + 1 + t[i].child_count);
    max_depth = std::max(max_depth, ends.size());
  }
  return max_depth;
}

// one row of a block: a single expression, nothing after it
subtree parse_row(ev_cursor& c) {
  const auto start = c.tok();
  subtree row = parse_expr(c, 0);
  if (tree_depth(row) > max_expression_depth)
    throw script_syntax_error(tavl::error_type::err_nesting_too_deep, start.span,
                              std::format("expression nests deeper than {} levels", max_expression_depth));
  if (!c.at_row_end())
    throw script_syntax_error(tavl::error_type::err_too_many_values, c.tok().span,
                              std::format("unexpected '{}' after the expression", c.content()));
  if (c.type() == tavl::event_type::row_end) c.next();
  return row;
}

// The operand of `op` must start here. Without this check the operand was an empty-token leaf,
// which a string context accepted as "".
void require_operand(const ev_cursor& c, const tavl::token& op) {
  if (!c.at_row_end()) return;
  throw script_syntax_error(tavl::error_type::err_missing_value, op.span,
                            std::format("missing operand after '{}'", c.p.content(op.span)));
}

// a bracket group: consume *_begin, each inner row -> a child expr, consume *_end.
// `{}` ALWAYS becomes an object node (no single-child unwrap); `()`/`[]` likewise wrapped here.
subtree parse_bracket(ev_cursor& c) {
  const auto open = c.tok();
  const auto nt = block_node_type(c.type());
  c.next();                                    // eat *_begin
  std::vector<subtree> kids;
  while (c.type() == tavl::event_type::row_begin) {
    c.next();                                  // eat row_begin
    kids.push_back(parse_row(c));
  }
  if (is_end_block(c.type())) c.next();         // eat *_end
  return wrap(nt, open, kids);
}

subtree parse_primary(ev_cursor& c) {
  if (is_begin_block(c.type())) return parse_bracket(c);
  const auto t = c.tok();                       // operand leaf (identifier / unrecognized path / number)
  // An operator can be a leaf (a function named by a literal operator: `m = {...}`), but the call
  // operators themselves cannot: `= x` has no function to call.
  if (c.is_op() && (c.content() == "=" || c.content() == "?="))
    throw script_syntax_error(tavl::error_type::err_expected_identifier, t.span,
                              std::format("expected a function name before '{}'", c.content()));
  // A literal that is plainly an integer but does not fit is rejected here, where its position is
  // known, rather than deep inside overload resolution.
  if (script_int_t value = 0; parse_integer_literal(c.content(), value) == integer_literal::out_of_range)
    throw script_syntax_error(tavl::error_type::err_number_out_of_range, t.span, integer_literal_out_of_range_message(c.content()));
  c.next();
  return make_leaf(t);
}

subtree parse_postfix(ev_cursor& c) {
  subtree v = parse_primary(c);
  for (;;) {
    if (c.is_op() && c.p.operator_info(c.content(), tavl::op_fixity::postfix)) {
      const auto op = c.tok(); c.next();
      v = wrap(tavl::node_type::pair, op, { v });             // unary postfix
    } else if (is_begin_block(c.type())) {
      const subtree args = parse_bracket(c);                  // call: f(...) / f{...}
      v = wrap(tavl::node_type::pair, tavl::token{}, { v, args });
    } else break;
  }
  return v;
}

subtree parse_prefix(ev_cursor& c) {
  depth_guard guard(c);
  if (c.is_op() && c.p.operator_info(c.content(), tavl::op_fixity::prefix)) {
    const auto op = c.tok(); c.next();
    require_operand(c, op);
    const subtree operand = parse_prefix(c);
    return wrap(tavl::node_type::pair, op, { operand });      // unary prefix
  }
  return parse_postfix(c);
}

subtree parse_expr(ev_cursor& c, int min_prec) {
  depth_guard guard(c);
  subtree lhs = parse_prefix(c);
  for (;;) {
    if (!c.is_op()) break;
    const auto info = c.p.operator_info(c.content(), tavl::op_fixity::binary);
    if (!info || info->precedence < min_prec) break;
    const auto op = c.tok(); c.next();
    require_operand(c, op);
    const int next_min = (info->assoc == tavl::op_assoc::right) ? info->precedence : info->precedence + 1;
    const subtree rhs = parse_expr(c, next_min);
    lhs = wrap(tavl::node_type::pair, op, { lhs, rhs });
  }
  return lhs;
}

bool is_script_event(tavl::event_type t) {
  return t != tavl::event_type::empty_row && t != tavl::event_type::got_comment;
}

// tavl's critical errors are lexical/structural, except err_expected_identifier: that is the
// data-profile rule that an object row needs a name. A devils_script `{}` is an expression list,
// where `{ -7 / 2 }` or a call to a literal operator `{ m = {...} }` is valid; the Pratt pass and
// the semantic pass judge such rows instead.
bool is_syntax_error(const tavl::error& err) {
  return err.is_critical() && err.type != tavl::error_type::err_expected_identifier;
}

std::string syntax_error_message(const tavl::error& err) {
  // `err_unterminated_string` -> `unterminated string`
  std::string what(tavl::to_string(err.type));
  if (what.starts_with("err_")) what.erase(0, 4);
  else if (what.starts_with("warn_")) what.erase(0, 5);
  std::replace(what.begin(), what.end(), '_', ' ');
  return format_syntax_error(err.span, what);
}

void build_script_ast(const std::vector<tavl::event>& evs, const tavl::parser& p, std::vector<tavl::node>& ast_nodes) {
  ev_cursor c{ evs, p, 0 };
  std::vector<subtree> rows;
  while (c.type() == tavl::event_type::row_begin) {
    c.next();                                   // eat root row_begin
    rows.push_back(parse_row(c));
  }

  // wrap top-level rows in a synthetic root block; the semantic pass picks the combinator.
  ast_nodes = wrap(tavl::node_type::object, tavl::token{}, rows);
}

}  // namespace

void script_ast_context::clear() {
  events.clear();
  error = tavl::error{};
  nest_counter = 0;
  got_start = false;
}

std::tuple<tavl::event, tavl::error> make_script_ast(tavl::parser& p, script_ast_context& ctx, std::vector<tavl::node>& ast_nodes) {
  ast_nodes.clear();
  // The row is complete: build it unless tavl already rejected it, then reset for the next row.
  const auto finish_row = [&](const tavl::event& ev) -> std::tuple<tavl::event, tavl::error> {
    auto row_error = ctx.error;
    std::string message = row_error.no_error() ? std::string() : syntax_error_message(row_error);
    if (row_error.no_error() && !ctx.events.empty()) {
      try {
        build_script_ast(ctx.events, p, ast_nodes);
      } catch (const script_syntax_error& e) {
        ast_nodes.clear();
        row_error = e.err;
        message = e.what();
      }
    }
    ctx.clear();
    ctx.error_message = std::move(message);
    return {ev, row_error};
  };

  for (;;) {
    auto [ev, err] = p.poll_event();
    // Critical errors can arrive on any event, including the comments and empty rows skipped
    // below; keep the first one and report it when the row ends, so the stream stays in sync.
    if (is_syntax_error(err) && ctx.error.no_error()) ctx.error = err;
    if (ev.type == tavl::event_type::not_enought_data) return {ev, tavl::error{}};

    if (ev.type == tavl::event_type::eof) return finish_row(ev);

    if (!is_script_event(ev.type)) continue;

    if (!ctx.got_start && ev.type != tavl::event_type::row_begin) {
      ctx.events.push_back(tavl::event{tavl::event_type::row_begin, ev.token});
      ctx.got_start = true;
    }

    if (ev.type == tavl::event_type::row_begin) {
      ctx.got_start = true;
    } else if (is_begin_block(ev.type)) {
      ctx.nest_counter += 1;
    } else if (is_end_block(ev.type) && ctx.nest_counter > 0) {
      ctx.nest_counter -= 1;
    }

    ctx.events.push_back(ev);

    if (ev.type == tavl::event_type::row_end && ctx.nest_counter == 0) return finish_row(ev);
  }
}

std::vector<tavl::node> make_script_ast(tavl::parser& p, std::string_view src) {
  p.clear();
  p.flush(src);
  p.finish();

  std::vector<tavl::event> evs;
  std::vector<tavl::node> nodes;
  for (;;) {
    auto [ev, err] = p.poll_event();
    if (is_syntax_error(err)) throw std::runtime_error(syntax_error_message(err));
    if (ev.type == tavl::event_type::eof) break;
    if (!is_script_event(ev.type)) continue;
    evs.push_back(ev);
  }
  build_script_ast(evs, p, nodes);
  return nodes;
}

}
