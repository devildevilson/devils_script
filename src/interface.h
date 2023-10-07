#ifndef DEVILS_ENGINE_SCRIPT_INTERFACE_H
#define DEVILS_ENGINE_SCRIPT_INTERFACE_H

#include "object.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    struct context;
    struct local_state;
    class local_state_allocator;
    class interface;

    // возможно было бы неплохо иметь что то такое для каждого скрипта
    struct script_data {
      const interface* begin;
      size_t max_locals;
      size_t count;
      size_t size;
      std::vector<std::string> locals;
    };

    class interface {
    public:
      inline interface() noexcept : next(nullptr) {}
      virtual ~interface() noexcept = default;
      virtual struct object process(context* ctx) const = 0;
      virtual local_state* compute(context* ctx, local_state_allocator* allocator) const = 0;

      const interface* next;
      void dirty_set_next(const interface* n) const { ( const_cast<interface*>(this) )->next = n; }
    };
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
