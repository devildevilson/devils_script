# devils_script
Script system similar to Paradox games scripts (CK3, EU4). While working on this I have completely forgot how Paradox lang is looks like. Thus the comma separator was born

The only dependency is STL + Catch2 for tests

Look example folder and tests for examples

The language design specialy for:
- sequence of c++ functions call
- minimal call overhead
- multithreading support
- script description with names and values 

## Usage
Register any function with **devils_script**'s system's `register_function` call, you need provide function type and function pointer. The **devils_script** would try to find the function scope by its signature, but sometimes you need to help system and provide valid scope type. 
Scope type T can be anything that 
1) sizeof(T) <= 16
2) T is trivially destructable
See tests/case1.cpp for handle type example
For example:
```
int func1(int a, int b) { return a + b; }
handle<character> liege(handle<character> cur) { return cur->liege; }
int character::strength() { return this->strength; }
double each_soldier(army* a, std::function<bool(soldier*)> filter, std::function<double(soldier*)> fn) {
  double val = 0.0;
  for (auto sold : army->soldiers) {
	if (filter && !filter(sold)) continue;
	val += fn(sold);
  }
  return val;
}

// ...

devils_script::system sys;
sys.register_function<decltype(&func1), &func1>("func1"); // any scope
sys.register_function<decltype(&liege), &liege>("liege"); // scope is handle<character>
sys.register_function<decltype(&character::strength), &character::strength, handle<character>>("strength"); // tell script to use handle<character> as scope for this function
sys.register_function_iter<decltype(&each_soldier), &each_soldier>("each_soldier", { "filter", "value" }); // argument names is mandatory for iterators
```

**!!!Important** **devils_script** ignores constness of pointers 

## Main features
1. Standart math functions and operators
2. Standart language blocks: value_or, select, sequence, switch (not tested yet), random
3. Make your own operators thru `register_operator`
4. Toggle stack safety checks to better performance (actually does not provide outstanding boost, 10-15% faster I believe)
5. Script containers have descriptions for every commands in it + reverse AST tree to generating descriptions (see example/desc.cpp)
6. Script container fully copyable and movable
7. Script context consists of stack, stack has types for each values on it
8. Save and load any value in context
9. Script arguments can be used and script and overwritten with new value
10. I made everything I can to register any arbitrary functions you can imagine, but for some logic you may need to provide custom init function

## TODO:
1. different functions depending on scope (+) (tests?)
2. 'this' and 'prev' blocks fixes (+)
3. assert (?)
4. repair 'switch'
5. better callable for iterators (subblock in common.h ?)
6. another file for benchmark (+)
7. container::make_table does not work (+-) (how to get a value after iterator function call?)
8. better list design
9. More examples and tests (wanna return here after I try to use it somewhere)
10. benchmarks?
11. instruments for debugging
12. proper error descriptions - line numbers and context
13. ???

## License
MIT