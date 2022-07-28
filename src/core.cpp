#include "core.h"

#define MAKE_MAP_PAIR(name) std::make_pair(names[values::name], values::name)

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    namespace script_types {
      const std::string_view names[] = {
#define SCRIPT_TYPE_FUNC(name) #name,
        SCRIPT_TYPES_LIST
#undef SCRIPT_TYPE_FUNC
      };

      const phmap::flat_hash_map<std::string_view, values> map = {
#define SCRIPT_TYPE_FUNC(name) MAKE_MAP_PAIR(name),
        SCRIPT_TYPES_LIST
#undef SCRIPT_TYPE_FUNC
      };
    }

    namespace commands {
      const std::string_view names[] = {
#define COMMAND_NAME_FUNC(name) #name,

#define LOGIC_BLOCK_COMMAND_FUNC(name) COMMAND_NAME_FUNC(name)
#define NUMERIC_COMMAND_BLOCK_FUNC(name) COMMAND_NAME_FUNC(name)
#define COMMON_COMMAND_FUNC(name) COMMAND_NAME_FUNC(name)

        SCRIPT_COMMANDS_LIST

#undef LOGIC_BLOCK_COMMAND_FUNC
#undef NUMERIC_COMMAND_BLOCK_FUNC
#undef COMMON_COMMAND_FUNC

#undef COMMAND_NAME_FUNC
      };

      const phmap::flat_hash_map<std::string_view, values> map = {
#define COMMAND_NAME_FUNC(name) MAKE_MAP_PAIR(name),

#define LOGIC_BLOCK_COMMAND_FUNC(name) COMMAND_NAME_FUNC(name)
#define NUMERIC_COMMAND_BLOCK_FUNC(name) COMMAND_NAME_FUNC(name)
#define COMMON_COMMAND_FUNC(name) COMMAND_NAME_FUNC(name)

        SCRIPT_COMMANDS_LIST

#undef LOGIC_BLOCK_COMMAND_FUNC
#undef NUMERIC_COMMAND_BLOCK_FUNC
#undef COMMON_COMMAND_FUNC

#undef COMMAND_NAME_FUNC

      };

      static_assert(sizeof(names) / sizeof(names[0]) == count);
    }

    namespace ignore_keys {
      const std::string_view names[] = {
#define SCRIPT_IGNORE_KEY_FUNC(name) #name,
        SCRIPT_IGNORE_KEY_VALUES
#undef SCRIPT_IGNORE_KEY_FUNC
        "count"
      };

      const phmap::flat_hash_map<std::string_view, values> map = {
#define SCRIPT_IGNORE_KEY_FUNC(name) MAKE_MAP_PAIR(name),
        SCRIPT_IGNORE_KEY_VALUES
#undef SCRIPT_IGNORE_KEY_FUNC
        std::make_pair(names[_count], _count)
      };

      static_assert(sizeof(names) / sizeof(names[0]) == count);
    }

    namespace compare_operators {
      const std::string_view names[] = {
#define COMPARE_OPERATOR_FUNC(name) #name,
        COMPARE_OPERATORS_LIST
#undef COMPARE_OPERATOR_FUNC
      };

      const phmap::flat_hash_map<std::string_view, values> map = {
#define COMPARE_OPERATOR_FUNC(name) MAKE_MAP_PAIR(name),
        COMPARE_OPERATORS_LIST
#undef COMPARE_OPERATOR_FUNC
      };

      static_assert(sizeof(names) / sizeof(names[0]) == count);
    }

    namespace complex_variable_valid_string {
      const std::string_view names[] = {
#define COMPLEX_VARIABLE_VALID_STRING_FUNC(name) #name,
        COMPLEX_VARIABLE_VALID_STRINGS_LIST
#undef COMPLEX_VARIABLE_VALID_STRING_FUNC
      };

      const phmap::flat_hash_map<std::string_view, values> map = {
#define COMPLEX_VARIABLE_VALID_STRING_FUNC(name) MAKE_MAP_PAIR(name),
        COMPLEX_VARIABLE_VALID_STRINGS_LIST
#undef COMPLEX_VARIABLE_VALID_STRING_FUNC
      };

      static_assert(sizeof(names) / sizeof(names[0]) == count);
    }

    enum class uniquness_check {
#define COMMAND_NAME_FUNC(name) name,

#define LOGIC_BLOCK_COMMAND_FUNC(name) COMMAND_NAME_FUNC(name)
#define NUMERIC_COMMAND_BLOCK_FUNC(name) COMMAND_NAME_FUNC(name)
#define COMMON_COMMAND_FUNC(name) COMMAND_NAME_FUNC(name)

      SCRIPT_COMMANDS_LIST

#undef LOGIC_BLOCK_COMMAND_FUNC
#undef NUMERIC_COMMAND_BLOCK_FUNC
#undef COMMON_COMMAND_FUNC

#undef COMMAND_NAME_FUNC

#define SCRIPT_IGNORE_KEY_FUNC(name) name,
      SCRIPT_IGNORE_KEY_VALUES
#undef SCRIPT_IGNORE_KEY_FUNC
      _count,

#define COMMON_COMMAND_FUNC(name) name,
      ONLY_INIT_LIST
#undef COMMON_COMMAND_FUNC

      count
    };
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
