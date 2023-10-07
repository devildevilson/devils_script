#ifndef DEVILS_ENGINE_SCRIPT_LOGIC_COMMANDS_H
#define DEVILS_ENGINE_SCRIPT_LOGIC_COMMANDS_H

#include "logic_commands_macro.h"
#include "core_interface.h"
#include "object.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    struct context;

#define LOGIC_BLOCK_COMMAND_FUNC(func_name)               \
    class func_name final : public children_interface {   \
    public:                                               \
      static const size_t type_index;                     \
      func_name(const interface* childs) noexcept;        \
      ~func_name() noexcept;                              \
      struct object process(context* ctx) const override; \
      local_state* compute(context* ctx, local_state_allocator* allocator) const override; \
      /*void draw(context* ctx) const override;*/         \
      std::string_view get_name() const;                  \
    private:                                              \
      /*const interface* childs;*/                        \
    };                                                    \

    LOGIC_BLOCK_COMMANDS_LIST

#undef LOGIC_BLOCK_COMMAND_FUNC
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
