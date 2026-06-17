#pragma once

#include <vector>
#include <string_view>

#include "tavl/tavl.h"
#include "tavl/ext.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

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
// `p` must have its operators registered already (see system::configure_parser). The whole `src`
// is flushed at once and finished; no streaming. Error handling / recovery is NOT done here yet.
std::vector<tavl::node> make_script_ast(tavl::parser& p, std::string_view src);

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
