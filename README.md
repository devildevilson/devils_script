# devils_script

`devils_script` is a C++20 embeddable script system inspired by Paradox-style game scripts.
It is designed for low-overhead calls into registered C++ functions, typed scope navigation,
iterator blocks, and description/introspection of compiled script containers.

Dependencies:
- C++20 standard library
- [tavl](https://github.com/devildevilson/tavl) for parsing
- [doctest](https://github.com/doctest/doctest) for tests only

See `examples/` and `tests/` for complete usage examples.

## Building

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DDS_BUILD_TESTS=ON -DDS_BUILD_EXAMPLES=ON
cmake --build build-release --target devils_script_tests devils_script_benchs
./build-release/devils_script_tests
./build-release/devils_script_benchs
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

auto script = sys.parse<double, handle<character>>("liege:strength + func1(1, 2)");

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
- Debug helpers `assert` and `trace`.
- Safe and unsafe opcode variants; `system::toggle_safety()` disables stack safety checks
  for lower overhead after scripts are trusted.
- Copyable and movable compiled `container` objects.

## Description and Introspection

Compiled containers can be traversed without normal execution through `container::describe`.
Description entries expose node name, kind, nesting level, scope, value state, and partially
evaluated values where available. This supports tooling such as UI descriptions, previews,
debug views, and editor introspection.

`container::make_table` remains available for the older node-view style traversal; tests keep
it aligned with `describe`.

## Benchmarks

The standalone benchmark target is `devils_script_benchs`. It measures parse, execution,
unsafe execution, and description traversal on the same scripted scenario used by the tests.

```sh
cmake --build build-release --target devils_script_benchs
./build-release/devils_script_benchs
```

## Current Gaps

- More real-world examples would help document intended patterns.
- Error messages could still include richer source context.
- Debugging/editor tooling is mostly exposed through primitives, not a finished tool.

## License

MIT
