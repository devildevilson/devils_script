#include <doctest/doctest.h>
#include <string>
#include "devils_script/system.h"
#include "devils_script/container.h"

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE::DEVILS_SCRIPT_INNER_NAMESPACE;
#else
namespace ds = DEVILS_SCRIPT_OUTER_NAMESPACE;
#endif

// Golden disassembly tests for the compilation step. These PIN the current emitted command
// array (opcodes + args + branch targets) for small scripts. A small, targeted alternative
// to the end-to-end introspection golden net: when the codegen of a combinator changes,
// these are the first to move — re-baseline deliberately (run the script through
// `disassemble` and paste the new output) rather than tweaking by hand.

static std::string disasm_double(const char* script) {
  ds::system sys; sys.init_basic_functions(); sys.init_math();
  return ds::disassemble(sys.parse<double, void>(script));
}

static std::string disasm_bool(const char* script) {
  ds::system sys; sys.init_basic_functions(); sys.init_math();
  return ds::disassemble(sys.parse<bool, void>(script));
}

TEST_CASE("disassembly golden") {
  SUBCASE("equality (bool root folds the single operand as AND)") {
    const std::string expected =
      "  0: pushvalue 10\n"
      "  1: pushvalue 20\n"
      "  2: ==\n"
      "  3: condjump_get -> 4\n"
      "  4: pushreturn\n";
    CHECK_EQ(disasm_bool("10 == 20"), expected);
  }

  SUBCASE("sum") {
    const std::string expected =
      "  0: pushvalue 5\n"
      "  1: pushvalue 5\n"
      "  2: + 9223372036854775807\n"
      "  3: pushreturn\n";
    CHECK_EQ(disasm_double("5 + 5"), expected);
  }

  // select = first clause whose `condition` holds; last (unconditional) clause is the else.
  SUBCASE("select") {
    const std::string expected =
      "  0: pushbool false\n"
      "  1: condjump_get -> 2\n"
      "  2: condjump -> 5\n"
      "  3: pushvalue 10\n"
      "  4: jump -> 12\n"
      "  5: pushbool true\n"
      "  6: condjump_get -> 7\n"
      "  7: condjump -> 10\n"
      "  8: pushvalue 20\n"
      "  9: jump -> 12\n"
      " 10: pushvalue 100\n"
      " 11: jump -> 12\n"
      " 12: pushreturn\n";
    CHECK_EQ(disasm_double("{ select = { { condition = false, 10 }, { condition = true, 20 }, { 100 } } }"), expected);
  }

  // sequence = run clause bodies while each `condition` holds, accumulating (here: sum).
  // The leading `pushvalue 0` is the fold identity (the "no clause ran" default) — this is
  // exactly the identity-vs-ignore_value behavior we are pinning before revisiting it.
  SUBCASE("sequence") {
    const std::string expected =
      "  0: pushvalue 0\n"
      "  1: pushbool true\n"
      "  2: condjump_get -> 3\n"
      "  3: condjump -> 16\n"
      "  4: pushvalue 5\n"
      "  5: sum\n"
      "  6: pushbool true\n"
      "  7: condjump_get -> 8\n"
      "  8: condjump -> 16\n"
      "  9: pushvalue 10\n"
      " 10: sum\n"
      " 11: pushbool false\n"
      " 12: condjump_get -> 13\n"
      " 13: condjump -> 16\n"
      " 14: pushvalue 15\n"
      " 15: sum\n"
      " 16: pushreturn\n";
    CHECK_EQ(disasm_double("{ sequence = { { condition = true, 5 }, { condition = true, 10 }, { condition = false, 15 } } }"), expected);
  }

  // value_or = first arg is the condition; pick the second on true, the third on false.
  SUBCASE("value_or") {
    const std::string expected =
      "  0: pushbool false\n"
      "  1: condjump -> 4\n"
      "  2: pushvalue 10\n"
      "  3: jump -> 5\n"
      "  4: pushvalue 20\n"
      "  5: pushreturn\n";
    CHECK_EQ(disasm_double("{ value_or = { false, 10, 20 } }"), expected);
  }
}
