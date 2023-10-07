#include "core_interface.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    one_child_interface::one_child_interface(const interface* child) noexcept : child(child) {}
    one_child_interface::~one_child_interface() noexcept { if (child != nullptr) child->~interface(); }

    condition_interface::condition_interface(const interface* condition) noexcept : condition(condition) {}
    condition_interface::~condition_interface() noexcept { if (condition != nullptr) condition->~interface(); }

    scope_interface::scope_interface(const interface* scope) noexcept : scope(scope) {}
    scope_interface::~scope_interface() noexcept { if (scope != nullptr) scope->~interface(); }

    children_interface::children_interface(const interface* childs) noexcept : childs(childs) {}
    children_interface::~children_interface() noexcept {
      for (auto child = childs; child != nullptr; child = child->next) {
        child->~interface();
      }
    }

    additional_children_interface::additional_children_interface(const interface* additional_childs) noexcept
      : additional_childs(additional_childs) {}
    additional_children_interface::~additional_children_interface() noexcept {
      for (auto child = additional_childs; child != nullptr; child = child->next) {
        child->~interface();
      }
    }

    additional_child_interface::additional_child_interface(const interface* child) noexcept : additional(child) {}
    additional_child_interface::~additional_child_interface() noexcept { if (additional != nullptr) additional->~interface(); }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
