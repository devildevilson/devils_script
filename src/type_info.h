#ifndef DEVILS_ENGINE_SCRIPT_TYPE_INFO_H
#define DEVILS_ENGINE_SCRIPT_TYPE_INFO_H

#include <type_traits>
#include <cstddef>
#include <atomic>
#include "type_traits.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    // remove_cv_t не убирает константность с указателя, что предпочтительно
    static_assert(std::is_same_v<std::remove_cv_t<std::remove_reference_t<int* const>>, int*>);
    static_assert(std::is_same_v<std::remove_cv_t<std::remove_reference_t<const int*>>, const int*>);

    template <typename T>
    constexpr size_t type_id() {
      using type = std::remove_reference_t<std::remove_cv_t<T>>; // std::remove_pointer_t
      const auto name = detail::get_type_name<type>();
      return murmur_hash64A(name, default_murmur_seed);
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
