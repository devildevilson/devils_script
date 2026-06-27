#pragma once

#include <vector>
#include <string_view>
#include <tuple>

#include "tavl/tavl.h"
#include "tavl/ext.h"

// AST adapter between tavl and the devils_script semantic compiler.
//
// tavl owns tokenization, comments, string literal escaping, brackets, and operator
// precedence. This layer turns tavl's event stream into the flat prefix `tavl::node`
// layout consumed by `system::rpn_conversion_ctx::normalize`. The semantic compiler still
// performs scope-path splitting, function lookup, type checking, and bytecode emission.
//
// The important language distinction is that `=` and `?=` are call operators, not storage
// assignment. Storage is expressed explicitly through builtins such as `ctx_save` and
// `ctx_set`.

namespace devils_script {

// Builds the devils_script AST over tavl's event stream and returns ONE flat tavl::node tree
// (prefix order: nodes[0] = root, descendants flat after it; node.child_count = footprint).
// The root is a synthetic `object` node (empty token) whose children are the top-level rows; the
// semantic pass assigns the root combinator (AND/ADD/effect/...) from the script's RETURN_T.
//
// Language model: `=` / `?=` are CALL operators (lowest precedence, right-assoc), not assignment —
// `abc = {...}` == `abc(...)`. Math operators bind tighter and are parsed with a single Pratt pass.
// `{}` is ALWAYS a block (object node), never unwrapped. A call `f(...)` / `f{...}` is a pair with
// an EMPTY operator token; a `=`/`?=`/math pair carries its operator token. Scope paths `a.b:c`
// arrive as one `unrecognized` token and are kept as a leaf (split later, at the semantic stage).
//
struct script_ast_context {
  std::vector<tavl::event> events;
  size_t nest_counter = 0;
  bool got_start = false;

  void clear();
};

// `p` must have its operators registered already (see system::configure_parser). Parses one row
// from the current event stream and returns its terminal event (`row_end`, `eof`, or
// `not_enought_data`). On `not_enought_data`, `ctx` keeps enough state to resume later.
std::tuple<tavl::event, tavl::error> make_script_ast(tavl::parser& p, script_ast_context& ctx, std::vector<tavl::node>& ast_nodes);

// Whole-string helper kept for non-streaming callers.
std::vector<tavl::node> make_script_ast(tavl::parser& p, std::string_view src);

}
