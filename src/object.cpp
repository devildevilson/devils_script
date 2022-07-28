#include "object.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    const object ignore_value(init_ignore_value{});
    const object unresolved_value(init_unresolved_value{});

    constexpr bool func() {
      constexpr object ignore_value(init_ignore_value{});
      constexpr object unresolved_value(init_unresolved_value{});
      return ignore_value.ignore() && unresolved_value.unresolved() && !ignore_value.unresolved() && !unresolved_value.ignore();
    }

    static_assert(func());
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
