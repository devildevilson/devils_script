# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2026-10-20

### Added

- Hexadecimal, octal and binary integer literals: `0x1f`, `0o17`, `0b101`, with the prefix letter in
  either case.
- `switch` default case: a last case without `value`. A value-producing `switch` without one that
  matches nothing is a runtime error instead of leaving no value on the stack.
- Integer overloads of `max`, `min` and `abs`; integer `sequence`; `sum`/`min`/`max`/`average` over an
  integer list (through the registered integer -> floating-point conversion).
- `context::max_execute_depth` (128): nested `execute`, including recursion through the resolver, is a
  runtime error past it instead of overflowing the C++ stack. `container::max_args` bounds nested
  argument frames at parse time.
- `std::optional<T>` parameters work: an absent argument (positional or named) is passed as
  `std::nullopt`, a present one is converted to `T`.
- Floating-point literals with a signed exponent: `1e-3`, `1.5e+3` (tavl v1.1.2 no longer splits
  them at a registered `+`/`-`).

### Changed

- `tavl` dependency updated from v1.0.3 to v1.1.2. Every block, the root included, is parsed in tavl's
  data-driven mode (`set_default_block_modes`): a devils_script `{}` is a list of expressions, so
  tavl's row-naming rules no longer apply and the AST builder alone judges each row. Consumers no
  longer build tavl's tests and command-line tool.
- A number is not a truth value: every child of an `AND`/`OR`/`NAND`/`NOR` block must be a `bool`. The
  first child used to reach the short-circuit jump unconverted, later ones were silently converted.
- `random` rejects a negative or NaN weight at run time; such weights used to make every branch fail
  and leave no value.
- A variadic function (`T f(T, T)`) called by name needs at least two arguments: `max(1)` used to
  return 1 without calling `max`, `max()` crashed. A function without parameters rejects arguments
  (`age = 5` used to drop the 5). An operator operand, and a call argument, must be values.
- A top-level `process()` starts at the first command on an empty stack, so a context can be reused
  without `clear()`. `clear()` also resets the saved-value types, so a read before the write fails
  again instead of returning the previous run's value. A script with lists run on a context without
  `create_lists()` is an error up front instead of an out-of-bounds access.
- A named saved value or argument keeps one type: storing a different type is a parse error unless it
  converts implicitly. `ctx_set_as` now names an argument slot (it looked up the saved-value table).
- An empty effect script, or `{}` in an effect block, does nothing; in a value context it is an error.
  Synthetic block names (`__string_block`, ...) no longer turn into strings.
- An expression nests at most 256 levels deep (`max_expression_depth`).
- Overload resolution converts arguments as late as possible: an argument conversion costs more than a
  result conversion. An integer expression in a floating-point block is now computed in integers and
  converted once, as the README promises and as the constant folder already did (`-7` used to pick the
  double `unary_minus`, `2 % 0` the double `%`).
- `chance` and `random`: every parsed script draws its fixed state from the system's own sequence
  (`system::next_script_state`, restarted by `reseed`), named or not; every script used to draw the same
  state from the system seed, so different scripts rolled the same values. Each callback call (iterator
  or list-pipeline body) mixes its own index into the context's state, so every element rolls anew; a
  run repeated in the same frame (`prng_state`) rolls the same values. Seeded values differ from 1.3.1.
- `switch` compares floating-point values within the same epsilon as `==` (it compared bytes).
- Error positions. A run-time error points at the failing call: a node's commands emitted before its
  nested block used to carry the nested block's position. A compile error now leads with
  `@ line:column` of the innermost node it happened in and, when no overload fits, with the candidate
  failure that got furthest into the source; the full candidate list follows in parentheses.
- `disassemble` names type conversions (`convert long -> double`) through `container::conversions`;
  they used to print without a name or with the name of a neighbouring node.
- `describe()` stops at the first command that cannot run, and never runs a void iterator.
- An overload trial no longer copies the script's token storage (copying a parse context gives an empty
  token scratch); that copy grew with the square of a long expression's length.

### Fixed

- Syntax errors reported by `tavl` now reject the script instead of being dropped by the AST
  builder. Unterminated strings and block comments, invalid escapes, ill-formed UTF-8, forbidden
  control characters, mismatched brackets, empty rows (`(a,,b)`), a call operator without a value
  (`f =`) and out-of-range floats previously compiled; an unterminated string even lost its last
  character. `system::parse(name, text)` throws `Syntax error @ line:column: ...`; the streaming
  overload reports the same message through the error callback and returns the `tavl` error for the
  row, keeping the stream usable for the following rows.
- The AST builder checks expressions itself. A missing operand (`5 + 3 *`, `dbl = -`) used to become
  an empty-token leaf, which a string context accepted as `""`; a token after a complete expression
  (`dbl = 5.0 6.0`) was dropped together with the rest of its block; `= x` reached overload
  resolution. Each is now a syntax error with its position.
- An integer literal that does not fit `script_int_t` is a syntax error. It used to compile as an
  inexact double wherever a double was acceptable.
- Crashes and memory errors: an effect child left a placeholder in the parse-time stack model, shifting
  every later stack index (wrong values, SIGSEGV); a value-less child was folded as an operand; a list
  changed inside its own pipeline (heap-use-after-free); runtime type tags pointing into a destroyed
  container's string pool (heap-use-after-free); `remove_from` of an absent value erased `end()`;
  `clamp` with crossed bounds (now a script error); empty scripts, `max()`, a bare `AND` (unbounded
  recursion); unsafe void functions left their arguments on the stack; a streamed script with a root
  scope crashed on its second row, and a rejected streamed row left its commands in the container.
- Wrong results: an object block whose last child was skipped (condition, `?=`) left no value;
  a scoped call inside an `execute`d script read the caller's stack slot; `a.b = x`
  miscounted block sizes (swallowing the next row); `ctx:saved:x.next` dropped `next`; `select` /
  `switch` / `random` branches were not converted to the block's type; conditions inside numeric
  blocks did not compile; `execute` truncated a double into an integer argument; `value_or` converted
  after its branches merged; `?=` on a scope path did not guard the path; a skipped `?=` in an integer
  block produced a double; a `map` after `add_to` failed the earlier `add_to`; folded `!=` and folded
  integers in floating-point blocks disagreed with the unfolded code; `condition` in an argument block
  was silently ignored; `(1, 2) * 3` spliced the group into its parent; `max(NaN, 1)` depended on the
  argument order.
- Types: `const T*` and `const bool` values; an untyped scope (an undeclared argument) can call scoped
  functions; an explicit `HT = void` keeps the first parameter in the signature; a member-function
  iterator compiles; an enum script result is readable as the enum; `f(...)` / `f{...}` compile like
  `f = {...}` (scope paths, `=` rows inside braces); malformed scope paths (`a.`, `a:`) are rejected.

## [1.3.1] - 2026-09-08

### Fixed

- Overload probing is now diagnostic-free: rejected candidates neither call the user error callback
  nor emit warnings. A final resolution failure reports all candidate reasons, including incompatible
  return and expected types, exactly once.

## [1.3.0] - 2026-09-08

### Added

- `DS_32BIT` build option: script numbers can use `int32_t` and `float` instead of `int64_t` and
  `double`. The setting propagates to consumers because it changes the public stack layout.
- Peephole optimization of compiled commands, enabled by default and switchable with
  `system::toggle_optimizations()`. It removes unused context pushes, combines consecutive stack
  erases, removes jumps to the next instruction, and threads jump chains.
- Integer `%` and the explicit `to_int` conversion.
- Runtime, compile-fail, architecture, and 32-bit configuration checks registered with CTest.
- Build and public header versions derived from the nearest `vMAJOR.MINOR.PATCH` Git tag. Untagged
  commits carry their distance and hash in `DEVILS_SCRIPT_VERSION`.

### Changed

- Integer literals remain integers regardless of the surrounding block. Integer arithmetic is exact
  and wraps in two's-complement form; comparisons no longer lose precision through `double`.
- `/` always produces a floating-point result. An integer-returning script must convert it explicitly
  with `to_int`.
- Mixed integer/floating-point operations widen the integer operand. Floating-point values no longer
  narrow implicitly to reach an integer overload, and overload ties prefer the candidate whose return
  type already matches the expected type.
- Script stack values must be trivially copyable and no larger than 16 bytes. Typed byte reads use
  `bit_cast`, and `stack_element::rawget<T>()` returns a value like `get<T>()`.
- Allocating context constructors may throw. Non-x86 targets now default to `DS_ARCH=OFF`.
- Direct `ctx:arg:`, `ctx:saved:`, and `ctx:list:` reads produce a smaller command stream. Their
  namespace node may consequently have no value/scope in description output.

### Fixed

- Stack bounds checks and indexed tail erasure no longer access or discard the wrong slots.
- Parser storage remains alive while overload candidates are resolved.
- Implicit conversion cost and emitted conversion commands now use the same shortest path.
- Constant folding preserves registered overloads, exact constant types, `-0.0`, and the behavior of
  the equivalent non-folded script. Added `system::is_builtin_function()` for this distinction.
- `describe()` never runs effects inside `execute`; unavailable results are reported as such.
- Exceptions during nested execution restore the previous script and all frame registers.
- `%` reports division by zero and `INT_MIN % -1` as script errors instead of trapping.

## [1.2.1] - 2026-07-23

### Changed

- Expanded the custom arithmetic type documentation with registration and mixed-operator examples.

### Fixed

- Unary `+` and `-` now select integer overloads for `int64_t` operands.
- Iterator registration correctly infers whether a free function's first argument is a scope or a
  script callback, and void callbacks no longer create a phantom stack value.
- Iterator blocks accepting void callbacks can contain ordinary effect commands.

## [1.2.0] - 2026-06-27

### Added

- Registration for custom arithmetic value types, implicit conversion edges, and overloaded named
  functions and operators.
- Per-type arithmetic block reducers and priorities used by `parse<T>()` and nested blocks.
- A standalone `vec4` arithmetic example covering same-type and mixed scalar/vector expressions.

### Changed

- Function and operator overloads are resolved by scope, argument types, conversion cost, and
  priority.
- Conditional children in arithmetic blocks are skipped while preserving the `ADD`/`MUL` identity.

### Removed

- Legacy `DEVILS_SCRIPT_OUTER_NAMESPACE` / `DEVILS_SCRIPT_INNER_NAMESPACE` wrapping; the public API
  now consistently uses the `devils_script` namespace.

## [1.1.1] - 2026-06-24

### Changed
- Build flags no longer leak onto consumers: `devils_script_options` is now linked `PRIVATE` to the
  library (arch, warnings, RTTI, LTO, MSVC runtime stay internal); only the C++20 requirement and
  `tavl` propagate via `PUBLIC`.
- Replaced the hand-written `/MD` / `/MDd` flags with a guarded `CMAKE_MSVC_RUNTIME_LIBRARY` default
  (DLL runtime), so an embedding project can choose the static runtime instead.
- Release archives are now built fat so they link with or without consumer LTO: GCC
  `-ffat-lto-objects`, MSVC `/GL` at compile paired with `/LTCG` at archive and link time.

### Added
- `DS_ARCH` cache variable (`OFF` / `AVX` / `AVX2` / `NATIVE`, default `AVX`) selecting the ISA
  baseline for devils_script's own sources, mapped to the right GCC/Clang/MSVC flags. Applied
  PRIVATE — no ABI impact, consumers pick their own arch.

## [1.1.0] - 2026-06-24

### Added
- `execute` in/out scalar/object arguments: when a call site passes a bare `ctx:saved:x` /
  `ctx:arg:x` lvalue, the sub-script's final value is written back into the caller's slot (a computed
  expression stays by-value).
- `execute` in/out list bindings: a sub-script's `ctx:list:name` can be bound to the caller's
  same-named list (`execute = { sub, name }`); mutations land back in the caller. Sub-scripts may also
  use their own lists (a frame stacked above the caller's).
- Parse-time peak-usage estimation on `script_container`: `max_stack`, `max_saved`, and `max_lists`
  — each accounting for `execute` nesting — so callers can size contexts or bucket scripts by cost.
- `context(stack_capacity, saved_capacity)` constructor for explicitly sizing the operand and
  saved-value stacks (e.g. from `max_stack` / `max_saved`). The default constructor is unchanged.
- Parse-time upper-bound check (`parse_context::max_stack_limit` / `max_saved_limit`) rejecting a
  script that needs more stack/saved slots than allowed.
- Runtime type check on saved-value reads (`ctx:saved:x`): reading a slot that was never written or
  holds the wrong type now throws, matching the existing check on argument reads.
- Upfront context-size check in `process()`: a too-small context fails immediately with a clear
  message instead of overflowing mid-run.

### Changed
- Eliminated the three remaining per-script side tables (`subscripts`, `list_pipeline_ops`,
  `command_names`) by packing their data into opcode arguments, immediate-data instruction slots, and
  the VM stack — smaller compiled containers.
- Frame-local VM registers (`arg_base` / `saved_base` / `list_base`, alongside `frame_base`) so a
  sub-script's arguments, saved values, and lists never clobber the caller's.
- Moved all non-template `inline` definitions out of the public headers into translation units (new
  `context.cpp` / `common.cpp`); dropped the now-redundant `inline` keyword from template functions.
- Normalized the whole repository to LF line endings and added `.gitattributes` to keep it that way.
- Link-time optimization now passes a per-compiler value at both compile and link: GCC `-flto=auto`,
  Clang `-flto=thin`.

### Fixed
- Crash / heap corruption when a list-pipeline callback (`filter` / `map` / a reducer) executed a
  list-using sub-script: growing `ctx->lists` reallocated the vector and dangled the reference the
  pipeline held across the call. `create_lists` now reserves `max_lists`, so it never reallocates.
- Reject (at parse time) binding the list a pipeline is currently iterating to an executed
  sub-script, which would let the sub mutate the live list mid-iteration.

## [1.0.0]

Initial release: compiled script containers, a stack VM with safe/unsafe opcode variants, block
forms, iterators, context values/arguments/lists, script-in-script `execute`, description and
introspection, a deterministic PRNG, and benchmarks.
