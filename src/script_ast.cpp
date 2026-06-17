#include "devils_script/script_ast.h"

// devils_script AST builder over tavl's event stream. See script_ast.h for the language model.
// Implementation mirrors tavl's make_math_ast (Pratt) but with devils_script bracket semantics:
//   - `{}` is ALWAYS an object node (no single-child unwrap, unlike make_math_ast);
//   - `=` / `?=` are binary call-operators (registered lowest precedence) so `fn = {...}` is a pair
//     carrying its op token; a juxtaposed call `f(...)` / `f{...}` is a pair with an EMPTY op token.
// Each parse_* returns a subtree in PREFIX order, so a parent's child_count == sum of child sizes.

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

namespace {

using subtree = std::vector<tavl::node>;

struct ev_cursor {
  const std::vector<tavl::event>& evs;
  const tavl::parser& p;
  size_t i = 0;

  const tavl::event& peek() const {
    static const tavl::event none{};
    return i < evs.size() ? evs[i] : none;
  }
  tavl::event_type type() const { return peek().type; }
  const tavl::token& tok() const { return peek().token; }
  bool is_op() const { return tok().type == tavl::token_type::op; }
  std::string_view content() const { return p.content(tok().span); }
  void next() { ++i; }
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

// a bracket group: consume *_begin, each inner row -> a child expr, consume *_end.
// `{}` ALWAYS becomes an object node (no single-child unwrap); `()`/`[]` likewise wrapped here.
subtree parse_bracket(ev_cursor& c) {
  const auto open = c.tok();
  const auto nt = block_node_type(c.type());
  c.next();                                    // eat *_begin
  std::vector<subtree> kids;
  while (c.type() == tavl::event_type::row_begin) {
    c.next();                                  // eat row_begin
    kids.push_back(parse_expr(c, 0));
    if (c.type() == tavl::event_type::row_end) c.next();
  }
  if (is_end_block(c.type())) c.next();         // eat *_end
  return wrap(nt, open, kids);
}

subtree parse_primary(ev_cursor& c) {
  if (is_begin_block(c.type())) return parse_bracket(c);
  const auto t = c.tok();                       // operand leaf (identifier / unrecognized path / number)
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
  if (c.is_op() && c.p.operator_info(c.content(), tavl::op_fixity::prefix)) {
    const auto op = c.tok(); c.next();
    const subtree operand = parse_prefix(c);
    return wrap(tavl::node_type::pair, op, { operand });      // unary prefix
  }
  return parse_postfix(c);
}

subtree parse_expr(ev_cursor& c, int min_prec) {
  subtree lhs = parse_prefix(c);
  for (;;) {
    if (!c.is_op()) break;
    const auto info = c.p.operator_info(c.content(), tavl::op_fixity::binary);
    if (!info || info->precedence < min_prec) break;
    const auto op = c.tok(); c.next();
    const int next_min = (info->assoc == tavl::op_assoc::right) ? info->precedence : info->precedence + 1;
    const subtree rhs = parse_expr(c, next_min);
    lhs = wrap(tavl::node_type::pair, op, { lhs, rhs });
  }
  return lhs;
}

}  // namespace

std::vector<tavl::node> make_script_ast(tavl::parser& p, std::string_view src) {
  p.clear();
  p.flush(src);
  p.finish();

  std::vector<tavl::event> evs;
  for (;;) {
    auto [ev, err] = p.poll_event();
    (void)err;
    if (ev.type == tavl::event_type::eof) break;
    evs.push_back(ev);
  }

  ev_cursor c{ evs, p, 0 };
  std::vector<subtree> rows;
  while (c.type() == tavl::event_type::row_begin) {
    c.next();                                   // eat root row_begin
    rows.push_back(parse_expr(c, 0));
    if (c.type() == tavl::event_type::row_end) c.next();
  }

  // wrap top-level rows in a synthetic root block; the semantic pass picks the combinator.
  return wrap(tavl::node_type::object, tavl::token{}, rows);
}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
