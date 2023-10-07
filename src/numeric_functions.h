#ifndef DEVILS_ENGINE_SCRIPT_NUMERIC_FUNCTIONS_H
#define DEVILS_ENGINE_SCRIPT_NUMERIC_FUNCTIONS_H

#include "numeric_commands_macro.h"
#include "core_interface.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    struct context;

    // нужно наверное при запуске process просто посчитать значения везде, а применять эффекты в отдельной функции
    // посчитанные данные легко пихнуть потом в драв, да ввод функции compute решит довольно много моих проблем
    // и драв после этого даже будет неплохо работать
    // отрисока может идти по данным полученным из compute, собственно основная проблема compute заключается только в том
    // что нужно очень много памяти для хранения полного стейта функции


#define NUMERIC_COMMAND_BLOCK_FUNC(func_name)                \
  class func_name final : public children_interface {        \
  public:                                                    \
    static const size_t type_index;                          \
    func_name(const interface* childs) noexcept;             \
    ~func_name() noexcept;                                   \
    struct object process(context* ctx) const override;      \
    local_state* compute(context* ctx, local_state_allocator* allocator) const override; \
    /*void draw(context* ctx) const override;*/              \
    std::string_view get_name() const;                       \
  private:                                                   \
    /*const interface* childs;*/                             \
  };                                                         \

    NUMERIC_COMMANDS_LIST2

#undef NUMERIC_COMMAND_BLOCK_FUNC

  }
#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
