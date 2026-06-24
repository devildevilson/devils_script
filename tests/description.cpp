#include "test_helpers.h"

TEST_CASE("Script description evaluation") {
  using kind = ds::container::description_node_kind;
  using state = ds::container::description_value_state;

  SUBCASE("folded value is visible") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    const auto cont = sys.parse<double, void>("script", "5 + 5");
    ds::context ctx;
    bool found_value = false;

    cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      if (entry.kind == kind::block && entry.state == state::value && entry.value.is<double>()) {
        found_value = found_value || entry.value.get<double>() == 10.0;
      }
    });

    CHECK(found_value);
  }

  SUBCASE("invalid object scope does not stop description traversal") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_id_is>("object_id_is");

    const auto cont = sys.parse<bool, object_ref>("script", "object_id_is = { 11 }");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 0 });

    size_t nodes = 0;
    bool found_unavailable = false;
    CHECK_NOTHROW(cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      nodes += 1;
      found_unavailable = found_unavailable || entry.state == state::unavailable;
    }));

    CHECK(nodes > 0);
    CHECK(found_unavailable);
  }

  SUBCASE("iterator children are placeholders") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function_iter<&func6>("func6", { "value" });

    const auto cont = sys.parse<double, scope2>("script", "{ func6 = { value = 5 + 5 } }");
    CHECK(cont.description_cmd_index_offsets.size() == cont.cmds.size() + 1);
    CHECK(cont.description_cmd_index_nodes.size() > 0);

    ds::context ctx;
    ctx.set_arg(0, scope2{});

    bool iterator_value = false;
    bool child_placeholder = false;
    cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      if (entry.name == "func6" && entry.kind == kind::iterator && entry.state == state::value && entry.value.is<double>())
        iterator_value = entry.value.get<double>() == 10.0;
      if (entry.kind == kind::argument && entry.state == state::placeholder)
        child_placeholder = true;
    });

    CHECK(iterator_value);
    CHECK(child_placeholder);
  }

  SUBCASE("node kinds cover common script forms") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_id_is>("object_id_is");
    sys.register_function<&runtime_num>("runtime_num");

    {
      const auto cont = sys.parse<double, void>("script", "runtime_num + runtime_num");
      ds::context ctx;

      bool saw_function = false;
      bool saw_operator = false;
      cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
        saw_function = saw_function || (entry.name == "runtime_num" && entry.kind == kind::function && entry.state == state::value && entry.value.is<double>());
        saw_operator = saw_operator || (entry.name == "+" && entry.kind == kind::operator_t);
      });

      CHECK(saw_function);
      CHECK(saw_operator);
    }

    {
      const auto cont = sys.parse<std::string_view, void>("script", "red");
      ds::context ctx;
      bool saw_literal = false;
      cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
        saw_literal = saw_literal || (entry.name == "red" && entry.kind == kind::literal && entry.state == state::value && entry.value.is<std::string_view>() && entry.value.get<std::string_view>() == "red");
      });
      CHECK(saw_literal);
    }
  }

  SUBCASE("effect nodes are described but not evaluated") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&object_effect>("object_effect");

    const auto cont = sys.parse<void, object_ref>("script", "object_effect");
    ds::context ctx;
    ctx.set_arg(0, object_ref{ 1 });

    bool saw_effect = false;
    CHECK_NOTHROW(cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      saw_effect = saw_effect || (entry.name == "object_effect" && entry.kind == kind::effect && entry.state == state::unavailable);
    }));
    CHECK(saw_effect);
  }

  SUBCASE("custom_description folds described children") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();
    sys.register_function<&runtime_num>("runtime_num");

    const auto cont = sys.parse<double, void>("script", "{ custom_description = summary, runtime_num, 5 }");
    ds::context ctx;
    size_t nodes = 0;
    bool saw_summary = false;
    bool saw_runtime_num = false;

    cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
      nodes += 1;
      saw_summary = saw_summary || entry.custom_description == "summary";
      saw_runtime_num = saw_runtime_num || entry.name == "runtime_num";
    });

    CHECK(saw_summary);
    CHECK_FALSE(saw_runtime_num);
    CHECK(nodes == 1);
  }

  SUBCASE("custom_description accepts only static string tokens") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    {
      sys.register_function<&runtime_num>("runtime_num");
      const auto cont = sys.parse<double, void>("script", "{ custom_description = 'abc abc', runtime_num }");
      ds::context ctx;
      bool saw_desc = false;
      cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
        saw_desc = saw_desc || entry.custom_description == "abc abc";
      });
      CHECK(saw_desc);
      cont.process(&ctx);
      REQUIRE(ctx.is_return<double>());
      CHECK(ctx.get_return<double>() == 3.0);
    }

    {
      const auto cont = sys.parse<double, void>("script", "{ custom_description = abc.def.123, 5 }");
      ds::context ctx;
      bool saw_desc = false;
      cont.describe(&ctx, [&](const ds::container::description_entry& entry) {
        saw_desc = saw_desc || entry.custom_description == "abc.def.123";
      });
      CHECK(saw_desc);
    }

    CHECK_THROWS(sys.parse<double, void>("script", "{ custom_description = { 5 }, 5 }"));
    CHECK_THROWS(sys.parse<double, void>("script", "{ custom_description = {}, 5 }"));
  }
}
TEST_CASE("Debug assert and trace") {
  SUBCASE("assert fails loudly with message and environment") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    const auto cont = sys.parse<void, void>("script", "assert = { false, failed_check }");
    {
      ds::context ctx;
      CHECK_THROWS_WITH_AS(cont.process(&ctx), doctest::Contains("failed_check"), std::runtime_error);
    }
    {
      ds::context ctx;
      CHECK_THROWS_WITH_AS(cont.process(&ctx), doctest::Contains("@ 1:1"), std::runtime_error);
    }
  }

  SUBCASE("trace sends formatted message to callback") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    const auto cont = sys.parse<void, void>("script", "trace = \"123 123\"");
    ds::context ctx;
    std::string out;
    ctx.trace = [&](const std::string& msg) { out = msg; };
    cont.process(&ctx);
    CHECK(out.find("123 123") != std::string::npos);
    CHECK(out.find("@ 1:1") != std::string::npos);
  }

  SUBCASE("trace accepts dotted static tokens and rejects script blocks") {
    ds::system sys;
    sys.init_basic_functions();
    sys.init_math();

    const auto cont = sys.parse<void, void>("script", "trace = abc.def.123");
    ds::context ctx;
    std::string out;
    ctx.trace = [&](const std::string& msg) { out = msg; };
    cont.process(&ctx);
    CHECK(out.find("abc.def.123") != std::string::npos);

    CHECK_THROWS(sys.parse<void, void>("script", "trace = { 5 + 5 }"));
  }
}
TEST_CASE("Source locations (line/column)") {
  ds::system sys;
  sys.init_basic_functions();
  sys.init_math();

  SUBCASE("assert error carries the line and column of the assert call") {
    // assert sits on line 2, indented by two spaces -> column 3.
    const auto cont = sys.parse<void, void>("script",
      "trace = \"a\"\n"
      "  assert = { false, boom }");
    ds::context ctx;
    ctx.trace = [](const std::string&) {};  // silence the line-1 trace
    std::string what;
    try { cont.process(&ctx); }
    catch (const std::runtime_error& e) { what = e.what(); }
    CHECK(what.find("script 'script' @ 2:3:") != std::string::npos);
    CHECK(what.find("boom") != std::string::npos);
  }

  SUBCASE("trace message embeds the line and column of the trace call") {
    // blank line 1, then trace indented by three spaces on line 2 -> column 4.
    const auto cont = sys.parse<void, void>("script", "\n   trace = \"hello\"");
    ds::context ctx;
    std::string out;
    ctx.trace = [&](const std::string& msg) { out = msg; };
    cont.process(&ctx);
    CHECK(out.find("@ 2:4:") != std::string::npos);
    CHECK(out.find("hello") != std::string::npos);
  }

  SUBCASE("locs is 1:1 with cmds and every command has a populated position") {
    const auto cont = sys.parse<void, void>("script",
      "assert = { false, boom }\n"
      "trace = \"hi\"");
    REQUIRE(cont.locs.size() == cont.cmds.size());
    for (const auto& loc : cont.locs) {
      CHECK(loc.line >= 1);     // never the degraded 0 placeholder
      CHECK(loc.column >= 1);
    }
  }

  SUBCASE("each command's loc points at its own source position") {
    // false literal on line 1 col 12; assert call on line 1 col 1; trace on line 2 col 1.
    const auto cont = sys.parse<void, void>("script",
      "assert = { false, boom }\n"
      "trace = \"hi\"");

    bool saw_false = false, saw_assert = false, saw_trace = false;
    for (size_t i = 0; i < cont.cmds.size(); ++i) {
      const auto name = cont.get_command_name(i);
      if (name == "pushbool") {
        saw_false = true;
        CHECK(cont.locs[i].line == 1);
        CHECK(cont.locs[i].column == 12);
      } else if (name == "assert") {
        saw_assert = true;
        CHECK(cont.locs[i].line == 1);
        CHECK(cont.locs[i].column == 1);
      } else if (name == "trace") {
        saw_trace = true;
        CHECK(cont.locs[i].line == 2);
        CHECK(cont.locs[i].column == 1);
      }
    }
    CHECK(saw_false);
    CHECK(saw_assert);
    CHECK(saw_trace);
  }
}
