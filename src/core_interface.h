#ifndef DEVILS_ENGINE_SCRIPT_CORE_INTERFACE_H
#define DEVILS_ENGINE_SCRIPT_CORE_INTERFACE_H

#include "interface.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {

    class one_child_interface : virtual public interface {
    public:
      explicit one_child_interface(const interface* child) noexcept;
      virtual ~one_child_interface() noexcept;
    protected:
      const interface* child;
    };

    class additional_child_interface : virtual public interface {
    public:
      explicit additional_child_interface(const interface* child) noexcept;
      virtual ~additional_child_interface() noexcept;
    protected:
      const interface* additional;
    };

    class condition_interface : virtual public interface {
    public:
      explicit condition_interface(const interface* condition) noexcept;
      virtual ~condition_interface() noexcept;
    protected:
      const interface* condition;
    };

    class scope_interface : virtual public interface {
    public:
      explicit scope_interface(const interface* condition) noexcept;
      virtual ~scope_interface() noexcept;
    protected:
      const interface* scope;
    };

    class children_interface : virtual public interface {
    public:
      explicit children_interface(const interface* childs) noexcept;
      virtual ~children_interface() noexcept;
    protected:
      const interface* childs;
    };

    class additional_children_interface : virtual public interface {
    public:
      explicit additional_children_interface(const interface* childs) noexcept;
      virtual ~additional_children_interface() noexcept;
    protected:
      const interface* additional_childs;
    };
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif // CORE_INTERFACE_H
