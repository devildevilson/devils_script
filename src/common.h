#ifndef DEVILS_SCRIPT_COMMON_H
#define DEVILS_SCRIPT_COMMON_H

#include <cstddef>

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#define SCRIPT_SYSTEM_DEFAULT_GENERATOR_NAMESPACE utils::xoshiro256starstar

    struct context;

    void allocate_locals(context* ctx, const size_t add_count);

    struct allocate_additional_locals {
      context* ctx;
      size_t last_size;
      size_t last_offset;
      allocate_additional_locals(context* ctx, const size_t add_count) noexcept;
      ~allocate_additional_locals() noexcept;
    };
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif // DEVILS_ENGINE_SCRIPT_COMMON_H
