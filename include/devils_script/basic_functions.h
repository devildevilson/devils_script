#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <climits>

// Runtime opcode declarations and compact argument packing helpers.
//
// `devils_script` compiles source text into a flat array of `container::command` entries.
// Each command calls one of these functions with a single 64-bit immediate argument, the
// current execution context, and the owning container. Safety-aware opcodes have paired
// safe/unsafe implementations; the unsafe variants skip stack type checks but keep the same
// stack contract.
//
// Several instructions need two indexes or a string span in that single immediate argument.
// The pack/unpack helpers below define that ABI and are shared by code generation,
// execution, and disassembly.

namespace devils_script {

struct context;
struct script_container;
struct container;

namespace internal {
struct thisarg { context* ctx; const script_container* scr; bool valid() const noexcept { return ctx != nullptr && scr != nullptr; } };
struct thisctx { context* ctx; bool valid() const noexcept { return ctx != nullptr; } };
struct thisctxlist { context* ctx; size_t idx; bool valid() const noexcept { return ctx != nullptr; } };
}

// A string id splits into the offset and size of a byte range inside the container string pool.
constexpr size_t packed_pos_bit_size = 28;
constexpr size_t packed_size_bit_size = 28;
static_assert(packed_pos_bit_size + packed_size_bit_size <= sizeof(int64_t) * CHAR_BIT);
int64_t packstrid(const uint32_t pos, const uint32_t size) noexcept;
std::tuple<uint32_t, uint32_t> unpackstrid(const int64_t value) noexcept;
int64_t pack2(const int32_t val1, const int32_t val2) noexcept;
std::tuple<int32_t, int32_t> unpack2(const int64_t val) noexcept;
bool check_value(const size_t val, const size_t bits) noexcept;
bool check_value(const int64_t val, const size_t bits) noexcept;

int64_t andjump(int64_t, context*, const script_container*);
int64_t orjump(int64_t, context*, const script_container*);
int64_t condjump(int64_t, context*, const script_container*);
int64_t condjump_get(int64_t, context*, const script_container*);
int64_t condjumpt_get(int64_t, context*, const script_container*);
int64_t andjump_unsafe(int64_t, context*, const script_container*);
int64_t orjump_unsafe(int64_t, context*, const script_container*);
int64_t condjump_unsafe(int64_t, context*, const script_container*);
int64_t condjump_get_unsafe(int64_t, context*, const script_container*);
int64_t condjumpt_get_unsafe(int64_t, context*, const script_container*);
int64_t jump(int64_t, context*, const script_container*);
int64_t jumpinvalid(int64_t, context*, const script_container*);

int64_t andbin(int64_t, context*, const script_container*);

int64_t invb(int64_t, context*, const script_container*);
int64_t sum(int64_t, context*, const script_container*);
int64_t mul(int64_t, context*, const script_container*);
int64_t neg(int64_t, context*, const script_container*);
int64_t pos(int64_t, context*, const script_container*);
int64_t invd(int64_t, context*, const script_container*);

int64_t cmpeq2(int64_t, context*, const script_container*);
int64_t cmplessd2(int64_t, context*, const script_container*);
int64_t cmplesseqd2(int64_t, context*, const script_container*);
int64_t sumsetstack(int64_t, context*, const script_container*);
int64_t mulsetstack(int64_t, context*, const script_container*);

int64_t andbin_unsafe(int64_t, context*, const script_container*);
int64_t invb_unsafe(int64_t, context*, const script_container*);
int64_t sum_unsafe(int64_t, context*, const script_container*);
int64_t mul_unsafe(int64_t, context*, const script_container*);
int64_t neg_unsafe(int64_t, context*, const script_container*);
int64_t pos_unsafe(int64_t, context*, const script_container*);
int64_t invd_unsafe(int64_t, context*, const script_container*);
int64_t cmpeq2_unsafe(int64_t, context*, const script_container*);
int64_t cmplessd2_unsafe(int64_t, context*, const script_container*);
int64_t cmplesseqd2_unsafe(int64_t, context*, const script_container*);
int64_t sumsetstack_unsafe(int64_t, context*, const script_container*);
int64_t mulsetstack_unsafe(int64_t, context*, const script_container*);

int64_t pushbool(int64_t, context*, const script_container*);
int64_t pushvalue(int64_t, context*, const script_container*);
int64_t pushint(int64_t, context*, const script_container*);
int64_t pushstring(int64_t, context*, const script_container*);

int64_t pushroot(int64_t, context*, const script_container*);
int64_t pushthis(int64_t, context*, const script_container*);
int64_t pushprev(int64_t, context*, const script_container*);
int64_t pushreturn(int64_t, context*, const script_container*);
int64_t pusharg(int64_t, context*, const script_container*);
int64_t pushinvalid(int64_t, context*, const script_container*);
int64_t erase(int64_t, context*, const script_container*);
// Fused scope unwind: erases `count` stack slots starting at `first` (packed as {first, count})
// with a single shift, replacing a run of descending `erase` opcodes.
int64_t erase_range(int64_t, context*, const script_container*);
int64_t pushcurrent(int64_t, context*, const script_container*);
int64_t pushchance(int64_t, context*, const script_container*);

int64_t pushargcontext(int64_t, context*, const script_container*);
int64_t pushcontext(int64_t, context*, const script_container*);

int64_t pushargvalue(int64_t, context*, const script_container*);
int64_t setargrvalue(int64_t, context*, const script_container*);
int64_t setarglvalue(int64_t, context*, const script_container*);
int64_t pushctxvalue(int64_t, context*, const script_container*);
int64_t savectxrvalue(int64_t, context*, const script_container*);
int64_t savectxlvalue(int64_t, context*, const script_container*);

int64_t pushlist(int64_t, context*, const script_container*);
int64_t list_pipeline(int64_t, context*, const script_container*);
// Placeholder for the immediate-data cmd slots that follow a list_pipeline opcode (its packed
// ranges/input-type). Never executed — list_pipeline reads them as data and skips past them.
int64_t list_op_data(int64_t, context*, const script_container*);
// Pushes a command name (packed string-pool ref) onto the stack ahead of an effect call so its
// on_effect callback can read the name as a normal stack value (no command_names side table).
int64_t push_command_name(int64_t, context*, const script_container*);

}
