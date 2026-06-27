#include <doctest/doctest.h>
#include <string>
#include "devils_script/system.h"
#include "devils_script/container.h"

namespace ds = devils_script;

// Golden disassembly tests for the compilation step. These PIN the current emitted command
// array (opcodes + args + branch targets) for small scripts. A small, targeted alternative
// to the end-to-end introspection golden net: when the codegen of a combinator changes,
// these are the first to move — re-baseline deliberately (run the script through
// `disassemble` and paste the new output) rather than tweaking by hand.

static std::string disasm_double(const char* script) {
  ds::system sys; sys.init_basic_functions(); sys.init_math();
  return ds::disassemble(sys.parse<double, void>("script", script));
}

static std::string disasm_bool(const char* script) {
  ds::system sys; sys.init_basic_functions(); sys.init_math();
  return ds::disassemble(sys.parse<bool, void>("script", script));
}

TEST_CASE("script AST row-level parsing") {
  SUBCASE("builder returns at row_end") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    tavl::parser p;
    sys.configure_parser(p);
    p.flush("5, 6");
    p.finish();

    ds::script_ast_context ctx;
    std::vector<tavl::node> nodes;

    auto [ev1, err1] = ds::make_script_ast(p, ctx, nodes);
    REQUIRE(err1.no_error());
    CHECK(ev1.type == tavl::event_type::row_end);
    REQUIRE(nodes.size() >= 2);
    CHECK(p.content(nodes[1].token.span) == "5");

    auto [ev2, err2] = ds::make_script_ast(p, ctx, nodes);
    REQUIRE(err2.no_error());
    CHECK(ev2.type == tavl::event_type::row_end);
    REQUIRE(nodes.size() >= 2);
    CHECK(p.content(nodes[1].token.span) == "6");
  }

  SUBCASE("partial parse stores script text for container get_string") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    tavl::parser p;
    sys.configure_parser(p);
    p.flush("red, 6");
    p.finish();

    ds::system::parse_context ctx;
    ds::container cont;
    ctx.init<std::string_view, void>(sys, cont);

    auto [ev, err] = sys.parse("script", p, ctx, cont);
    REQUIRE(err.no_error());
    CHECK(ev.type == tavl::event_type::row_end);
    // `source` is the compact token pool now, not the raw script text: it holds the script name
    // ("script", stored first) followed by the referenced token ("red"); get_string resolves
    // description names against it (checked below).
    CHECK(cont.get_name() == "script");
    CHECK(cont.string_pool == "scriptred");
    REQUIRE(!cont.block_descs.empty());
    bool found_red = false;
    for (const auto& desc : cont.block_descs) {
      found_red = found_red || cont.get_string(desc.name) == "red";
    }
    CHECK(found_red);

    ds::context runtime_ctx;
    cont.process(&runtime_ctx);
    REQUIRE(runtime_ctx.is_return<std::string_view>());
    CHECK(runtime_ctx.get_return<std::string_view>() == "red");
  }

  SUBCASE("partial parse checks configured return type") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    CHECK_THROWS(sys.parse<double, void>("script", "red"));

    tavl::parser p;
    sys.configure_parser(p);
    p.flush("red");
    p.finish();

    ds::system::parse_context ctx;
    ds::container cont;
    ctx.init<double, void>(sys, cont);

    auto [ev, err] = sys.parse("script", p, ctx, cont);
    CHECK(ev.type == tavl::event_type::row_end);
    CHECK_FALSE(err.no_error());
  }
}

TEST_CASE("disassembly golden") {
  SUBCASE("equality (bool root folds the single operand as AND)") {
    const std::string expected =
      "  0: pushbool false\n"
      "  1: pushreturn\n";
    CHECK_EQ(disasm_bool("10 == 20"), expected);
  }

  SUBCASE("sum") {
    const std::string expected =
      "  0: pushvalue 10\n"
      "  1: pushreturn\n";
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

  // A double `{a, b, c}` root folds through the registered unlimited "ADD" function (rawadd),
  // not the fold_block combinator. max() is not const-foldable, so the calls really emit.
  SUBCASE("ADD via unlimited function") {
    const std::string expected =
      "  0: pushvalue 1\n"
      "  1: pushvalue 2\n"
      "  2: max 9223372036854775807\n"
      "  3: pushvalue 3\n"
      "  4: pushvalue 4\n"
      "  5: max 9223372036854775807\n"
      "  6: ADD 9223372036854775807\n"
      "  7: pushvalue 5\n"
      "  8: pushvalue 6\n"
      "  9: max 9223372036854775807\n"
      " 10: ADD 9223372036854775807\n"
      " 11: pushreturn\n";
    CHECK_EQ(disasm_double("{ max(1,2), max(3,4), max(5,6) }"), expected);
  }

  // A bool `{x, y}` root folds through the fold_block AND combinator, which now emits the real
  // `andjump` opcode (Level-2: routed through emit()/insn_table) with short-circuit to the end.
  SUBCASE("AND combinator fold (andjump)") {
    const std::string expected =
      "  0: pushvalue 1\n"
      "  1: pushvalue 2\n"
      "  2: max 9223372036854775807\n"
      "  3: pushvalue 0\n"
      "  4: > 9223372036854775807\n"
      "  5: condjump_get -> 12\n"
      "  6: pushvalue 3\n"
      "  7: pushvalue 4\n"
      "  8: max 9223372036854775807\n"
      "  9: pushvalue 0\n"
      " 10: > 9223372036854775807\n"
      " 11: andjump -> 12\n"
      " 12: pushreturn\n";
    CHECK_EQ(disasm_bool("{ max(1,2) > 0, max(3,4) > 0 }"), expected);
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
