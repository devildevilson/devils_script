#include "logic_commands.h"

#include "core.h"
#include "context.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    static object compute_AND_func(const local_state* s) {
      bool all_ignore = true;
      bool value = true;
      for (auto cur = s->children; cur != nullptr && value; cur = cur->next) {
        const auto &obj = cur->value;
        all_ignore = all_ignore && obj.ignore();
        value = obj.ignore() ? value : value && obj.get<bool>();
      }

      return all_ignore ? ignore_value : object(value);
    }

    static object compute_OR_func(const local_state* s) {
      bool all_ignore = true;
      bool value = false;
      for (auto cur = s->children; cur != nullptr && !value; cur = cur->next) {
        const auto &obj = cur->value;
        all_ignore = all_ignore && obj.ignore();
        value = obj.ignore() ? value : value || obj.get<bool>();
      }

      return all_ignore ? ignore_value : object(value);
    }

    static object compute_NAND_func(const local_state* s) {
      bool all_ignore = true;
      bool value = true;
      for (auto cur = s->children; cur != nullptr && value; cur = cur->next) {
        const auto &obj = cur->value;
        all_ignore = all_ignore && obj.ignore();
        value = obj.ignore() ? value : value && obj.get<bool>();
      }

      return all_ignore ? ignore_value : object(!value);
    }

    static object compute_NOR_func(const local_state* s) {
      bool all_ignore = true;
      bool value = false;
      for (auto cur = s->children; cur != nullptr && !value; cur = cur->next) {
        const auto &obj = cur->value;
        all_ignore = all_ignore && obj.ignore();
        value = obj.ignore() ? value : value || obj.get<bool>();
      }

      return all_ignore ? ignore_value : object(!value);
    }

    static std::tuple<object, const local_state*> get_first(const local_state* s) {
      object obj = ignore_value;
      auto cur = s->children;
      for (; cur != nullptr && obj.ignore(); cur = cur->next) { obj = s->value; }
      return std::make_tuple(obj, cur);
    }

    static object compute_XOR_func(const local_state* s) {
      const auto [val, start] = get_first(s);
      if (val.ignore()) return ignore_value;

      bool value = val.get<bool>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        value = obj.ignore() ? value : value != obj.get<bool>();
      }

      return object(value);
    }

    static object compute_IMPL_func(const local_state* s) {
      const auto [val, start] = get_first(s);
      if (val.ignore()) return ignore_value;

      bool value = val.get<bool>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        value = obj.ignore() ? value : value <= obj.get<bool>();
      }

      return object(value);
    }

    static object compute_EQ_func(const local_state* s) {
      const auto [val, start] = get_first(s);
      if (val.ignore()) return ignore_value;

      bool value = val.get<bool>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        value = obj.ignore() ? value : value == obj.get<bool>();
      }

      return object(value);
    }

    static object compute_AND_sequence_func(const local_state* s) {
      bool value = true;
      object obj(true);
      for (auto cur = s->children; cur != nullptr && value && !obj.ignore(); cur = cur->next) {
        value = value && obj.get<bool>();
        obj = cur->value;
      }

      if (!obj.ignore()) value = value && obj.get<bool>(); // последний

      return object(value);
    }

    static object compute_OR_sequence_func(const local_state* s) {
      bool value = false;
      object obj(false);
      for (auto cur = s->children; cur != nullptr && !value && !obj.ignore(); cur = cur->next) {
        value = value || obj.get<bool>();
        obj = cur->value;
      }

      if (!obj.ignore()) value = value || obj.get<bool>(); // последний

      return object(value);
    }

//     void func_name::draw(context* ctx) const {                    \
//       {                                                           \
//         const auto obj = process(ctx);                            \
//         local_state dd(ctx);                                      \
//         dd.function_name = get_name();                            \
//         dd.value = obj;                                           \
//         ctx->draw(&dd);                                           \
//       }                                                           \
//       change_nesting cn(ctx, ctx->nest_level+1);                  \
//       change_function_name cfn(ctx, get_name());                  \
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {  \
//         cur->draw(ctx);                                           \
//       }                                                           \
//     }                                                             \
//

#define LOGIC_BLOCK_COMMAND_FUNC(func_name)                       \
    const size_t func_name::type_index = commands::values::func_name; \
    func_name::func_name(const interface* childs) noexcept : childs(childs) {} \
    func_name::~func_name() noexcept {                            \
      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); } \
    }                                                             \
    local_state* func_name::compute(context* ctx, local_state_allocator* allocator) const { \
      auto ptr = allocator->create(ctx, get_name());              \
      local_state* first = nullptr;                               \
      local_state* children = nullptr;                            \
      {                                                           \
        change_function_name cfn(ctx, get_name());                \
        change_nesting cn(ctx, ctx->nest_level+1);                \
        for (auto cur = childs; cur != nullptr; cur = cur->next) { \
          auto s = cur->compute(ctx, allocator);                  \
          if (first == nullptr) first = s;                        \
          if (children != nullptr) children->next = s;            \
          children = s;                                           \
        }                                                         \
      }                                                           \
      ptr->children = first;                                      \
      ptr->func = &compute_##func_name##_func;                    \
      ptr->value = ptr->func(ptr);                                \
      return ptr;                                                 \
    }                                                             \
    std::string_view func_name::get_name() const { return commands::names[type_index]; } \


    LOGIC_BLOCK_COMMANDS_LIST
#undef LOGIC_BLOCK_COMMAND_FUNC

    struct object AND::process(context* ctx) const {
      bool all_ignore = true;
      bool value = true;
      for (auto cur = childs; cur != nullptr && value; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        all_ignore = all_ignore && obj.ignore();
        value = obj.ignore() ? value : value && obj.get<bool>();
      }

      return all_ignore ? ignore_value : object(value);
    }

    struct object OR::process(context* ctx) const {
      bool all_ignore = true;
      bool value = false;
      for (auto cur = childs; cur != nullptr && !value; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        all_ignore = all_ignore && obj.ignore();
        value = obj.ignore() ? value : value || obj.get<bool>();
      }

      return all_ignore ? ignore_value : object(value);
    }

    struct object NAND::process(context* ctx) const {
      bool all_ignore = true;
      bool value = true;
      for (auto cur = childs; cur != nullptr && value; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        all_ignore = all_ignore && obj.ignore();
        value = obj.ignore() ? value : value && obj.get<bool>();
      }

      return all_ignore ? ignore_value : object(!value);
    }

    struct object NOR::process(context* ctx) const {
      bool all_ignore = true;
      bool value = false;
      for (auto cur = childs; cur != nullptr && !value; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        all_ignore = all_ignore && obj.ignore();
        value = obj.ignore() ? value : value || obj.get<bool>();
      }

      return all_ignore ? ignore_value : object(!value);
    }

    static std::tuple<object, const interface*> get_first(context* ctx, const interface* childs) {
      object obj = ignore_value;
      auto cur = childs;
      for (; cur != nullptr && obj.ignore(); cur = cur->next) { obj = cur->process(ctx); }
      return std::make_tuple(obj, cur);
    }

    struct object XOR::process(context* ctx) const {
      const auto [val, start] = get_first(ctx, childs);
      if (val.ignore()) return ignore_value;

      bool value = val.get<bool>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        value = obj.ignore() ? value : value != obj.get<bool>();
      }

      return object(value);
    }

    struct object IMPL::process(context* ctx) const {
      const auto [val, start] = get_first(ctx, childs);
      if (val.ignore()) return ignore_value;

      bool value = val.get<bool>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        value = obj.ignore() ? value : value <= obj.get<bool>();
      }

      return object(value);
    }

    struct object EQ::process(context* ctx) const {
      const auto [val, start] = get_first(ctx, childs);
      if (val.ignore()) return ignore_value;

      bool value = val.get<bool>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        value = obj.ignore() ? value : value == obj.get<bool>();
      }

      return object(value);
    }

    struct object AND_sequence::process(context* ctx) const {
      bool value = true;
      object obj(true);
      for (auto cur = childs; cur != nullptr && value && !obj.ignore(); cur = cur->next) {
        value = value && obj.get<bool>();
        obj = cur->process(ctx);
      }

      if (!obj.ignore()) value = value && obj.get<bool>(); // последний

      return object(value);
    }

    struct object OR_sequence::process(context* ctx) const {
      bool value = false;
      object obj(false);
      for (auto cur = childs; cur != nullptr && !value && !obj.ignore(); cur = cur->next) {
        value = value || obj.get<bool>();
        obj = cur->process(ctx);
      }

      if (!obj.ignore()) value = value || obj.get<bool>(); // последний

      return object(value);
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
