#include "devils_script/system.h"

#include <array>
#include <format>
#include <stdexcept>
#include "devils_script/string-utils.hpp"
#include "tavl/detail.h"

namespace devils_script {

std::tuple<system::rpn_conversion_ctx::token_ref, size_t> system::rpn_conversion_ctx::convert_scope(const std::string_view& expr, block* arr, const size_t max_size, const size_t line, const size_t column) {
  using kind = block_kind;

  size_t counter = 0;
  std::array<std::string_view, 16> dot_arr;
  const size_t count = utils::string::split(expr, ".", dot_arr.data(), dot_arr.size());
  if (count == SIZE_MAX) throw std::runtime_error(std::format("Could not parse expr '{}', too many dots", expr));

  size_t size = 0;
  token_ref lfn;
  for (size_t i = 0; i < count; ++i) {
    if (lfn.offset != SIZE_MAX) throw std::runtime_error(std::format("Could not parse expr '{}', found scope call stack after left value function", expr));
    if (counter >= max_size) throw std::runtime_error(std::format("Could not parse expr '{}', not enough 'block' memory ({})", expr, max_size));

    std::array<std::string_view, 3> colon_arr;
    const size_t colon_count = utils::string::split(dot_arr[i], ":", colon_arr.data(), colon_arr.size());
    if (colon_count == SIZE_MAX) throw std::runtime_error(std::format("Could not parse expr '{}' in lvalue '{}', too many colons", dot_arr[i], expr));

    arr[counter] = { store_token(colon_arr[0], line, column), 1, kind::scope_path }; counter += 1;
    size += 1;
    if (colon_count > 2) {
      arr[counter] = { store_token(colon_arr[1], line, column), 2, kind::scope_path_call }; counter += 1;
      arr[counter] = { store_token(colon_arr[2], line, column), 1 }; counter += 1;
      size += 2;
    } else if (colon_count == 2) {
      lfn = store_token(colon_arr[1], line, column);
    }
  }

  if (lfn.offset == SIZE_MAX && counter == 1) return std::make_tuple(arr[0].token, 0);

  for (size_t i = 0; i < counter;) {
    const size_t cursize = arr[i].size;
    arr[i].size = size;
    size -= cursize;
    i += cursize;
  }

  return std::make_tuple(lfn, counter);
}

std::string_view system::rpn_conversion_ctx::token_text(const token_ref& token) const noexcept {
  if (token.offset == SIZE_MAX) return std::string_view();
  if (token.offset + token.size > token_storage.size()) return std::string_view();
  return std::string_view(token_storage).substr(token.offset, token.size);
}

auto system::rpn_conversion_ctx::store_token(std::string_view text, const size_t line, const size_t column) -> token_ref {
  if (text.empty()) return token_ref{ token_storage.size(), 0, line, column };

  const size_t offset = token_storage.size();
  token_storage.append(text.data(), text.size());
  return token_ref{ offset, text.size(), line, column };
}

namespace {
// text of a tavl token via its source span (empty for synthetic/empty tokens, e.g. call operator)
std::string_view node_text(const tavl::node* n, std::string_view src) {
  const auto& sp = n->token.span;
  return sp.offset == SIZE_MAX ? std::string_view() : src.substr(sp.offset, sp.size);
}
system::rpn_conversion_ctx::token_ref node_token_ref(system::rpn_conversion_ctx* ctx, const tavl::node* n, std::string_view src) {
  const auto& sp = n->token.span;
  return ctx->store_token(node_text(n, src), sp.line, sp.column);
}
bool is_string_literal(const tavl::node* n) {
  return n->token.type == tavl::token_type::singlequote_string || n->token.type == tavl::token_type::doublequote_string;
}
// direct children of a flat-prefix node (step by child_count+1 over its footprint)
std::vector<const tavl::node*> node_children(const tavl::node* n) {
  std::vector<const tavl::node*> r;
  for (size_t i = 1; i < n->child_count + 1; i += n[i].child_count + 1) r.push_back(n + i);
  return r;
}

system::rpn_conversion_ctx::block_kind call_kind(const bool nullable, const bool braced) noexcept {
  using kind = system::rpn_conversion_ctx::block_kind;
  if (nullable && braced) return kind::nullable_braced_call;
  if (nullable) return kind::nullable_call;
  if (braced) return kind::braced_call;
  return kind::node;
}

size_t direct_child_count(const system::rpn_conversion_ctx::block* data, const size_t size) {
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

size_t direct_child_count(std::span<system::rpn_conversion_ctx::block> data) {
  return data.empty() ? 0 : direct_child_count(data.data(), data.size());
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

auto system::rpn_conversion_ctx::normalize_token(const tavl::node* n, std::string_view src) -> token_ref {
  const auto raw = node_text(n, src);
  const auto& sp = n->token.span;
  if (!is_string_literal(n)) return store_token(raw, sp.line, sp.column);

  std::string out;
  out.reserve(raw.size());
  auto inner = raw;
  if (!inner.empty()) inner.remove_prefix(1);
  if (!inner.empty()) inner.remove_suffix(1);

  if (n->token.type == tavl::token_type::singlequote_string)
    tavl::detail::unescape_singlequote_string(inner, out);
  else
    tavl::detail::unescape_doublequote_string(inner, out);

  return store_token(out, sp.line, sp.column);
}

// Port of convert_block's per-token body, sourcing (lvalue, rhs) from a tavl row node.
// Same size/args bookkeeping; reuses convert_scope.
void system::rpn_conversion_ctx::normalize_row(const tavl::node* row, std::string_view src) {
  const size_t cur_size = output.size();
  const auto empty_lvalue = store_token("__empty_lvalue");
  token_ref lfn = empty_lvalue;
  std::array<block, 16 * 3 + 1> arr;
  size_t lvalue_tokens_count = 0;

  std::string_view lvalue;
  size_t lvalue_line = 0, lvalue_column = 0;
  const tavl::node* rhs = row;
  bool nullable_call = false;
  if (row->type == tavl::node_type::pair) {
    const auto op = node_text(row, src);
    if (op == "=" || op == "?=") {            // call/assignment: lhs = function, rhs = its argument
      const auto ch = node_children(row);
      if (ch.size() != 2 || ch[0]->type != tavl::node_type::token)
        throw std::runtime_error(std::format("Assignment operator '{}' expects a plain function name on the left side", op));
      lvalue = node_text(ch[0], src);          // lhs is a single token (scope paths arrive whole)
      lvalue_line = ch[0]->token.span.line;
      lvalue_column = ch[0]->token.span.column;
      rhs = ch[1];
      nullable_call = op == "?=";
    }
  }

  if (!lvalue.empty()) {
    const auto [func_name, count] = convert_scope(lvalue, arr.data(), arr.size(), lvalue_line, lvalue_column);
    lvalue_tokens_count = count;
    if (func_name.offset != SIZE_MAX) lfn = func_name;
  }

  for (size_t i = 0; i < lvalue_tokens_count; i += direct_child_count(arr.data() + i, lvalue_tokens_count - i) + 1) {
    output.push_back(arr[i]);
    const size_t children = direct_child_count(arr.data() + i, lvalue_tokens_count - i);
    for (size_t j = i + 1; j < i + children + 1; ++j)
      output.push_back(arr[j]);
  }

  size_t lfn_index = SIZE_MAX;
  const bool lvalue_is_empty = token_text(lfn) == "__empty_lvalue";
  const bool rhs_is_block = rhs->type == tavl::node_type::object;
  if (lvalue_is_empty && rhs_is_block) { output.push_back({ lfn, 1 }); lfn_index = output.size() - 1; }
  if (!lvalue_is_empty) { output.push_back({ lfn, 1 }); lfn_index = output.size() - 1; }

  const size_t block_size = output.size();
  if (rhs_is_block) normalize_block(rhs, src);
  else normalize_expr(rhs, src);

  const size_t size = output.size() - block_size;
  for (size_t i = cur_size; i < cur_size + lvalue_tokens_count; i += direct_child_count(output.data() + i, output.size() - i) + 1)
    output[i].size += size + 1;
  if (lfn_index != SIZE_MAX) {
    output[lfn_index].size += size;
    output[lfn_index].kind = call_kind(nullable_call, rhs_is_block);
  }
}

// Expression (rvalue) -> prefix blocks, mirroring convert()'s output but reading tavl's already
// precedence-correct tree. Returns the footprint (1 + descendants).
size_t system::rpn_conversion_ctx::normalize_expr(const tavl::node* n, std::string_view src) {
  if (n->type == tavl::node_type::token) {
    const auto kind = is_string_literal(n) ? block_kind::string_literal : block_kind::node;
    output.push_back({ normalize_token(n, src), 1, kind });   // leaf (incl. whole rvalue scope paths)
    return 1;
  }

  if (n->type == tavl::node_type::pair) {
    const auto ch = node_children(n);
    const auto op = node_text(n, src);
    if (op == "=" || op == "?=")
      throw std::runtime_error(std::format("Assignment operator '{}' is not allowed inside expressions", op));

    if (op.empty()) {                                 // call f(...) / f{...}: pair with empty op token
      const size_t idx = output.size();
      output.push_back({ node_token_ref(this, ch[0], src), 1 });
      size_t s = 0;
      for (const auto* a : node_children(ch[1])) s += normalize_expr(a, src);
      output[idx].size += s;
      return 1 + s;
    }

    if (ch.size() == 1) {                             // unary prefix (rpn names: unary_minus/unary_plus)
      std::string_view o = op == "-" ? "unary_minus" : (op == "+" ? "unary_plus" : op);
      const size_t idx = output.size();
      output.push_back({ store_token(o, n->token.span.line, n->token.span.column), 1 });
      const size_t s = normalize_expr(ch[0], src);
      output[idx].size += s;
      return 1 + s;
    }

    const size_t idx = output.size();                 // binary
    output.push_back({ store_token(op, n->token.span.line, n->token.span.column), 1 });
    const size_t s = normalize_expr(ch[0], src) + normalize_expr(ch[1], src);
    output[idx].size += s;
    return 1 + s;
  }

  if (n->type == tavl::node_type::object) {           // anonymous block at expr position
    const size_t idx = output.size();
    output.push_back({ store_token("__empty_lvalue", n->token.span.line, n->token.span.column), 1, block_kind::braced_call });
    normalize_block(n, src);
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
  token_storage.clear();
}

system::command_block::command_block() noexcept {}
system::command_block::command_block(const std::span<rpn_conversion_ctx::block>& data, const std::string* token_storage) noexcept : data(data), token_storage(token_storage) {}
system::command_block::command_block(const command_block& block, const size_t index) noexcept {
  if (index < block.size()) {
    data = std::span(&block.data[index], block.data[index].size);
    token_storage = block.token_storage;
  }
}

system::command_block system::command_block::find(const std::string_view& name) const {
  size_t counter = 1;
  while (counter < data.size()) {
    if (token_storage != nullptr) {
      const auto& token = data[counter].token;
      if (token.offset != SIZE_MAX && token.offset + token.size <= token_storage->size() &&
          std::string_view(*token_storage).substr(token.offset, token.size) == name) return command_block(*this, counter);
    }
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

std::string_view system::command_block::name() const {
  if (data.empty() || token_storage == nullptr) return std::string_view();
  const auto& token = data[0].token;
  if (token.offset == SIZE_MAX || token.offset + token.size > token_storage->size()) return std::string_view();
  return std::string_view(*token_storage).substr(token.offset, token.size);
}
size_t system::command_block::line() const { return data.empty() ? 0 : data[0].token.line; }
size_t system::command_block::column() const { return data.empty() ? 0 : data[0].token.column; }
size_t system::command_block::args_count() const { return direct_child_count(data); }
size_t system::command_block::size() const { return data.size(); }
bool system::command_block::nullable() const {
  using kind = rpn_conversion_ctx::block_kind;
  return !data.empty() && (data[0].kind == kind::nullable_call || data[0].kind == kind::nullable_braced_call);
}
bool system::command_block::string_literal() const {
  return !data.empty() && data[0].kind == rpn_conversion_ctx::block_kind::string_literal;
}
bool system::command_block::braced_args() const {
  using kind = rpn_conversion_ctx::block_kind;
  return !data.empty() && (data[0].kind == kind::braced_call || data[0].kind == kind::nullable_braced_call);
}
bool system::command_block::empty() const { return data.empty(); }

}
