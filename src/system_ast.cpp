#include "devils_script/system.h"

#include <array>
#include <format>
#include <stdexcept>
#include "devils_script/string-utils.hpp"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

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

// Port of convert_block's per-token body, sourcing (lvalue, rhs) from a tavl row node.
// Same size/args bookkeeping; reuses convert_scope.
void system::rpn_conversion_ctx::normalize_row(const tavl::node* row, std::string_view src) {
  const size_t cur_size = output.size();
  std::string_view lfn = "__empty_lvalue";
  std::array<block, 16 * 3 + 1> arr;
  size_t lvalue_tokens_count = 0;

  std::string_view lvalue;
  const tavl::node* rhs = row;
  bool nullable_call = false;
  if (row->type == tavl::node_type::pair) {
    const auto op = node_text(row, src);
    if (op == "=" || op == "?=") {            // call/assignment: lhs = function, rhs = its argument
      const auto ch = node_children(row);
      if (ch.size() != 2 || ch[0]->type != tavl::node_type::token)
        throw std::runtime_error(std::format("Assignment operator '{}' expects a plain function name on the left side", op));
      lvalue = node_text(ch[0], src);          // lhs is a single token (scope paths arrive whole)
      rhs = ch[1];
      nullable_call = op == "?=";
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
    output[lfn_index].nullable = nullable_call;
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
    if (op == "=" || op == "?=")
      throw std::runtime_error(std::format("Assignment operator '{}' is not allowed inside expressions", op));

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
bool system::command_block::nullable() const { return !data.empty() && data[0].nullable; }
bool system::command_block::empty() const { return data.empty(); }

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
