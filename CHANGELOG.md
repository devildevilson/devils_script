# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
