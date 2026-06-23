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

## Basic Usage

Register C++ functions in a `devils_script::system`, parse a script into a `container`,
then execute that container with a `context`.

```cpp
int func1(int a, int b) { return a + b; }

struct character {
  int strength;
  character* liege;
};

template <typename T>
struct handle {
  T* ptr;
  size_t type;

  T& operator*() const { return *ptr; }
  bool valid() const { return ptr != nullptr; }
};

handle<character> liege(handle<character> cur) { return handle<character>{cur.ptr->liege, cur.type}; }
int character_strength(handle<character> cur) { return cur.ptr->strength; }

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
- Script-in-script calls with `execute = { script_name, arg = value, ... }`; sub-scripts are
  resolved by name through a user-installed resolver (see *Script-in-script calls* below).
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

Current `execute` limitations: sub-scripts that use lists are rejected at parse (list storage is
sized per top-level script), and `describe()` does not special-case an `execute` node.

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

Two standalone benchmark targets (no test framework):

- `devils_script_benchs` — parse, execution, unsafe execution, and description traversal on the
  scripted scenario shared with the tests.
- `devils_script_execute_bench` — script-in-script (`execute`) call overhead: inline-vs-execute
  baselines, rooted calls, nesting depth, fan-out, and a switch-heavy sub-script.

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

- More real-world examples would help document intended patterns.
- `execute` does not yet support sub-scripts that use lists, or describe-aware `execute` nodes.
- Debugging/editor tooling is mostly exposed through primitives, not a finished tool.

## License

MIT
