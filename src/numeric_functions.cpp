#include "numeric_functions.h"

#include <limits>
#include "core.h"
#include "context.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    std::tuple<struct object, const local_state*> find_first_not_ignore(const local_state* childs) {
      object obj = ignore_value;
      auto cur = childs;
      for (; cur != nullptr && obj.ignore(); cur = cur->next) { obj = cur->value; }
      // по идее cur должен быть следующим или nullptr
      return std::make_tuple(obj, cur); //cur != nullptr ? cur->next : nullptr
    }

    static object compute_add_func(const local_state* s) {
      bool all_ignored = true;
      double value = 0.0;
      for (auto cur = s->children; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        all_ignored = all_ignored && obj.ignore();
        value += obj.ignore() ? 0.0 : obj.get<double>();
      }

      return all_ignored ? ignore_value : object(value);
    }

    static object compute_sub_func(const local_state* s) {
      bool all_ignored = true;
      double value = 0.0;
      for (auto cur = s->children; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        all_ignored = all_ignored && obj.ignore();
        value += obj.ignore() ? 0.0 : -obj.get<double>();
      }

      return all_ignored ? ignore_value : object(value);
    }

    static object compute_multiply_func(const local_state* s) {
      bool all_ignored = true;
      double value = 1.0;
      for (auto cur = s->children; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        all_ignored = all_ignored && obj.ignore();
        value *= obj.ignore() ? 1.0 : obj.get<double>();
      }

      return all_ignored ? ignore_value : object(value);
    }

    static object compute_divide_func(const local_state* s) {
      bool all_ignored = true;
      double value = 0.0;
      for (auto cur = s->children; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        all_ignored = all_ignored && obj.ignore();
        const double final_val = obj.ignore() ? 1.0 : (1.0 / obj.get<double>());
        value *= final_val;
      }

      return all_ignored ? ignore_value : object(value);
    }

    static object compute_mod_func(const local_state* s) {
      const auto [first, start] = find_first_not_ignore(s);
      if (first.ignore()) return ignore_value;

      double value = first.get<double>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        value = obj.ignore() ? value : std::fmod(value, obj.get<double>());
      }

      return object(value);
    }

    static object compute_min_func(const local_state* s) {
      const auto [first, start] = find_first_not_ignore(s);
      if (first.ignore()) return ignore_value;

      double value = first.get<double>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        value = obj.ignore() ? value : std::min(value, obj.get<double>());
      }

      return object(value);
    }

    static object compute_max_func(const local_state* s) {
      const auto [first, start] = find_first_not_ignore(s);
      if (first.ignore()) return ignore_value;

      double value = first.get<double>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->value;
        value = obj.ignore() ? value : std::max(value, obj.get<double>());
      }

      return object(value);
    }

    static object compute_add_sequence_func(const local_state* s) {
      double value = 0.0;
      object obj(value);
      for (auto cur = s->children; cur != nullptr && !obj.ignore(); cur = cur->next) {
        value += obj.get<double>();
        obj = cur->value;
      }

      if (!obj.ignore()) value += obj.get<double>(); // последний

      return object(value);
    }

    static object compute_multiply_sequence_func(const local_state* s) {
      double value = 1.0;
      object obj(value);
      for (auto cur = s->children; cur != nullptr && !obj.ignore(); cur = cur->next) {
        value *= obj.get<double>();
        obj = cur->value;
      }

      if (!obj.ignore()) value *= obj.get<double>(); // последний

      return object(value);
    }

    static object compute_min_sequence_func(const local_state* s) {
      double value = std::numeric_limits<double>::max();
      object obj(value);
      for (auto cur = s->children; cur != nullptr && !obj.ignore(); cur = cur->next) {
        value = std::min(value, obj.get<double>());
        obj = cur->value;
      }

      if (!obj.ignore()) value = std::min(value, obj.get<double>()); // последний

      return value == std::numeric_limits<double>::max() ? ignore_value : object(value);
    }

    static object compute_max_sequence_func(const local_state* s) {
      double value = -std::numeric_limits<double>::max();
      object obj(value);
      for (auto cur = s->children; cur != nullptr && !obj.ignore(); cur = cur->next) {
        value = std::max(value, obj.get<double>());
        obj = cur->value;
      }

      if (!obj.ignore()) value = std::max(value, obj.get<double>()); // последний

      return value == -std::numeric_limits<double>::max() ? ignore_value : object(value);
    }

//     void name::draw(context* ctx) const {             \
//       {                                               \
//         const auto obj = process(ctx);                \
//         local_state dd(ctx);                          \
//         dd.function_name = get_name();                \
//         dd.value = obj;                               \
//         if (!ctx->draw(&dd)) return;                  \
//       }                                               \
//       change_function_name cfn(ctx, get_name());      \
//       change_nesting cn(ctx, ctx->nest_level+1);      \
//       for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->draw(ctx); } \
//     }                                                 \
//

#define NUMERIC_COMMAND_BLOCK_FUNC(name) \
    const size_t name::type_index = commands::values::name; \
    name::name(const interface* childs) noexcept : childs(childs) {} \
    name::~name() noexcept {                          \
      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); } \
    }                                                 \
    local_state* name::compute(context* ctx, local_state_allocator* allocator) const { \
      auto ptr = allocator->create(ctx, get_name());  \
      local_state* first = nullptr;                   \
      local_state* children = nullptr;                \
      {                                               \
        change_function_name cfn(ctx, get_name());    \
        change_nesting cn(ctx, ctx->nest_level+1);    \
        for (auto cur = childs; cur != nullptr; cur = cur->next) { \
          auto s = cur->compute(ctx, allocator);      \
          if (first == nullptr) first = s;            \
          if (children != nullptr) children->next = s; \
          children = s;                               \
        }                                             \
      }                                               \
      ptr->children = first;                          \
      ptr->func = &compute_##name##_func;             \
      ptr->value = ptr->func(ptr);                    \
      return ptr;                                     \
    }                                                 \
    std::string_view name::get_name() const { return commands::names[type_index]; } \


    NUMERIC_COMMANDS_LIST2

#undef NUMERIC_COMMAND_BLOCK_FUNC

    std::tuple<struct object, const interface*> find_first_not_ignore(context* ctx, const interface* childs) {
      object obj = ignore_value;
      auto cur = childs;
      for (; cur != nullptr && obj.ignore(); cur = cur->next) { obj = cur->process(ctx); }
      return std::make_tuple(obj, cur); //cur != nullptr ? cur->next : nullptr
    }

    struct object add::process(context* ctx) const {
      bool all_ignored = true;
      double value = 0.0;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        all_ignored = all_ignored && obj.ignore();
        value += obj.ignore() ? 0.0 : obj.get<double>();
      }

      return all_ignored ? ignore_value : object(value);
    }

    struct object sub::process(context* ctx) const {
//       const auto [first, start] = find_first_not_ignore(ctx, childs);
//       if (first.ignore()) return ignore_value;
//
//       double value = first.get<double>();
//       for (auto cur = start; cur != nullptr; cur = cur->next) {
//         const auto &obj = cur->process(ctx);
//         value -= obj.ignore() ? 0.0 : obj.get<double>();
//       }
//
//       return object(value);

      bool all_ignored = true;
      double value = 0.0;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        all_ignored = all_ignored && obj.ignore();
        value += obj.ignore() ? 0.0 : -obj.get<double>();
      }

      return all_ignored ? ignore_value : object(value);
    }

    struct object multiply::process(context* ctx) const {
      bool all_ignored = true;
      double value = 1.0;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        all_ignored = all_ignored && obj.ignore();
        value *= obj.ignore() ? 1.0 : obj.get<double>();
      }

      return all_ignored ? ignore_value : object(value);
    }

    struct object divide::process(context* ctx) const {
//       const auto [first, start] = find_first_not_ignore(ctx, childs);
//       if (first.ignore()) return ignore_value;
//
//       double value = first.get<double>();
//       for (auto cur = start; cur != nullptr; cur = cur->next) {
//         const auto &obj = cur->process(ctx);
//         value /= obj.ignore() ? 1.0 : obj.get<double>();
//       }
//
//       return object(value);

      bool all_ignored = true;
      double value = 0.0;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        all_ignored = all_ignored && obj.ignore();
        const double final_val = obj.ignore() ? 1.0 : (1.0 / obj.get<double>());
        value *= final_val;
      }

      return all_ignored ? ignore_value : object(value);
    }

    struct object mod::process(context* ctx) const {
      const auto [first, start] = find_first_not_ignore(ctx, childs);
      if (first.ignore()) return ignore_value;

      double value = first.get<double>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        value = obj.ignore() ? value : std::fmod(value, obj.get<double>());
      }

      return object(value);
    }

    struct object min::process(context* ctx) const {
      const auto [first, start] = find_first_not_ignore(ctx, childs);
      if (first.ignore()) return ignore_value;

      double value = first.get<double>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        value = obj.ignore() ? value : std::min(value, obj.get<double>());
      }

      return object(value);
    }

    struct object max::process(context* ctx) const {
      const auto [first, start] = find_first_not_ignore(ctx, childs);
      if (first.ignore()) return ignore_value;

      double value = first.get<double>();
      for (auto cur = start; cur != nullptr; cur = cur->next) {
        const auto &obj = cur->process(ctx);
        value = obj.ignore() ? value : std::max(value, obj.get<double>());
      }

      return object(value);
    }

    struct object add_sequence::process(context* ctx) const {
      double value = 0.0;
      object obj(value);
      for (auto cur = childs; cur != nullptr && !obj.ignore(); cur = cur->next) {
        value += obj.get<double>();
        obj = cur->process(ctx);
      }

      if (!obj.ignore()) value += obj.get<double>(); // последний

      return object(value);
    }

    struct object multiply_sequence::process(context* ctx) const {
      double value = 1.0;
      object obj(value);
      for (auto cur = childs; cur != nullptr && !obj.ignore(); cur = cur->next) {
        value *= obj.get<double>();
        obj = cur->process(ctx);
      }

      if (!obj.ignore()) value *= obj.get<double>(); // последний

      return object(value);
    }

    struct object min_sequence::process(context* ctx) const {
      double value = std::numeric_limits<double>::max();
      object obj(value);
      for (auto cur = childs; cur != nullptr && !obj.ignore(); cur = cur->next) {
        value = std::min(value, obj.get<double>());
        obj = cur->process(ctx);
      }

      if (!obj.ignore()) value = std::min(value, obj.get<double>()); // последний

      return value == std::numeric_limits<double>::max() ? ignore_value : object(value);
    }

    struct object max_sequence::process(context* ctx) const {
      double value = -std::numeric_limits<double>::max();
      object obj(value);
      for (auto cur = childs; cur != nullptr && !obj.ignore(); cur = cur->next) {
        value = std::max(value, obj.get<double>());
        obj = cur->process(ctx);
      }

      if (!obj.ignore()) value = std::max(value, obj.get<double>()); // последний

      return value == -std::numeric_limits<double>::max() ? ignore_value : object(value);
    }
  }
#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
