#ifndef DEVILS_ENGINE_SCRIPT_CORE2_H
#define DEVILS_ENGINE_SCRIPT_CORE2_H

#include <string_view>
#include "parallel_hashmap/phmap.h"
#include "interface.h"
#include "all_commands_macro.h"

#define SCRIPT_TYPES_LIST \
  SCRIPT_TYPE_FUNC(condition) \
  SCRIPT_TYPE_FUNC(numeric) \
  SCRIPT_TYPE_FUNC(string) \
  SCRIPT_TYPE_FUNC(object) \
  SCRIPT_TYPE_FUNC(effect) \

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    class container;

    namespace script_types {
      enum values {
#define SCRIPT_TYPE_FUNC(name) name,
        SCRIPT_TYPES_LIST
#undef SCRIPT_TYPE_FUNC
      };

      extern const std::string_view names[];
      extern const phmap::flat_hash_map<std::string_view, values> map;
    }

    namespace commands {
      enum values {
#define COMMAND_NAME_FUNC(name) name,

#define LOGIC_BLOCK_COMMAND_FUNC(name) COMMAND_NAME_FUNC(name)
#define NUMERIC_COMMAND_BLOCK_FUNC(name) COMMAND_NAME_FUNC(name)
#define COMMON_COMMAND_FUNC(name) COMMAND_NAME_FUNC(name)
        SCRIPT_COMMANDS_LIST
#undef LOGIC_BLOCK_COMMAND_FUNC
#undef NUMERIC_COMMAND_BLOCK_FUNC
#undef COMMON_COMMAND_FUNC

#undef COMMAND_NAME_FUNC

        count
      };

      extern const std::string_view names[];
      extern const phmap::flat_hash_map<std::string_view, values> map;
    }

    namespace ignore_keys {
      enum values {
#define SCRIPT_IGNORE_KEY_FUNC(name) name,
        SCRIPT_IGNORE_KEY_VALUES
#undef SCRIPT_IGNORE_KEY_FUNC
        _count,

        count
      };

      extern const std::string_view names[];
      extern const phmap::flat_hash_map<std::string_view, values> map;
    }

    namespace compare_operators {
      enum values : uint8_t {
#define COMPARE_OPERATOR_FUNC(name) name,
        COMPARE_OPERATORS_LIST
#undef COMPARE_OPERATOR_FUNC
        count
      };

      extern const std::string_view names[];
      extern const phmap::flat_hash_map<std::string_view, values> map;
    }

    namespace complex_variable_valid_string {
      enum values {
#define COMPLEX_VARIABLE_VALID_STRING_FUNC(name) name,
        COMPLEX_VARIABLE_VALID_STRINGS_LIST
#undef COMPLEX_VARIABLE_VALID_STRING_FUNC
        count
      };

      extern const std::string_view names[];
      extern const phmap::flat_hash_map<std::string_view, values> map;
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
