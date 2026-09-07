# devils_script

`devils_script` is a C++20 embeddable script system inspired by Paradox-style game scripts.
It is designed for low-overhead calls into registered C++ functions, typed scope navigation,
iterator blocks, script-in-script calls, and description/introspection of compiled script
containers. Compiled scripts split into a runtime-minimal `script_container` and a
description-carrying `container`, so production builds can ship the stripped form to cut memory.

Dependencies:
- C++20 standard library
- [tavl](https://github.com/devildevilson/tavl) for parsing
- [doctest](https://github.com/doctest/doctest) for tests only

See `examples/` and `tests/` for complete usage examples.

## Building

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DDS_BUILD_TESTS=ON -DDS_BUILD_EXAMPLES=ON
cmake --build build-release --target devils_script_tests devils_script_benchs devils_script_execute_bench
./build-release/devils_script_tests
./build-release/devils_script_benchs
./build-release/devils_script_execute_bench
```

For a debug build, use `-DCMAKE_BUILD_TYPE=Debug` and a separate build directory.

### Build options

The library's own compile flags (ISA, warnings, RTTI, LTO, MSVC runtime) are applied **privately** and
do not leak onto a consumer that links `devils_script` — only the C++20 requirement propagates.

| Option | Default | Meaning |
| --- | --- | --- |
| `DS_ARCH` | `AVX` | ISA baseline for devils_script's own sources: `OFF`, `AVX`, `AVX2`, `NATIVE`. No SIMD intrinsics live in the headers, so this has no ABI impact and consumers pick their own arch. |
| `DS_BUILD_TESTS` | `ON` | Build the doctest test suite. |
| `DS_BUILD_EXAMPLES` | `ON` | Build the examples. |

In release configs the static archive is built **fat** (GCC `-ffat-lto-objects`, MSVC `/GL` + `/LTCG`),
so it links into a consumer whether or not that consumer enables LTO. The MSVC C runtime defaults to the
DLL runtime via `CMAKE_MSVC_RUNTIME_LIBRARY`; set that variable to override (e.g. for the static runtime).

## Basic Usage

Register C++ functions in a `devils_script::system`, parse a script into a `container`,
then execute that container with a `context`.

```cpp
int32_t func1(int32_t a, int32_t b) { return a + b; }

struct character {
  int32_t strength;
  character* liege;
};

template <typename T>
struct handle {
  T* ptr;
  std::size_t type;

  T& operator*() const { return *ptr; }
  bool valid() const { return ptr != nullptr; }
};

handle<character> liege(handle<character> cur) { return handle<character>{cur.ptr->liege, cur.type}; }
int32_t character_strength(handle<character> cur) { return cur.ptr->strength; }

devils_script::system sys;
sys.init_basic_functions();
sys.init_math();

sys.register_function<&func1>("func1");
sys.register_function<&liege>("liege");
sys.register_function<&character_strength>("strength");

// parse<RETURN_T, ROOT_T>(script_name, source); the name is used in error messages.
auto script = sys.parse<double, handle<character>>("my_script", "liege:strength + func1(1, 2)");

devils_script::context ctx;
ctx.set_arg(0, handle<character>{root, 0});
script.process(&ctx);
double result = ctx.get_return<double>();
```

Scope types are expected to be small, trivially destructible values. The built-in validity
checks use common forms such as `valid()`, `is_valid()`, and `operator bool`; registration
can also provide a custom validity predicate.

`devils_script` intentionally ignores pointer constness when matching function signatures.

## Arithmetic Types

`init_math()` registers the default arithmetic model for `int64_t` and `double`. The parser keeps
those types distinct, resolves overloaded functions by signature, and may use implicit conversions
only along edges registered in the `system`. The built-in default conversion is `int64_t -> double`;
custom value types do not get casts automatically.

To make a custom value participate in arithmetic:

1. Mark it as a script arithmetic value type with `is_script_arithmetic_type<T>`.
2. Register the arithmetic block reducer with `register_arithmetic_type<T>(block_name, priority)`.
3. Register explicit implicit conversions, for example `double -> vec4`.
4. Register the overloads the language may call: `ADD`/`MUL` block reducers, named functions, and
   symbolic operators such as `+`, `-`, `*`, `/`.

```cpp
struct vec4 {
  float x;
  float y;
  float z;
  float w;

  explicit vec4(double v) : x(float(v)), y(float(v)), z(float(v)), w(float(v)) {}
};

namespace devils_script {
template <>
struct is_script_arithmetic_type<::vec4> : std::true_type {};
}

vec4 vec_add(vec4 a, vec4 b);
vec4 vec_sub(vec4 a, vec4 b);
vec4 vec_mul(vec4 a, vec4 b);
vec4 vec_div(vec4 a, vec4 b);
vec4 vec_mul_scalar(vec4 a, double b);
vec4 scalar_mul_vec(double a, vec4 b);

devils_script::system sys;
sys.init_basic_functions();
sys.init_math();

sys.register_arithmetic_type<vec4>("ADD", 30);
sys.register_implicit_conversion<double, vec4>();

sys.register_function<&vec_add, void>("ADD");
sys.register_function<&vec_mul, void>("MUL");

const devils_script::system::operator_props mul_props{
  12,
  devils_script::system::command_data::math_ftype::binary,
  devils_script::system::command_data::associativity::left
};
const devils_script::system::operator_props add_props{
  11,
  devils_script::system::command_data::math_ftype::binary,
  devils_script::system::command_data::associativity::left
};

sys.register_operator<&vec_mul, void>("*", mul_props);
sys.register_operator<&vec_div, void>("/", mul_props);
sys.register_operator<&vec_add, void>("+", add_props);
sys.register_operator<&vec_sub, void>("-", add_props);
sys.register_operator<&vec_mul_scalar, void>("*", mul_props);
sys.register_operator<&scalar_mul_vec, void>("*", mul_props);

auto script = sys.parse<vec4, void>("script", "a + scalar_b * c");
```

`HT=void` is intentional for free functions whose first argument is a value such as `vec4`; otherwise
the registration logic may treat the first argument as a scope type. The resolver does not invent
commutativity: if both `vec4 * double` and `double * vec4` are valid, register both overloads.

`register_arithmetic_type<T>("ADD", priority)` does not create the `ADD` function. It tells
`parse<T>()` and arithmetic blocks which reducer name to use when a block of values must collapse
to `T`, so that function must also be registered. `MUL` is registered separately because explicit
`MUL = { ... }` blocks and `*` operators use their own overloads.

Conditional arithmetic children are skipped without changing the fold identity: `ADD` uses `0`,
`MUL` uses `1`. For a custom arithmetic type this means a conditional `MUL` with no active value
requires an implicit conversion from the numeric identity to that type, usually `double -> T`.

See [examples/arithmetic_types.cpp](examples/arithmetic_types.cpp) for a complete standalone example
with scalar/vector overloads and explicit conversion rules.

## Script Features

- Typed function registration with automatic scope inference.
- Scoped calls with `:` and dotted scope paths, for example `country.leader:age`.
- Nullable scoped calls with `?=`, which skip invalid branches and return a typed default.
- Prefix, postfix, binary, and literal custom operators through `register_operator`.
- Arithmetic, comparison, boolean, trigonometric, and numeric builtins via `init_math()`.
- Block forms such as `value_or`, `select`, `sequence`, `switch`, and weighted `random`.
- Iterator registration with named subblocks through `register_function_iter`.
- Context values through `ctx_save`, `ctx_save_as`, and `ctx:saved:name`.
- Script arguments through `ctx:arg:name`, `ctx_set`, and `ctx_set_as`.
- Context lists with add/filter/map/first/count/min/max style pipelines.
- Static string tokens, quoted strings, enum literal parsing, and typed literal checks.
- Script-in-script calls with `execute = { script_name, arg = value, ... }`, including in/out scalar
  arguments and in/out list bindings; sub-scripts are resolved by name through a user-installed
  resolver (see *Script-in-script calls* below).
- Debug helpers `assert` and `trace`; runtime errors report `script '<name>' @ line:column`.
- Deterministic PRNG (`chance`, `random`, `rndmix`) seeded per `system` and per `context`.
- Safe and unsafe opcode variants; `system::toggle_safety()` disables stack safety checks
  for lower overhead after scripts are trusted.
- Copyable and movable compiled `container` objects, reusable across many `context` runs.

## Script-in-script calls

A script can invoke another, already-compiled script through `execute`. Sub-scripts are resolved
by name at parse time via a resolver installed on the `system`; the resolver hands back a
caller-owned `const script_container*` (its lifetime must outlive the caller).

```cpp
sys.set_script_resolver([&](std::string_view name) -> const devils_script::script_container* {
  auto it = registry.find(std::string(name));
  return it == registry.end() ? nullptr : it->second;
});

// addup is a separately compiled script: "ctx:arg:base + ctx:arg:bonus"
auto caller = sys.parse<double, void>("caller", "execute = { addup, base = 10, bonus = 5 }");
```

Named arguments are matched by name and type to the sub-script's declared `ctx:arg:` arguments
(with the usual numeric conversions). If the sub-script has a root scope it is fed implicitly from
the caller's current scope, type-checked against the caller. The return type is checked against the
caller's context. At runtime the call frame is just C++ locals plus a stack-base offset
(`context::frame_base`), so no stack copy is made and nesting composes.

A sub-script may use its own lists (their frame is stacked above the caller's) and may bind the
caller's lists in/out by name (`execute = { sub, mylist }`, matched to the sub's `ctx:list:mylist`).
A scalar/object argument is passed in/out — the sub's final value is written back into the caller's
slot — when the call site gives it a bare `ctx:saved:x` / `ctx:arg:x` lvalue; a computed expression
is by-value. A list-pipeline callback (`filter`/`map`/`sum`/…) may itself `execute` a list-using
sub-script, with one rule: it cannot bind the very list it is currently iterating (rejected at parse).

Current `execute` limitation: `describe()` does not yet special-case an `execute` node.

## Memory model and deployment

A freshly parsed script is a `container`: it carries the executable command stream **and** a
parallel description tree for tooling. For deployment you can drop the description data:

- `container::strip_description()` returns a runtime-minimal `script_container` (commands, source
  locations, argument/saved/list metadata, the deduplicated `string_pool`, and the script name —
  enough to `process()` and to report `script '<name>' @ line:column` on error).
- `shrink_to_fit(...)` (free function over a `span` of containers or script_containers) compacts
  the backing storage.
- `system::reserve_from_hint(container*, block_count, token_bytes)` pre-reserves the command and
  string buffers before codegen — useful when parsing many small scripts.
- `script_container::max_stack` / `max_saved` / `max_lists` record the peak operand-stack,
  saved-value, and list depth a script needs (accounting for `execute` nesting). Size a
  `context(stack_capacity, saved_capacity)` from them — or bucket scripts into nesting classes — and
  `process()` rejects an undersized context up front instead of overflowing mid-run.

This split is why most runtime-facing signatures take `const script_container*`: the engine runs
the stripped form, while `container` is only needed where descriptions are produced or consumed.

## Description and Introspection

Compiled containers can be traversed without normal execution through `container::describe`.
Description entries expose node name, kind, nesting level, scope, value state, and partially
evaluated values where available. This supports tooling such as UI descriptions, previews,
debug views, and editor introspection.

`container::make_table` remains available for the older node-view style traversal; tests keep
it aligned with `describe`.

## Benchmarks

Three standalone benchmark targets (no test framework):

- `devils_script_benchs` — parse, execution, unsafe execution, and description traversal on the
  scripted scenario shared with the tests. Description benchmarks compare the default context
  against one sized from `max_stack` / `max_saved` (`describe sized`). Pass `--script-function`
  to compare iterator callbacks using `script_function` instead of `std::function`.
- `devils_script_execute_bench` — script-in-script (`execute`) call overhead: inline-vs-execute
  baselines, rooted calls, nesting depth, fan-out, and a switch-heavy sub-script.
- `devils_script_process_bench` — execution/codegen microbenchmarks, hand-simplified equivalents,
  and two runtime-only opcode prototypes. Pass `--disassemble` to print the generated streams.
  See [PROCESS_OPTIMIZATION.md](PROCESS_OPTIMIZATION.md) for results and limitations.

```sh
cmake --build build-release --target devils_script_benchs devils_script_execute_bench
./build-release/devils_script_benchs
./build-release/devils_script_execute_bench
```

## Type-safety notes

In-script typing is **nominal**, keyed on the C++ `type_name<T>()` of registered signatures. In
safe mode every call re-validates each argument and the scope against the expected type and throws
on a mismatch; unsafe mode skips those checks. The model cannot see *past* a C++ type: if many
script-domain types share one C++ type (e.g. a single `handle<entity>` with a runtime tag), the
engine treats them as one type and will not catch feeding the wrong entity to a function. Prefer a
distinct C++ type per script-domain type so safe mode can enforce the distinction.

## Current Gaps

See [AUDIT.md](AUDIT.md) for the 2026-09-07 correctness audit, remaining defects,
validation results, and measured optimization priorities.

- More real-world examples would help document intended patterns.
- `describe()` does not special-case `execute` nodes (script-in-script is otherwise fully supported,
  including in/out arguments and lists).
- Debugging/editor tooling is mostly exposed through primitives, not a finished tool.

## License

MIT
