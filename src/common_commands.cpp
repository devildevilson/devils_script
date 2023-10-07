#include "common_commands.h"

#include "context.h"
#include "core.h"

#include <cassert>
#include <iostream>

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    static object get_last_value(const local_state* s) {
      object o;
      for (auto c = s->children; c != nullptr; c = c->next) {
        o = c->value;
      }
      return o;
    }

    //change_scope_condition::change_scope_condition(const interface* scope, const interface* condition, const interface* child) noexcept : scope(scope), condition(condition), child(child) {}
    change_scope_condition::change_scope_condition(const interface* scope, const interface* condition, const interface* child) noexcept
      : scope_interface(scope), condition_interface(condition), one_child_interface(child) {}
    change_scope_condition::~change_scope_condition() noexcept {
      //if (scope != nullptr) scope->~interface();
      //if (condition != nullptr) condition->~interface();
      //child->~interface();
    }

    object change_scope_condition::process(context* ctx) const {
      object prev_scope = ctx->prev;
      object new_scope = ctx->current;
      if (scope != nullptr) {
        prev_scope = new_scope;
        new_scope = scope->process(ctx);
      }

      change_scope sc(ctx, new_scope, prev_scope);

      if (condition != nullptr) {
        const auto &ret = condition->process(ctx);
        if (ret.ignore() || !ret.get<bool>()) return ignore_value;
      }

//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         const auto &ret = cur->process(ctx);
//         if (!ret.ignore() && !ret.get<bool>()) return object(false);
//       }

      return child->process(ctx);
    }

    local_state* change_scope_condition::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* scope_state = nullptr;
      local_state* condition_state = nullptr;
      local_state* child_state = nullptr;
      object cond_obj;
      {
        object prev_scope = ctx->prev;
        object new_scope = ctx->current;

        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        if (scope != nullptr) {
          scope_state = scope->compute(ctx, allocator);
          scope_state->argument_name = "scope";
          prev_scope = new_scope;
          new_scope = scope_state->value;
        }

        change_scope sc(ctx, new_scope, prev_scope);

        if (condition != nullptr) {
          condition_state = condition->compute(ctx, allocator);
          condition_state->argument_name = "condition";
          cond_obj = condition_state->value;
        }

        child_state = child->compute(ctx, allocator);
      }

      local_state* first = nullptr;
      if (scope_state != nullptr) { first = scope_state; }
      if (condition_state != nullptr) {
        if (first == nullptr) first = condition_state;
        else first->next = condition_state;
      }

      if (first != nullptr) {
        if (first->next != nullptr) first->next->next = child_state;
        else first->next = child_state;
      } else first = child_state;

      ptr->children = first;
      if (cond_obj.ignore() || !cond_obj.get<bool>()) ptr->value = ignore_value;
      else ptr->value = child_state->value;
      // берем последнее значение
      ptr->func = &get_last_value;
      return ptr;
    }

//     void change_scope_condition::draw(context* ctx) const {
//       object prev_scope = ctx->prev;
//       object new_scope = ctx->current;
//       size_t nesting = ctx->nest_level;
//       nesting += size_t(scope != nullptr || condition != nullptr);
//       if (scope != nullptr) {
//         scope->draw(ctx);
//         prev_scope = new_scope;
//         new_scope = scope->process(ctx);
//       }
//
//       {
//         const auto &ret = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = ret;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_scope sc(ctx, new_scope, prev_scope);
//       change_nesting сn(ctx, nesting);
//
//       if (condition != nullptr) {
//         {
//           const auto &ret = condition->process(ctx);
//           local_state dd(ctx);
//           dd.function_name = "condition";
//           dd.value = object(ret.get<bool>());
//           ctx->draw(&dd);
//         }
//         change_nesting cn(ctx, ctx->nest_level+1);
//         change_function_name cfn(ctx, "condition");
//         condition->draw(ctx);
//       }
//
//       child->draw(ctx);
//     }

    size_t change_scope_condition::get_type_id() const { return type_id<object>(); }
    std::string_view change_scope_condition::get_name() const { return "change_scope_condition"; }

    change_scope_effect::change_scope_effect(const interface* scope, const interface* condition, const interface* childs) noexcept
      : scope_interface(scope), condition_interface(condition), children_interface(childs) {}
    change_scope_effect::~change_scope_effect() noexcept {
//      if (scope != nullptr) scope->~interface();
//      if (condition != nullptr) condition->~interface();
//      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    object change_scope_effect::process(context* ctx) const {
      object prev_scope = ctx->prev;
      object new_scope = ctx->current;
      if (scope != nullptr) {
        prev_scope = new_scope;
        new_scope = scope->process(ctx);
      }

      change_scope sc(ctx, new_scope, prev_scope);

      if (condition != nullptr) {
        const auto &ret = condition->process(ctx);
        if (ret.ignore() || !ret.get<bool>()) return ignore_value;
      }

      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        cur->process(ctx); // реагируем ли мы тут как то если получаем ignore_value? если бы это был ifelse то тогда бы реагировали
      }

      return object();
    }

    local_state* change_scope_effect::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* scope_state = nullptr;
      local_state* condition_state = nullptr;
      local_state* first_child_state = nullptr;
      local_state* child_states = nullptr;
      {
        object prev_scope = ctx->prev;
        object new_scope = ctx->current;

        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        if (scope != nullptr) {
          scope_state = scope->compute(ctx, allocator);
          scope_state->argument_name = "scope";
          prev_scope = new_scope;
          new_scope = scope_state->value;
        }

        change_scope sc(ctx, new_scope, prev_scope);

        if (condition != nullptr) {
          condition_state = condition->compute(ctx, allocator);
          condition_state->argument_name = "condition";
        }

        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          auto child_state = cur->compute(ctx, allocator);

          if (first_child_state == nullptr) first_child_state = child_state;
          if (child_states != nullptr) child_states->next = child_state;
          child_states = child_state;
        }
      }

      local_state* first = nullptr;
      if (scope_state != nullptr) { first = scope_state; }
      if (condition_state != nullptr) {
        if (first == nullptr) first = condition_state;
        else first->next = condition_state;
      }

      if (first != nullptr) {
        if (first->next != nullptr) first->next->next = first_child_state;
        else first->next = first_child_state;
      } else first = first_child_state;

      ptr->children = first;
      return ptr;
    }

//     void change_scope_effect::draw(context* ctx) const {
//       object prev_scope = ctx->prev;
//       object new_scope = ctx->current;
//       size_t nesting = ctx->nest_level;
//       nesting += size_t(scope != nullptr || condition != nullptr);
//       if (scope != nullptr) {
//         scope->draw(ctx); // вызываем ли тут вообще драв? может быть нужно просто получить скоуп, наверрное все же вызываем
//         prev_scope = new_scope;
//         new_scope = scope->process(ctx);
//       }
//
//       {
//         const auto &ret = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = ret;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_scope sc(ctx, new_scope, prev_scope);
//       change_nesting cn(ctx, nesting);
//
//       // нужно как то дать понять что выполняется условие
//       if (condition != nullptr) {
//         {
//           const auto &ret = condition->process(ctx);
//           local_state dd(ctx);
//           dd.function_name = "condition";
//           dd.value = object(ret.get<bool>());
//           ctx->draw(&dd);
//         }
//         change_nesting cn(ctx, ctx->nest_level+1);
//         change_function_name cfn(ctx, "condition");
//         condition->draw(ctx);
//       }
//
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t change_scope_effect::get_type_id() const { return type_id<object>(); }
    std::string_view change_scope_effect::get_name() const { return "change_scope_effect"; }

    //compute_string::compute_string(const interface* condition, const interface* childs) noexcept : condition(condition), childs(childs) {}
    compute_string::compute_string(const interface* condition, const interface* childs) noexcept
      : condition_interface(condition), children_interface(childs) {}
    compute_string::~compute_string() noexcept {
      //if (condition != nullptr) condition->~interface();
      //for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    object compute_string::process(context* ctx) const {
      if (condition != nullptr) {
        const auto &ret = condition->process(ctx);
        if (ret.ignore() || !ret.get<bool>()) return ignore_value;
      }

      object cur_obj = ignore_value;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        cur_obj = cur->process(ctx);
      }

      return cur_obj;
    }

    local_state* compute_string::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto cond = condition != nullptr ? condition->compute(ctx, allocator) : nullptr;
      local_state* first_child_state = nullptr;
      local_state* child_states = nullptr;
      object o;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        auto child_state = cur->compute(ctx, allocator);

        o = child_state->value;

        if (first_child_state == nullptr) first_child_state = child_state;
        if (child_states != nullptr) child_states->next = child_state;
        child_states = child_state;
      }

      if (cond != nullptr) {
        cond->argument_name = "condition";
        cond->next = first_child_state;
      }

      ptr->children = cond != nullptr ? cond : first_child_state;
      ptr->value = o;
      ptr->func = [] (const local_state* s) -> object { object o; for (auto c = s->children; c != nullptr; c = c->next) { o = c->value; } return o; };
      return ptr;
    }

//     void compute_string::draw(context* ctx) const {
//       {
//         const auto &ret = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = ret;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_nesting cn(ctx, ctx->nest_level+1);
//
//       if (condition != nullptr) {
//         {
//           const auto &ret = condition->process(ctx);
//           local_state dd(ctx);
//           dd.function_name = "condition";
//           dd.value = object(ret.get<bool>());
//           ctx->draw(&dd);
//         }
//         change_nesting cn(ctx, ctx->nest_level+1);
//         change_function_name cfn(ctx, "condition");
//         condition->draw(ctx);
//       }
//
//       change_function_name cfn(ctx, get_name());
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t compute_string::get_type_id() const { return type_id<object>(); }
    std::string_view compute_string::get_name() const { return "compute_string"; }

    //compute_object::compute_object(const interface* condition, const interface* childs) noexcept : condition(condition), childs(childs) {}
    compute_object::compute_object(const interface* condition, const interface* childs) noexcept
      : condition_interface(condition), children_interface(childs) {}
    compute_object::~compute_object() noexcept {
      //if (condition != nullptr) condition->~interface();
      //for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    object compute_object::process(context* ctx) const {
      if (condition != nullptr) {
        const auto &ret = condition->process(ctx);
        if (ret.ignore() || !ret.get<bool>()) return ignore_value;
      }

      object cur_obj = ignore_value;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        cur_obj = cur->process(ctx);
      }

      return cur_obj;
    }

    local_state* compute_object::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto cond = condition != nullptr ? condition->compute(ctx, allocator) : nullptr;
      local_state* first_child_state = nullptr;
      local_state* child_states = nullptr;
      object o;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        auto child_state = cur->compute(ctx, allocator);

        o = child_state->value;

        if (first_child_state == nullptr) first_child_state = child_state;
        if (child_states != nullptr) child_states->next = child_state;
        child_states = child_state;
      }

      if (cond != nullptr) {
        cond->argument_name = "condition";
        cond->next = first_child_state;
      }

      ptr->children = cond != nullptr ? cond : first_child_state;
      ptr->value = o;
      ptr->func = [] (const local_state* s) -> object { object o; for (auto c = s->children; c != nullptr; c = c->next) { o = c->value; } return o; };
      return ptr;
    }

//     void compute_object::draw(context* ctx) const {
//       // тут надо че нить рисовать?
//       {
//         const auto &ret = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = ret;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_nesting cn(ctx, ctx->nest_level+1);
//
//       if (condition != nullptr) {
//         {
//           const auto &ret = condition->process(ctx);
//           local_state dd(ctx);
//           dd.function_name = "condition";
//           dd.value = object(ret.get<bool>());
//           ctx->draw(&dd);
//         }
//         change_nesting cn(ctx, ctx->nest_level+1);
//         change_function_name cfn(ctx, "condition");
//         condition->draw(ctx);
//       }
//
//       change_function_name cfn(ctx, get_name());
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t compute_object::get_type_id() const { return type_id<object>(); }
    std::string_view compute_object::get_name() const { return "compute_object"; }

    //compute_number::compute_number(const interface* scope, const interface* condition, const interface* child) noexcept : scope(scope), condition(condition), child(child) {}
    compute_number::compute_number(const interface* scope, const interface* condition, const interface* child) noexcept
      : scope_interface(scope), condition_interface(condition), one_child_interface(child) {}
    compute_number::~compute_number() noexcept {
      //if (scope != nullptr) scope->~interface();
      //if (condition != nullptr) condition->~interface();
      //child->~interface();
    }
    object compute_number::process(context* ctx) const {
      object prev_scope = ctx->prev;
      object new_scope = ctx->current;
      if (scope != nullptr) {
        prev_scope = new_scope;
        new_scope = scope->process(ctx);
      }

      change_scope sc(ctx, new_scope, prev_scope);

      if (condition != nullptr) {
        const auto &ret = condition->process(ctx);
        if (ret.ignore() || !ret.get<bool>()) return ignore_value;
      }

      return child->process(ctx);
    }

    local_state* compute_number::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* scope_state = nullptr;
      local_state* condition_state = nullptr;
      local_state* child_state = nullptr;
      object cond_obj;
      {
        object prev_scope = ctx->prev;
        object new_scope = ctx->current;

        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        if (scope != nullptr) {
          scope_state = scope->compute(ctx, allocator);
          scope_state->argument_name = "scope";
          prev_scope = new_scope;
          new_scope = scope_state->value;
        }

        change_scope sc(ctx, new_scope, prev_scope);

        if (condition != nullptr) {
          condition_state = condition->compute(ctx, allocator);
          condition_state->argument_name = "condition";
          cond_obj = condition_state->value;
        }

        child_state = child->compute(ctx, allocator);
      }

      local_state* first = nullptr;
      if (scope_state != nullptr) { first = scope_state; }
      if (condition_state != nullptr) {
        if (first == nullptr) first = condition_state;
        else first->next = condition_state;
      }

      if (first != nullptr) {
        if (first->next != nullptr) first->next->next = child_state;
        else first->next = child_state;
      } else first = child_state;

      ptr->children = first;
      if (cond_obj.ignore() || !cond_obj.get<bool>()) ptr->value = ignore_value;
      else ptr->value = child_state->value;
      // берем последнее значение
      ptr->func = &get_last_value;
      return ptr;
    }

//     void compute_number::draw(context* ctx) const {
//       object prev_scope = ctx->prev;
//       object new_scope = ctx->current;
//       size_t nesting = ctx->nest_level;
//       nesting += size_t(scope != nullptr || condition != nullptr);
//       if (scope != nullptr) {
//         scope->draw(ctx);
//         prev_scope = new_scope;
//         new_scope = scope->process(ctx);
//       }
//
//       {
//         const auto &ret = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = ret;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_scope sc(ctx, new_scope, prev_scope);
//       change_nesting cn(ctx, nesting);
//
//       if (condition != nullptr) {
//         {
//           const auto &ret = condition->process(ctx);
//           local_state dd(ctx);
//           dd.function_name = "condition";
//           dd.value = object(ret.get<bool>());
//           ctx->draw_function(&dd);
//         }
//         change_nesting cn(ctx, ctx->nest_level+1);
//         change_function_name cfn(ctx, "condition");
//         condition->draw(ctx);
//       }
//
//       child->draw(ctx);
//     }

    size_t compute_number::get_type_id() const { return type_id<object>(); }
    std::string_view compute_number::get_name() const { return "compute_number"; }

    const size_t selector::type_index = commands::values::selector;
    selector::selector(const interface* childs) noexcept : children_interface(childs) {}
    //selector::~selector() noexcept { for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); } }
    selector::~selector() noexcept {}
    object selector::process(context* ctx) const {
      object obj = ignore_value;
      for (auto cur = childs; cur != nullptr && obj.ignore(); cur = cur->next) {
        obj = cur->process(ctx);
      }
      return obj;
    }

    local_state* selector::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* first_child_state = nullptr;
      local_state* child_states = nullptr;
      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        auto s = cur->compute(ctx, allocator);

        if (first_child_state == nullptr) first_child_state = s;
        if (child_states != nullptr) child_states->next = s;
        child_states = s;
      }

      ptr->children = first_child_state;
      ptr->func = [] (const local_state* s) -> object {
        object o = ignore_value;
        for (auto c = s->children; c != nullptr && o.ignore() && !o.unresolved(); c = c->next) {
          o = c->value;
        }
        return o;
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void selector::draw(context* ctx) const {
//       {
//         const auto obj = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = obj.ignore() ? object() : obj;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_nesting cn(ctx, ctx->nest_level+1);
//       change_function_name cfn(ctx, get_name());
//
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t selector::get_type_id() const { return type_id<object>(); }
    std::string_view selector::get_name() const { return commands::names[type_index]; }

    const size_t sequence::type_index = commands::values::sequence;
    //sequence::sequence(const interface* count, const interface* childs) noexcept : count(count), childs(childs) {}
    sequence::sequence(const interface* count, const interface* childs) noexcept
      : additional_child_interface(count), children_interface(childs) {}
    sequence::~sequence() noexcept {
      //for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }
    object sequence::process(context* ctx) const {
      size_t max_count = SIZE_MAX;
      if (additional != nullptr) {
        const auto obj = additional->process(ctx);
        max_count = obj.get<double>();
      }

      size_t counter = 0;
      object prev_obj;
      object obj;
      for (auto cur = childs; cur != nullptr && counter < max_count && !obj.ignore(); cur = cur->next, ++counter) {
        const auto cur_obj = cur->process(ctx);
        prev_obj = obj;
        obj = cur_obj;
      }
      return !obj.ignore() ? obj : (prev_obj.valid() ? prev_obj : ignore_value);
    }

    local_state* sequence::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto count_arg = additional != nullptr ? additional->compute(ctx, allocator) : nullptr;
      local_state* first_child_state = nullptr;
      local_state* child_states = nullptr;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        auto s = cur->compute(ctx, allocator);

        if (first_child_state == nullptr) first_child_state = s;
        if (child_states != nullptr) child_states->next = s;
        child_states = s;
      }

      if (count_arg != nullptr) {
        count_arg->argument_name = "count";
        count_arg->next = first_child_state;
      }

      ptr->children = count_arg != nullptr ? count_arg : first_child_state;
      ptr->func = [] (const local_state* s) -> object {
        auto cur = s->children;
        size_t max_count = SIZE_MAX;
        if (s->children->argument_name == "count") {
          if (s->children->value.unresolved()) return unresolved_value;
          max_count = s->children->value.get<double>();
          cur = cur->next;
        }

        size_t counter = 0;
        object prev_obj;
        object obj;
        for (; cur != nullptr && counter < max_count && !obj.ignore() && !obj.unresolved(); cur = cur->next, ++counter) {
          const auto cur_obj = cur->value;
          prev_obj = obj;
          obj = cur_obj;
        }
        return !obj.ignore() ? obj : (prev_obj.valid() ? prev_obj : ignore_value);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void sequence::draw(context* ctx) const {
//       {
//         const auto obj = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = obj.ignore() ? object() : obj;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_nesting cn(ctx, ctx->nest_level+1);
//       change_function_name cfn(ctx, get_name());
//       for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->draw(ctx); }
//     }

    size_t sequence::get_type_id() const { return type_id<object>(); }
    std::string_view sequence::get_name() const { return commands::names[type_index]; }

    overload::overload(const std::array<size_t, MAXIMUM_OVERLOADS> &overload_types, const interface* childs) noexcept
      : children_interface(childs), overload_types(overload_types) {}
    overload::~overload() noexcept {
      //for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }
    struct object overload::process(context* ctx) const {
      // здесь к нам приходит какой то объект, по его типу нужно найти подходящего ребенка и вызвать его
      // как искать? нужно ввести еще парочку функций в интерфейс, например get_type, к сожалению придется делать их virtual
      // потенциально к ним может прийти type_id<object>()
      const interface* any_child = nullptr;
      size_t counter = 0;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        assert(counter < MAXIMUM_OVERLOADS);
        const size_t expected_type = overload_types[counter];
        ++counter;

        if (expected_type == type_id<void>()) any_child = cur; // может быть только один?
        if (expected_type == ctx->current.get_type()) return cur->process(ctx);
      }
      return any_child != nullptr ? any_child->process(ctx) : object();
    }

    local_state* overload::compute(context* ctx, local_state_allocator* allocator) const {
      const interface* any_child = nullptr;
      size_t counter = 0;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        assert(counter < MAXIMUM_OVERLOADS);
        const size_t expected_type = overload_types[counter];
        ++counter;

        if (expected_type == type_id<void>()) any_child = cur; // может быть только один?
        if (expected_type == ctx->current.get_type()) return cur->compute(ctx, allocator);
      }
      return any_child != nullptr ? any_child->compute(ctx, allocator) : nullptr;
    }

//     void overload::draw(context* ctx) const {
//       const interface* any_child = nullptr;
//       size_t counter = 0;
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         assert(counter < MAXIMUM_OVERLOADS);
//         const size_t expected_type = overload_types[counter];
//         ++counter;
//
//         //const size_t id = cur->get_type_id(); // get_type_id не потребовался, че я сразу не догадался до того чтобы хранить массив?
//         //if (id == type_id<object>()) {}
//         if (expected_type == type_id<void>()) any_child = cur; // может быть только один?
//         if (expected_type == ctx->current.get_type()) { cur->draw(ctx); return; }
//       }
//
//       if (any_child != nullptr) any_child->draw(ctx);
//     }

    size_t overload::get_type_id() const { return type_id<object>(); }
    std::string_view overload::get_name() const { return "overload"; }

//     const size_t at_least_sequence::type_index = commands::values::at_least_sequence;
//     at_least_sequence::at_least_sequence(const interface* count, const interface* childs) noexcept : count(count), childs(childs) {}
//     at_least_sequence::~at_least_sequence() noexcept { count->~interface(); for (auto c = childs; c != nullptr; c = c->next) { c->~interface(); } }
//     object at_least_sequence::process(context* ctx) const {
//       const auto obj = count->process(ctx);
//       const size_t max_count = obj.get<double>();
//       size_t counter = 0;
//       object cur = ignore_value;
//       for (auto c = childs; c != nullptr; c = c->next) {
//         if (counter >= max_count) break;
//         const auto cur_obj = c->process(ctx);
//         if (cur_obj.ignore()) break;
//         cur = cur_obj;
//         ++counter;
//       }
//       return obj;
//     }
//
//     void at_least_sequence::draw(context* ctx) const {
//       const auto obj = process(ctx);
//       local_state dd(ctx);
//       dd.function_name = commands::names[type_index];
//       dd.value = obj.ignore() ? object() : obj;
//       ctx->draw(&dd);
//
//       change_nesting cn(ctx, ctx->nest_level+1);
//       change_function_name cfn(ctx, dd.function_name);
//
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    const size_t chance::type_index = commands::values::chance;
    chance::chance(const size_t &state, const interface* value) noexcept : one_child_interface(value), state(state) {}
    chance::~chance() noexcept {}
    object chance::process(context* ctx) const {
      const auto obj = child->process(ctx);
      if (obj.ignore()) return ignore_value;
      const double val = obj.get<double>();
      const uint64_t rnd_val = ctx->get_random_value(state);
      const double norm = context::normalize_value(rnd_val);
      return object(norm < val);
    }

    local_state* chance::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto value_state = child->compute(ctx, allocator);
      value_state->argument_name = "value";

      ptr->local_rand_state = state;
      ptr->children = value_state;
      ptr->func = [] (const local_state* s) -> object {
        auto obj = s->children->value;
        if (obj.unresolved()) return unresolved_value;
        if (obj.ignore())     return ignore_value;
        const double val = obj.get<double>();
        const uint64_t rnd_val = s->get_random_value(s->local_rand_state);
        const double norm = context::normalize_value(rnd_val);
        return object(norm < val);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void chance::draw(context* ctx) const {
//       {
//         const auto origin = value->process(ctx);
//         const auto obj = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = obj;
//         dd.original = origin;
//         ctx->draw(&dd);
//       }
//
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       value->draw(ctx);
//     }

    size_t chance::get_type_id() const { return type_id<object>(); }
    std::string_view chance::get_name() const { return commands::names[type_index]; }

    static const interface* get_random_child(context* ctx, const size_t &random_state, const interface* childs, const interface* weights) {
      size_t counter = 0;
      double accum_w = 0.0;
      const size_t array_size = 128;
      std::array<double, array_size> w_numbers;
      for (auto cur_w = weights, cur = childs; cur_w != nullptr; cur_w = cur_w->next, cur = cur->next) {
        if (counter >= array_size) throw std::runtime_error("'weighted_random' has childs more than array size (" + std::to_string(array_size) + ")");
        if (cur == nullptr) throw std::runtime_error("Childs count less than weights");
        const auto obj = cur_w->process(ctx);
        const double local_w = obj.ignore() ? 0.0 : obj.get<double>();
        accum_w += local_w;
        w_numbers[counter] = local_w;
        counter += 1;
      }

      const uint64_t rand = ctx->get_random_value(random_state);
      const double rand_num = context::normalize_value(rand);
      const double final_num = accum_w * rand_num;

      const interface* choosed = childs;
      const interface* prev_choosed = childs;
      double cumulative = 0.0;
      size_t index = 0;
      for (; index < counter && cumulative <= final_num; cumulative += w_numbers[counter], ++index) { prev_choosed = choosed; choosed = choosed->next; }
      index -= 1; // нужно брать предыдущее значение, потому что в choosed может лежать nullptr

      return prev_choosed;
    }

    const size_t weighted_random::type_index = commands::values::weighted_random;
    weighted_random::weighted_random(const size_t &state, const interface* childs, const interface* weights) noexcept
      : children_interface(childs), additional_children_interface(weights), state(state) {}
    weighted_random::~weighted_random() noexcept {
      //for (auto cur = childs, cur_w = weights; cur != nullptr; cur = cur->next, cur_w = cur_w->next) { cur->~interface(); cur_w->~interface(); }
    }
    object weighted_random::process(context* ctx) const {
      auto choosed = get_random_child(ctx, state, childs, additional_childs);
      return choosed->process(ctx);
    }

    local_state* weighted_random::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* first = nullptr;
      local_state* children = nullptr;
      local_state* w_first = nullptr;
      local_state* w_children = nullptr;

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      for (auto cur_w = additional_childs, cur = childs; cur_w != nullptr; cur_w = cur_w->next, cur = cur->next) {
        if (cur == nullptr) throw std::runtime_error("Childs count less than weights");
        auto w_s = cur_w->compute(ctx, allocator);
        auto s = cur->compute(ctx, allocator);

        w_s->argument_name = "weight";

        if (first == nullptr) first = s;
        if (children != nullptr) children->next = s;
        children = s;

        if (w_first == nullptr) w_first = w_s;
        if (w_children != nullptr) w_children->next = w_s;
        w_children = w_s;
      }

      w_children->next = first;

      ptr->children = w_first;
      ptr->local_rand_state = state;
      ptr->func = [] (const local_state* s) -> object {
        size_t counter = 0;
        double accum_w = 0.0;
        const size_t array_size = 128;
        std::array<double, array_size> w_numbers;
        auto c = s->children;
        for (; c != nullptr && !c->argument_name.empty(); c = c->next) {
          const auto obj = c->value;
          if (obj.unresolved()) return unresolved_value;
          const double val = obj.ignore() ? 0.0 : obj.get<double>();
          accum_w += val;
          w_numbers[counter] = val;
          ++counter;
        }

        assert(c != nullptr && c->argument_name.empty());

        const uint64_t rand = s->get_random_value(s->local_rand_state);
        const double rand_num = context::normalize_value(rand);
        const double final_num = accum_w * rand_num;

        double cumulative = 0.0;
        local_state* prev_choosed = c;
        local_state* choosed = c;
        size_t index = 0;
        for (; index < counter && cumulative <= final_num; cumulative += w_numbers[counter], ++index) { prev_choosed = choosed; choosed = choosed->next; }
        index -= 1; // нужно брать предыдущее значение, потому что в choosed может лежать nullptr

        return prev_choosed->value;
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void weighted_random::draw(context* ctx) const {
//       auto choosed = get_random_child(ctx, state, childs, weights);
//       {
//         // запуск таких вот обходов для получения значения череват дополнительным запуском функций рандома, что может поломать описание, исправил
//         const auto obj = choosed->process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = obj;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       choosed->draw(ctx);
//     }

    size_t weighted_random::get_type_id() const { return type_id<object>(); }
    std::string_view weighted_random::get_name() const { return commands::names[type_index]; }

    const size_t random_value::type_index = commands::values::random_value;
    random_value::random_value(const size_t &state, const interface* maximum) noexcept
      : one_child_interface(maximum), state(state) {}
    random_value::~random_value() noexcept {}
    struct object random_value::process(context* ctx) const {
      double max = 1.0;
      if (child != nullptr) {
        const auto obj = child->process(ctx);
        if (obj.ignore()) return ignore_value;
        max = obj.get<double>();
      }

      const uint64_t rand = ctx->get_random_value(state);
      const double rand_num = context::normalize_value(rand);
      const double final_num = max * rand_num;

      return object(final_num);
    }

    local_state* random_value::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto max = child != nullptr ? child->compute(ctx, allocator) : nullptr;
      if (max != nullptr) max->argument_name = "maximum_value";

      ptr->children = max;
      ptr->local_rand_state = state;
      ptr->func = [] (const local_state* s) -> object {
        double max = 1.0;
        if (s->children != nullptr) {
          const auto obj = s->children->value;
          if (obj.unresolved()) return unresolved_value;
          if (obj.ignore()) return ignore_value;
          max = obj.get<double>();
        }

        const uint64_t rand = s->get_random_value(s->local_rand_state);
        const double rand_num = context::normalize_value(rand);
        const double final_num = max * rand_num;

        return object(final_num);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void random_value::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = process(ctx);
//         if (maximum != nullptr) dd.original = maximum->process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       // нужно ли идти дальше? не думаю, вообще наверное надо, хотя бы для дебага
//       if (maximum == nullptr) return;
//
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       maximum->draw(ctx);
//     }

    size_t random_value::get_type_id() const { return type_id<object>(); }
    std::string_view random_value::get_name() const { return commands::names[type_index]; }

    boolean_container::boolean_container(const bool value) noexcept : value(value) {}
    object boolean_container::process(context*) const { return object(value); }
    local_state* boolean_container::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = object(value);
      return ptr;
    }
//     void boolean_container::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = value;
//       ctx->draw(&dd);
//     }
    size_t boolean_container::get_type_id() const { return type_id<object>(); }
    std::string_view boolean_container::get_name() const { return "boolean_container"; }

    number_container::number_container(const double &value) noexcept : value(value) {}
    object number_container::process(context*) const { return object(value); }
    local_state* number_container::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = object(value);
      return ptr;
    }
//     void number_container::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = value;
//       ctx->draw(&dd);
//     }
    size_t number_container::get_type_id() const { return type_id<object>(); }
    std::string_view number_container::get_name() const { return "number_container"; }

    string_container::string_container(const std::string &value) noexcept : value(value) {}
    string_container::string_container(const std::string_view &value) noexcept : value(value) {}
    string_container::~string_container() noexcept { std::cout << "~string_container()\n"; }
    object string_container::process(context*) const { return object(value); }
    local_state* string_container::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = object(value);
      return ptr;
    }
//     void string_container::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = value;
//       ctx->draw(&dd);
//     }
    size_t string_container::get_type_id() const { return type_id<object>(); }
    std::string_view string_container::get_name() const { return "string_container"; }

    object_container::object_container(const object &value) noexcept : value(value) {}
    object object_container::process(context*) const { return value; }
    local_state* object_container::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = value;
      return ptr;
    }
//     void object_container::draw(context*) const { assert(false); }
    size_t object_container::get_type_id() const { return type_id<object>(); }
    std::string_view object_container::get_name() const { return "object_container"; }

    static bool compare_func(const uint8_t op, const double num1, const double num2) {
      const bool eq = std::abs(num1 - num2) < EPSILON;
      switch (static_cast<compare_operators::values>(op)) {
        case compare_operators::equal:     return eq;
        case compare_operators::not_equal: return !eq;
        case compare_operators::more:      return num1 > num2;
        case compare_operators::less:      return num1 < num2;
        case compare_operators::more_eq:   return num1 > num2 || eq;
        case compare_operators::less_eq:   return num1 < num2 || eq;
        default: throw std::runtime_error("Add aditional comparisons");
      }

      return false;
    }

    number_comparator::number_comparator(const uint8_t op, const interface* lvalue, const interface* rvalue) noexcept
      : additional_child_interface(lvalue), one_child_interface(rvalue), op(op) {}
    number_comparator::~number_comparator() noexcept {
//      lvalue->~interface();
//      rvalue->~interface();
    }

    object number_comparator::process(context* ctx) const {
      const auto &num1 = additional->process(ctx);
      const auto &num2 = child->process(ctx);
      const double num1_final = num1.get<double>();
      const double num2_final = num2.get<double>();
      return object(compare_func(op, num1_final, num2_final));
    }

    local_state* number_comparator::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto l_state = additional->compute(ctx, allocator);
      l_state->argument_name = "lvalue";
      auto r_state = child->compute(ctx, allocator);
      r_state->argument_name = "rvalue";
      l_state->next = r_state;

      ptr->children = l_state;
      ptr->operator_type = op;
      ptr->func = [] (const local_state* s) -> object {
        if (s->children->value.unresolved()) return unresolved_value;
        if (s->children->next->value.unresolved()) return unresolved_value;
        const double num1_final = s->children->value.get<double>();       // lvalue
        const double num2_final = s->children->next->value.get<double>(); // rvalue
        return object(compare_func(s->operator_type, num1_final, num2_final));
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void number_comparator::draw(context* ctx) const {
//       const auto &num2 = rvalue->process(ctx);
//       change_rvalue crv(ctx, num2, op);
//       lvalue->draw(ctx);
//     }

    size_t number_comparator::get_type_id() const { return type_id<object>(); }
    std::string_view number_comparator::get_name() const { return "number_comparator"; }

    boolean_comparator::boolean_comparator(const interface* lvalue, const interface* rvalue) noexcept
      : additional_child_interface(lvalue), one_child_interface(rvalue) {}
    boolean_comparator::~boolean_comparator() noexcept {
//      lvalue->~interface();
//      rvalue->~interface();
    }
    object boolean_comparator::process(context* ctx) const {
      const auto &num1 = additional->process(ctx);
      const auto &num2 = child->process(ctx);
      const bool num1_final = num1.get<bool>();
      const bool num2_final = num2.get<bool>();
      return object(num1_final == num2_final);
    }

    local_state* boolean_comparator::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto l_state = additional->compute(ctx, allocator);
      l_state->argument_name = "lvalue";
      auto r_state = child->compute(ctx, allocator);
      r_state->argument_name = "rvalue";
      l_state->next = r_state;

      ptr->children = l_state;
      ptr->func = [] (const local_state* s) -> object {
        if (s->children->value.unresolved()) return unresolved_value;
        if (s->children->next->value.unresolved()) return unresolved_value;
        const bool num1_final = s->children->value.get<bool>();       // lvalue
        const bool num2_final = s->children->next->value.get<bool>(); // rvalue
        return object(num1_final == num2_final);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void boolean_comparator::draw(context* ctx) const {
//       const auto &num2 = rvalue->process(ctx);
//       change_rvalue cr(ctx, num2, 0);
//       lvalue->draw(ctx);
//     }

    size_t boolean_comparator::get_type_id() const { return type_id<object>(); }
    std::string_view boolean_comparator::get_name() const { return "boolean_comparator"; }

    const size_t equals_to::type_index = commands::values::equals_to;
    equals_to::equals_to(const interface* get_obj) noexcept : one_child_interface(get_obj) {}
    equals_to::~equals_to() noexcept {}
    object equals_to::process(context* ctx) const {
      const auto obj = child->process(ctx);
      return object(ctx->current == obj);
    }

    local_state* equals_to::compute(context* ctx, local_state_allocator* allocator) const {
      local_state* state = nullptr;
      {
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        state = child->compute(ctx, allocator);
      }
      auto ptr = allocator->create(ctx, get_name());
      ptr->children = state;
      ptr->func = [] (const local_state* s) -> object {
        if (s->children->value.unresolved()) return unresolved_value;
        if (s->current.unresolved()) return unresolved_value;
        return object(s->current == s->children->value);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void equals_to::draw(context* ctx) const {
//       const auto obj = get_obj->process(ctx);
//       const auto val = object(ctx->current == obj);
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = val;
//       dd.original = obj;
//       ctx->draw(&dd);
//     }

    size_t equals_to::get_type_id() const { return type_id<object>(); }
    std::string_view equals_to::get_name() const { return commands::names[type_index]; }

    const size_t not_equals_to::type_index = commands::values::not_equals_to;
    not_equals_to::not_equals_to(const interface* get_obj) noexcept : one_child_interface(get_obj) {}
    not_equals_to::~not_equals_to() noexcept {}
    object not_equals_to::process(context* ctx) const {
      const auto obj = child->process(ctx);
      return object(ctx->current != obj);
    }

    local_state* not_equals_to::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto state = child->compute(ctx, allocator);
      ptr->children = state;
      ptr->func = [] (const local_state* s) -> object {
        if (s->children->value.unresolved()) return unresolved_value;
        if (s->current.unresolved()) return unresolved_value;
        return object(s->current != s->children->value);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void not_equals_to::draw(context* ctx) const {
//       const auto obj = get_obj->process(ctx);
//       const auto val = object(ctx->current != obj);
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = val;
//       dd.original = obj;
//       ctx->draw(&dd);
//     }

    size_t not_equals_to::get_type_id() const { return type_id<object>(); }
    std::string_view not_equals_to::get_name() const { return commands::names[type_index]; }

    const size_t equality::type_index = commands::values::equality;
    equality::equality(const interface* childs) noexcept : children_interface(childs) {}
    equality::~equality() noexcept {}
    object equality::process(context* ctx) const {
      object obj = ignore_value;
      auto cur = childs;
      for (; cur != nullptr && obj.ignore(); cur = cur->next) {
        obj = cur->process(ctx);
      }

      for (; cur != nullptr; cur = cur->next) {
        const auto tmp = cur->process(ctx);
        if (!tmp.ignore() && tmp != obj) return object(false);
        //obj = tmp; // зачем, если мы проверяем равенство?
      }

      return object(true);
    }

    local_state* equality::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* first = nullptr;
      local_state* children = nullptr;
      {
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          auto s = cur->compute(ctx, allocator);
          if (first == nullptr) first = s;
          if (children != nullptr) children->next = s;
          children = s;
        }
      }

      ptr->children = first;
      ptr->func = [] (const local_state* s) -> object {
        object obj = ignore_value;
        auto cur = s->children;
        for (; cur != nullptr && obj.ignore(); cur = cur->next) {
          obj = cur->value;
        }

        if (obj.unresolved()) return unresolved_value;

        for (; cur != nullptr; cur = cur->next) {
          const auto &tmp = cur->value;
          if (tmp.unresolved()) return unresolved_value;
          if (!tmp.ignore() && tmp != obj) return object(false);
        }

        return object(true);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void equality::draw(context* ctx) const {
//       {
//         const auto obj = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = obj;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       // нужно ли тут обходить детей? может и нужно, но как это дело рисовать не ясно
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->draw(ctx); }
//     }

    size_t equality::get_type_id() const { return type_id<object>(); }
    std::string_view equality::get_name() const { return commands::names[type_index]; }

    const size_t type_equality::type_index = commands::values::type_equality;
    type_equality::type_equality(const interface* childs) noexcept : children_interface(childs) {}
    type_equality::~type_equality() noexcept {}
    object type_equality::process(context* ctx) const {
      object obj = ignore_value;
      auto cur = childs;
      for (; cur != nullptr && obj.ignore(); cur = cur->next) {
        obj = cur->process(ctx);
      }

      for (; cur != nullptr; cur = cur->next) {
        const auto tmp = cur->process(ctx);
        if (!tmp.ignore() && !tmp.lazy_type_compare(obj)) return object(false);
        //obj = tmp; // зачем, если мы проверяем равенство?
      }

      return object(true);
    }

    local_state* type_equality::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* first = nullptr;
      local_state* children = nullptr;
      {
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          auto s = cur->compute(ctx, allocator);
          if (first == nullptr) first = s;
          if (children != nullptr) children->next = s;
          children = s;
        }
      }

      ptr->children = first;
      ptr->func = [] (const local_state* s) -> object {
        object obj = ignore_value;
        auto cur = s->children;
        for (; cur != nullptr && obj.ignore(); cur = cur->next) {
          obj = cur->value;
        }

        if (obj.unresolved()) return unresolved_value;

        for (; cur != nullptr; cur = cur->next) {
          const auto &tmp = cur->value;
          if (tmp.unresolved()) return unresolved_value;
          if (!tmp.ignore() && !tmp.lazy_type_compare(obj)) return object(false);
        }

        return object(true);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void type_equality::draw(context* ctx) const {
//       {
//         const auto obj = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = obj;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       // нужно ли тут обходить детей? может и нужно, но как это дело рисовать не ясно
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->draw(ctx); }
//     }

    size_t type_equality::get_type_id() const { return type_id<object>(); }
    std::string_view type_equality::get_name() const { return commands::names[type_index]; }

    const size_t compare::type_index = commands::values::compare;
    compare::compare(const uint8_t op, const interface* childs) noexcept : children_interface(childs), op(op) {}
    compare::~compare() noexcept {}
    object compare::process(context* ctx) const {
      // возможно нужно сделать последовательное сравнение
      // тип 3 > 2 > 1, чем 3 > 2 и 3 > 1, но это открытый вопрос

      object obj = ignore_value;
      auto cur = childs;
      for (; cur != nullptr && obj.ignore(); cur = cur->next) {
        obj = cur->process(ctx);
      }

      if (obj.ignore()) return ignore_value;

      bool ret = true;
      const double first_val = obj.get<double>();
      for (; cur != nullptr && ret; cur = cur->next) {
        const auto obj = cur->process(ctx);
        if (obj.ignore()) continue;
        const double val = obj.get<double>();
        ret = compare_func(op, first_val, val);
      }

      return object(ret);
    }

    local_state* compare::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* first = nullptr;
      local_state* children = nullptr;
      {
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          auto s = cur->compute(ctx, allocator);
          if (first == nullptr) first = s;
          if (children != nullptr) children->next = s;
          children = s;
        }
      }

      ptr->children = first;
      ptr->operator_type = op;
      ptr->func = [] (const local_state* s) -> object {
        object obj = ignore_value;
        auto cur = s->children;
        for (; cur != nullptr && obj.ignore(); cur = cur->next) {
          obj = cur->value;
        }

        if (obj.ignore()) return ignore_value;
        if (obj.unresolved()) return unresolved_value;

        bool ret = true;
        const double first_val = obj.get<double>();
        for (; cur != nullptr && ret; cur = cur->next) {
          const auto obj = cur->value;
          if (obj.ignore()) continue;
          if (obj.unresolved()) return unresolved_value;
          const double val = obj.get<double>();
          ret = compare_func(s->operator_type, first_val, val);
        }

        return object(ret);
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

//     void compare::draw(context* ctx) const {
//       {
//         const auto obj = childs->process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = process(ctx);
//         dd.original = obj;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t compare::get_type_id() const { return type_id<object>(); }
    std::string_view compare::get_name() const { return commands::names[type_index]; }

    struct save_set_on_stack {
      context* ctx;
      phmap::flat_hash_set<object> mem;

      inline save_set_on_stack(context* ctx) : ctx(ctx) { std::swap(mem, ctx->unique_objects); }
      inline ~save_set_on_stack() { std::swap(ctx->unique_objects, mem); }
    };

    const size_t keep_set::type_index = commands::keep_set;
    keep_set::keep_set(const interface* child) noexcept : one_child_interface(child) {}
    keep_set::~keep_set() noexcept {}
    struct object keep_set::process(context* ctx) const {
      save_set_on_stack ssos(ctx);
      return child->process(ctx);
    }

    local_state* keep_set::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      save_set_on_stack ssos(ctx);
      auto c = child->compute(ctx, allocator);

      // как тут правильно сделать сохранение стека? хороший вопрос
      // пока что незнаю, либо придется переделывать все сохраненные функции
      // чтобы они рекурсивно пересчитывали результат и там собственно сохранять в стеке
      // либо делать отдельную функцию которая запомнит это все дело самостоятельно
      // первое предпочтительно, но будет не особо полезно для итераторов
      // можно в принципе сделать второе, но позже
      ptr->children = c;
      ptr->value = c->value;
      return ptr;
    }

    // нужно ли это рисовать? наверное все нужно рисовать
//     void keep_set::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         if (!ctx->draw(&dd)) return;
//       }
//
//       save_set_on_stack ssos(ctx);
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       child->draw(ctx);
//     }

    size_t keep_set::get_type_id() const { return type_id<object>(); }
    std::string_view keep_set::get_name() const { return commands::names[type_index]; }

    const size_t is_unique::type_index = commands::is_unique;
    is_unique::is_unique(const interface* value) noexcept : one_child_interface(value) {}
    is_unique::~is_unique() noexcept {}
    struct object is_unique::process(context* ctx) const {
      auto obj = ctx->current;
      if (child != nullptr) obj = child->process(ctx);
      if (!obj.valid() || obj.ignore()) return ignore_value;
      const auto itr = ctx->unique_objects.find(obj);
      return object(itr == ctx->unique_objects.end());
    }

    local_state* is_unique::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto value_s = child != nullptr ? child->compute(ctx, allocator) : nullptr;
      if (value_s != nullptr) value_s->argument_name = "value";

      object ret;
      {
        auto obj = ctx->current;
        if (value_s != nullptr) obj = value_s->value;
        if (!obj.valid() || obj.ignore()) ret = ignore_value;
        else if (obj.unresolved()) ret = unresolved_value;
        else {
          const auto itr = ctx->unique_objects.find(obj);
          ret = object(itr == ctx->unique_objects.end());
        }
      }

      ptr->children = value_s;
      ptr->value = ret;
      // пока что без функции
      return ptr;
    }

//     void is_unique::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = process(ctx);
//         dd.original = value == nullptr ? ctx->current : value->process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       if (value != nullptr) value->draw(ctx);
//     }

    size_t is_unique::get_type_id() const { return type_id<object>(); }
    std::string_view is_unique::get_name() const { return commands::names[type_index]; }

    const size_t place_in_set::type_index = commands::place_in_set;
    place_in_set::place_in_set(const interface* value) noexcept : one_child_interface(value) {}
    place_in_set::~place_in_set() noexcept {}
    struct object place_in_set::process(context* ctx) const {
      auto obj = ctx->current;
      if (child != nullptr) obj = child->process(ctx);
      if (!obj.valid() || obj.ignore()) return ignore_value;
      ctx->unique_objects.insert(obj);
      return ignore_value;
    }

    local_state* place_in_set::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto value_s = child != nullptr ? child->compute(ctx, allocator) : nullptr;
      if (value_s != nullptr) value_s->argument_name = "value";

      {
        auto obj = ctx->current;
        if (value_s != nullptr) obj = value_s->value;
        if (!obj.valid() || obj.ignore()) ptr->value = ignore_value;
        else if (obj.unresolved()) ptr->value = unresolved_value;
        else { ctx->unique_objects.insert(obj); ptr->value = ignore_value; }
      }

      ptr->children = value_s;
      return ptr;
    }

    // для каждой функции которую можно вызвать в draw, нужно делать дополнительную функцию которая
    // вернет все объекты с вызова детей, чтобы функции детей вызывались один раз
//     void place_in_set::draw(context* ctx) const {
//       process(ctx); // возможно имеет смысл делать
//
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = value == nullptr ? ctx->current : value->process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       if (value != nullptr) value->draw(ctx);
//     }

    size_t place_in_set::get_type_id() const { return type_id<object>(); }
    std::string_view place_in_set::get_name() const { return commands::names[type_index]; }

    const size_t place_in_set_if_unique::type_index = commands::place_in_set_if_unique;
    place_in_set_if_unique::place_in_set_if_unique(const interface* value) noexcept : one_child_interface(value) {}
    place_in_set_if_unique::~place_in_set_if_unique() noexcept {}
    struct object place_in_set_if_unique::process(context* ctx) const {
      auto obj = ctx->current;
      if (child != nullptr) obj = child->process(ctx);
      if (!obj.valid() || obj.ignore()) return ignore_value;
      const auto itr = ctx->unique_objects.find(obj);
      ctx->unique_objects.insert(obj);
      return object(itr == ctx->unique_objects.end());
    }

    local_state* place_in_set_if_unique::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto value_s = child != nullptr ? child->compute(ctx, allocator) : nullptr;
      if (value_s != nullptr) value_s->argument_name = "value";

      {
        auto obj = ctx->current;
        if (value_s != nullptr) obj = value_s->value;
        if (!obj.valid() || obj.ignore()) ptr->value = ignore_value;
        else if (obj.unresolved()) ptr->value = unresolved_value;
        else {
          const auto itr = ctx->unique_objects.find(obj);
          ctx->unique_objects.insert(obj);
          ptr->value = object(itr == ctx->unique_objects.end());
        }
      }

      ptr->children = value_s;
      return ptr;
    }

//     void place_in_set_if_unique::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = process(ctx);
//         dd.original = value == nullptr ? ctx->current : value->process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, get_name());
//       change_nesting cn(ctx, ctx->nest_level+1);
//       if (value != nullptr) value->draw(ctx);
//     }
    size_t place_in_set_if_unique::get_type_id() const { return type_id<object>(); }
    std::string_view place_in_set_if_unique::get_name() const { return commands::names[type_index]; }

    // в текущем объекте к нам должен был придти новый контекст
    // меняем его пока не достигнем последнего ребенка,
    // в последнем ребенке должен находиться интересующий нас объект
    // как можно проверить при создании? такое поведение задается с помощью строки
    // при парсинге строки будет видно что возвращается
    complex_object::complex_object(const interface* childs) noexcept : children_interface(childs) {}
    complex_object::~complex_object() noexcept {}
    object complex_object::process(context* ctx) const {
      change_scope sc(ctx, ctx->current, ctx->prev);

      // что делать с игнор здесь? может ли он вообще тут быть? скорее нет
      for (auto cur = childs; cur != nullptr && !ctx->current.ignore(); cur = cur->next) {
        const auto &obj = cur->process(ctx);
        ctx->prev = ctx->current;
        ctx->current = obj;
      }

      assert(ctx->current.valid());
      if (ctx->current.ignore()) throw std::runtime_error("Complex value evaluates to ignore value, context: " + std::string(ctx->id));
      return ctx->current;
    }

    local_state* complex_object::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      local_state* first = nullptr;
      local_state* children = nullptr;
      {
        change_scope sc(ctx, ctx->current, ctx->prev);
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          auto s = cur->compute(ctx, allocator);
          ctx->prev = ctx->current;
          ctx->current = s->value.ignore() ? unresolved_value : s->value;

          if (first == nullptr) first = s;
          if (children != nullptr) children->next = s;
          children = s;
        }
      }

      ptr->children = first;
      ptr->func = &get_last_value;
      ptr->value = ptr->func(ptr);
      return ptr;
    }

    // тут явно не нужно рисовать детей
//     void complex_object::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t complex_object::get_type_id() const { return type_id<object>(); }
    std::string_view complex_object::get_name() const { return "complex_value"; }

    const size_t invalid::type_index = commands::invalid;
    struct object invalid::process(context*) const { return object(); }
    local_state* invalid::compute(context* ctx, local_state_allocator* allocator) const { auto ptr = allocator->create(ctx, get_name()); return ptr; }
    std::string_view invalid::get_name() const { return commands::names[type_index]; }
    const size_t ignore::type_index = commands::ignore;
    struct object ignore::process(context*) const { return ignore_value; }
    local_state* ignore::compute(context* ctx, local_state_allocator* allocator) const { auto ptr = allocator->create(ctx, get_name()); ptr->value = ignore_value; return ptr; }
    std::string_view ignore::get_name() const { return commands::names[type_index]; }
    const size_t root::type_index = commands::values::root;
    struct object root::process(context* ctx) const { return ctx->root; }
    size_t root::get_type_id() const { return type_id<object>(); }
    std::string_view root::get_name() const { return commands::names[type_index]; }
    const size_t prev::type_index = commands::values::prev;
    struct object prev::process(context* ctx) const { return ctx->prev; }
    size_t prev::get_type_id() const { return type_id<object>(); }
    std::string_view prev::get_name() const { return commands::names[type_index]; }
    const size_t current::type_index = commands::values::current;
    struct object current::process(context* ctx) const { return ctx->current; }
    size_t current::get_type_id() const { return type_id<object>(); }
    std::string_view current::get_name() const { return commands::names[type_index]; }

    local_state* root::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = ctx->root;
      return ptr;
    }

    local_state* prev::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = ctx->prev;
      return ptr;
    }

    local_state* current::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = ctx->current;
      return ptr;
    }

//     void root::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = process(ctx);
//       ctx->draw(&dd);
//     }
//
//     void prev::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = process(ctx);
//       ctx->draw(&dd);
//     }
//
//     void current::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = process(ctx);
//       ctx->draw(&dd);
//     }

    const size_t index::type_index = commands::index;
    struct object index::process(context* ctx) const { return object(double(ctx->index)); }
    size_t index::get_type_id() const { return type_id<object>(); }
    std::string_view index::get_name() const { return commands::names[type_index]; }

    const size_t prev_index::type_index = commands::prev_index;
    struct object prev_index::process(context* ctx) const { return object(double(ctx->prev_index)); }
    size_t prev_index::get_type_id() const { return type_id<object>(); }
    std::string_view prev_index::get_name() const { return commands::names[type_index]; }

    local_state* index::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = double(ctx->index);
      return ptr;
    }

    local_state* prev_index::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = double(ctx->prev_index);
      return ptr;
    }

//     void index::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = process(ctx);
//       ctx->draw(&dd);
//     }
//
//     void prev_index::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = process(ctx);
//       ctx->draw(&dd);
//     }

    const size_t value::type_index = commands::value;
    value::value(const interface* child) noexcept : one_child_interface(child) {}
    value::~value() noexcept {}
    struct object value::process(context* ctx) const { return child->process(ctx); }
    local_state* value::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      ptr->children = child->compute(ctx, allocator);
      ptr->value = ptr->children->value;
      return ptr;
    }
//     void value::draw(context* ctx) const { child->draw(ctx); }
    size_t value::get_type_id() const { return type_id<object>(); }
    std::string_view value::get_name() const { return commands::names[type_index]; }

    const size_t get_context::type_index = commands::context;
    get_context::get_context(const std::string str) noexcept : name(std::move(str)) {}
    get_context::get_context(const std::string_view str) noexcept : name(str) {}
    get_context::~get_context() noexcept { std::cout << "~get_context()\n"; }
    object get_context::process(context* ctx) const {
      const auto itr = ctx->map.find(name);
      if (itr == ctx->map.end()) throw std::runtime_error("Could not find context object using key " + name);
      return itr->second;
    }

    local_state* get_context::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = process(ctx);
      ptr->original = name;
      return ptr;
    }

//     void get_context::draw(context* ctx) const {
//       const auto obj = process(ctx);
//       // тут нам нужно как то словестно описать что мы получили на основе типа
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = obj;
//       dd.original = name;
//       ctx->draw(&dd);
//     }

    size_t get_context::get_type_id() const { return type_id<object>(); }
    std::string_view get_context::get_name() const { return commands::names[type_index]; }

    const size_t save_local::type_index = commands::values::save_local;
    save_local::save_local(const std::string name, const size_t index, const interface* var) noexcept
      : one_child_interface(var), name(std::move(name)), index(index) {}
    save_local::save_local(const std::string_view name, const size_t index, const interface* var) noexcept
      : one_child_interface(var), name(name), index(index) {}
    save_local::~save_local() noexcept {}
    object save_local::process(context* ctx) const {
      auto obj = ctx->current;
      if (child != nullptr) obj = child->process(ctx);

      // неочевидно почему нельзя
      //if (obj.ignore()) throw std::runtime_error("Trying to save ignore value to local '" + name + "'");

      // ошибка? ну было бы неплохо перезапись чекать
      // возможно имеет смысл вообще отменить перезапись в этих функциях
      // хотя зачем?
      const auto &local_obj = ctx->get_local(index);
      if (local_obj.valid() && !local_obj.ignore() && !obj.lazy_type_compare(local_obj))
        throw std::runtime_error("Rewriting local variable " + name + " with different type variable is considered as error");

      ctx->save_local(index, obj);
      //ctx->locals[index] = obj;

      return ignore_value;
    }

    local_state* save_local::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      auto val_s = child != nullptr ? child->compute(ctx, allocator) : nullptr;

      auto obj = ctx->current;
      if (child != nullptr) {
        obj = val_s->value;
        val_s->argument_name = "value";
      }

      //if (obj.ignore()) throw std::runtime_error("Trying to save ignore value to local '" + name + "'");

      const auto &local_obj = ctx->get_local(index);
      if (local_obj.valid() && !local_obj.ignore() && !obj.lazy_type_compare(local_obj))
        throw std::runtime_error("Rewriting local variable " + name + " with different type variable is considered as error");

      ctx->save_local(index, obj);
      //ctx->locals[index] = obj;

      ptr->children = val_s;
      ptr->original = name;
      ptr->value = obj.unresolved() ? unresolved_value : ignore_value;
      return ptr;
    }

//     void save_local::draw(context* ctx) const {
//       auto obj = ctx->current;
//       if (var != nullptr) obj = var->process(ctx);
//
//       // ошибка? ну было бы неплохо перезапись чекать
//       if (ctx->locals[index].valid() && !ctx->locals[index].ignore() && !obj.lazy_type_compare(ctx->locals[index]))
//         throw std::runtime_error("Rewriting local variable " + name + " with different type variable is considered as error");
//       ctx->locals[index] = obj;
//
//       local_state dd(ctx);
//       dd.value = obj;
//       dd.original = name;
//       dd.function_name = get_name();
//       ctx->draw(&dd);
//     }

    size_t save_local::get_type_id() const { return type_id<object>(); }
    std::string_view save_local::get_name() const { return commands::names[type_index]; }

    const size_t has_local::type_index = commands::values::has_local;
    has_local::has_local(const size_t &index) noexcept : index(index) {}
    has_local::~has_local() noexcept {}
    object has_local::process(context* ctx) const {
      const auto &local_obj = ctx->get_local(index);
      return object(local_obj.valid() && !local_obj.ignore());
    }

    local_state* has_local::compute(context* ctx, local_state_allocator* allocator) const {
      const auto &local_obj = ctx->get_local(index);
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = object(local_obj.valid() && !local_obj.ignore());
      return ptr;
    }

//     void has_local::draw(context* ctx) const {
//       const auto obj = process(ctx);
//       local_state dd(ctx);
//       dd.value = obj;
//       //dd.original = name; // наверное надо бы имя оставить
//       dd.function_name = get_name();
//       ctx->draw(&dd);
//     }

    size_t has_local::get_type_id() const { return type_id<object>(); }
    std::string_view has_local::get_name() const { return commands::names[type_index]; }

    const size_t remove_local::type_index = commands::values::remove_local;
    remove_local::remove_local(const std::string name, const size_t index) noexcept : name(std::move(name)), index(index) {}
    remove_local::remove_local(const std::string_view name, const size_t index) noexcept : name(name), index(index) {}
    remove_local::~remove_local() noexcept {}
    object remove_local::process(context* ctx) const {
      ctx->remove_local(index);
//       ctx->locals[index] = object();
      return ignore_value;
    }

    local_state* remove_local::compute(context* ctx, local_state_allocator* allocator) const {
      ctx->remove_local(index);
//       ctx->locals[index] = object();
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = ignore_value;
      return ptr;
    }

//     void remove_local::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.original = object(name);
//       dd.function_name = commands::names[type_index];
//       ctx->draw(&dd);
//     }

    size_t remove_local::get_type_id() const { return type_id<object>(); }
    std::string_view remove_local::get_name() const { return commands::names[type_index]; }

    const size_t get_local::type_index = commands::local;
    get_local::get_local(const std::string name, const size_t index) noexcept : name(std::move(name)), index(index) {}
    get_local::get_local(const std::string_view name, const size_t index) noexcept : name(name), index(index) {}
    get_local::~get_local() noexcept {}
    struct object get_local::process(context* ctx) const {
      const auto &obj = ctx->get_local(index);
      if (!obj.valid() || obj.ignore()) throw std::runtime_error("Local variable " + name + " is invalid");
      return obj;
    }

    local_state* get_local::compute(context* ctx, local_state_allocator* allocator) const {
      const auto &obj = ctx->get_local(index);
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = !obj.valid() || obj.ignore() ? unresolved_value : obj;
      return ptr;
    }

//     void get_local::draw(context* ctx) const {
//       const auto obj = process(ctx);
//       local_state dd(ctx);
//       dd.value = obj;
//       dd.original = object(name);
//       dd.function_name = get_name();
//       ctx->draw(&dd);
//     }

    size_t get_local::get_type_id() const { return type_id<object>(); }
    std::string_view get_local::get_name() const { return commands::names[type_index]; }

    const size_t save::type_index = commands::save;
    save::save(const std::string str, const interface* var) noexcept : one_child_interface(var), name(std::move(str)) {}
    save::save(const std::string_view str, const interface* var) noexcept : one_child_interface(var), name(str) {}
    save::~save() noexcept {}
    // перезапись?
    struct object save::process(context* ctx) const {
      auto obj = ctx->current;
      if (child != nullptr) obj = child->process(ctx);
      // можно ли вообще перезаписывать данные в контексте? вообще у нас строго заданы обычно root, prev, current и от них довольно сильно зависит скрипт
      // так что наверное почему нет
      if (const auto itr = ctx->map.find(name); itr != ctx->map.end()) {
        const auto &local_obj = itr->second;
        if (local_obj.valid() && !local_obj.ignore() && !obj.lazy_type_compare(local_obj))
          throw std::runtime_error("Rewriting context variable '" + name + "' with different type variable is considered as error");
      }
      ctx->map.emplace(name, obj);
      return ignore_value;
    }

    local_state* save::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      auto val_s = child != nullptr ? child->compute(ctx, allocator) : nullptr;

      auto obj = ctx->current;
      if (child != nullptr) {
        obj = val_s->value;
        val_s->argument_name = "value";
      }

      ptr->children = val_s;
      ptr->value = obj;
      ptr->original = name;
      return ptr;
    }

    //void draw(context* ctx) const override;
    size_t save::get_type_id() const { return type_id<object>(); }
    std::string_view save::get_name() const { return commands::names[type_index]; }

    const size_t remove::type_index = commands::remove;
    remove::remove(const std::string str) noexcept : name(std::move(str)) {}
    remove::remove(const std::string_view str) noexcept : name(str) {}
    remove::~remove() noexcept {}
    struct object remove::process(context* ctx) const {
      ctx->map.erase(name);
      return ignore_value;
    }

    local_state* remove::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = process(ctx);
      ptr->original = name;
      return ptr;
    }

    //void draw(context* ctx) const override;
    size_t remove::get_type_id() const { return type_id<object>(); }
    std::string_view remove::get_name() const { return commands::names[type_index]; }

    const size_t has_context::type_index = commands::has_context;
    has_context::has_context(const std::string str) noexcept : name(std::move(str)) {}
    has_context::has_context(const std::string_view str) noexcept : name(str) {}
    has_context::~has_context() noexcept {}
    struct object has_context::process(context* ctx) const {
      const auto itr = ctx->map.find(name);
      if (itr != ctx->map.end()) return object(itr->second.valid() && !itr->second.ignore());
      return object(false);
    }

    local_state* has_context::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = process(ctx);
      ptr->original = name;
      return ptr;
    }

    //void draw(context* ctx) const override;
    size_t has_context::get_type_id() const { return type_id<object>(); }
    std::string_view has_context::get_name() const { return commands::names[type_index]; }

    const size_t get_list::type_index = commands::list;
    get_list::get_list(const std::string name) noexcept : name(std::move(name)) {}
    get_list::get_list(const std::string_view name) noexcept : name(name) {}
    get_list::~get_list() noexcept {}
    struct object get_list::process(context* ctx) const {
      const auto itr = ctx->object_lists.find(name);
      // может пустой массив возвращать?
      if (itr == ctx->object_lists.end()) throw std::runtime_error("Could not find list '" + name + "'");
      //utils::array_view av(itr->second);
      std::span s(itr->second);
      return object(s);
    }

    local_state* get_list::compute(context* ctx, local_state_allocator* allocator) const {
      const auto itr = ctx->object_lists.find(name);
      // может пустой массив возвращать?
      if (itr == ctx->object_lists.end()) throw std::runtime_error("Could not find list '" + name + "'");
      //utils::array_view av(itr->second);
      std::span s(itr->second);
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = s;
      ptr->original = name;
      return ptr;
    }

    static_assert(!is_optional_v<std::span<int>>);
    //static_assert(!utils::is_array_view_v<std::span<int>>);
    static_assert(is_span_v<std::span<int>>);
    static_assert(is_span_v<std::span<int, 28>>);

    //void draw(context* ctx) const override;
    size_t get_list::get_type_id() const { return type_id<object>(); }
    std::string_view get_list::get_name() const { return commands::names[type_index]; }

    const size_t add_to_list::type_index = commands::add_to_list;
    add_to_list::add_to_list(const std::string &name) noexcept : name(name) {}
    add_to_list::~add_to_list() noexcept {}
    struct object add_to_list::process(context* ctx) const {
      const auto cur = ctx->current;
      ctx->object_lists[name].push_back(cur);
      return ignore_value;
    }

    // или не добавлять? не, надо бы, даже если там unresolved_value
    local_state* add_to_list::compute(context* ctx, local_state_allocator* allocator) const {
      const auto cur = ctx->current;
      ctx->object_lists[name].push_back(cur);
      auto ptr = allocator->create(ctx, get_name());
      ptr->value = ignore_value;
      return ptr;
    }

//     void add_to_list::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.original = object(name);
//       dd.function_name = get_name();
//       ctx->draw(&dd);
//     }

    size_t add_to_list::get_type_id() const { return type_id<object>(); }
    std::string_view add_to_list::get_name() const { return commands::names[type_index]; }

    static bool has_in_list(const std::vector<object> &objs, const object &obj) {
      for (const auto &local : objs) {
        if (local == obj) return true;
      }
      return false;
    }

    const size_t is_in_list::type_index = commands::is_in_list;
    is_in_list::is_in_list(const std::string &name) noexcept : name(name) {}
    is_in_list::~is_in_list() noexcept {}
    struct object is_in_list::process(context* ctx) const {
      const auto cur = ctx->current;
      const auto itr = ctx->object_lists.find(name);
      if (itr == ctx->object_lists.end()) return object(false);
      return object(has_in_list(itr->second, cur));
    }

    local_state* is_in_list::compute(context* ctx, local_state_allocator* allocator) const {
      object val;
      const auto itr = ctx->object_lists.find(name);
      if (itr == ctx->object_lists.end()) val = object(false);
      else val = object(has_in_list(itr->second, ctx->current));

      auto ptr = allocator->create(ctx, get_name());
      ptr->value = val;
      return ptr;
    }

//     void is_in_list::draw(context* ctx) const {
//       const auto obj = process(ctx);
//       local_state dd(ctx);
//       dd.value = obj;
//       dd.original = object(name);
//       dd.function_name = get_name();
//       ctx->draw(&dd);
//     }

    size_t is_in_list::get_type_id() const { return type_id<object>(); }
    std::string_view is_in_list::get_name() const { return commands::names[type_index]; }

    const size_t has_in_list::type_index = commands::has_in_list;
    has_in_list::has_in_list(const std::string &name, const interface* max_count, const interface* percentage, const interface* childs) noexcept
      : additional_child_interface(max_count), one_child_interface(percentage), children_interface(childs), name(name)
    {}

    has_in_list::~has_in_list() noexcept {}

    struct object has_in_list::process(context* ctx) const {
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) return object(0.0);

      auto percentage = child;
      auto max_count = additional;
      size_t final_max_count = SIZE_MAX;
      if (percentage != nullptr) {
        const auto val = percentage->process(ctx);
        const double final_percent = val.get<double>();
        if (final_percent < 0.0) throw std::runtime_error(std::string(name) + " percentage cannot be less than zero");
        const size_t counter = list_itr->second.size();
        final_max_count = counter * final_percent;
      } else if (max_count != nullptr) {
        const auto val = max_count->process(ctx);
        const double v = val.get<double>();
        if (v < 0.0) throw std::runtime_error(std::string(name) + " count cannot be less than zero");
        final_max_count = v;
      }

      if (final_max_count == 0) return object(0.0);

      size_t val = 0;
      change_scope cs(ctx, ctx->current, ctx->prev);
      change_indices ci(ctx, 0, ctx->index);
      ctx->prev = ctx->current;
      for (size_t i = 0; i < list_itr->second.size() && i < final_max_count; ++i) {
        const auto obj = list_itr->second[i];
        ctx->current = obj;
        ctx->index = i;

        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          const auto &obj = cur->process(ctx);
          const bool ret = obj.ignore() ? true : obj.get<bool>();
          val += size_t(ret);
        }
      }

      return object(double(val));
    }

    static struct object compute_every_numeric_child_value(const local_state* s) {
      bool unresolved = false;
      double val = 0;
      for (auto c = s->children; c != nullptr && !unresolved; c = c->next) {
        if (c->value.unresolved()) unresolved = true;
        else val += c->value.ignore() ? 0.0 : c->value.get<double>();
      }

      return unresolved ? unresolved_value : val;
    }

    static struct object compute_every_logic_child_value(const local_state* s) {
      bool unresolved = false;
      bool val = true;
      for (auto c = s->children; c != nullptr && !unresolved && val; c = c->next) {
        if (c->value.unresolved()) unresolved = true;
        else val = val && c->value.ignore() ? true : c->value.get<bool>();
      }

      return unresolved ? unresolved_value : val;
    }

    static struct object compute_every_numeric_value(const local_state* s) {
      bool unresolved = false;
      double val = 0;
      for (auto c = s->children; c != nullptr && !unresolved; c = c->next) {
        auto cond_val = c->children->value;
        if (!cond_val.ignore() && cond_val.get<bool>()) {
          if (c->value.unresolved()) unresolved = true;
          else val += c->value.get<double>();
        }
      }

      return unresolved ? unresolved_value : val;
    }

    static struct object compute_every_logic_value(const local_state* s) {
      bool unresolved = false;
      bool val = true;
      for (auto c = s->children; c != nullptr && !unresolved && val; c = c->next) {
        auto cond_val = c->children->value;
        if (!cond_val.ignore() && cond_val.get<bool>()) {
          if (c->value.unresolved()) unresolved = true;
          else val = val && c->value.get<bool>();
        }
      }

      return unresolved ? unresolved_value : val;
    }

    local_state* has_in_list::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->original = name;

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) ptr->value = object(0.0);
      else {
        local_state* first_child = nullptr;
        local_state* children = nullptr;
        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        for (size_t i = 0; i < list_itr->second.size(); ++i) {
          const auto obj = list_itr->second[i];
          ctx->current = obj;
          ctx->index = i;

          local_state* l_first_child = nullptr;
          local_state* l_children = nullptr;
          auto ptr = allocator->create(ctx, get_name());
          for (auto child = childs; child != nullptr; child = child->next) {
            auto c = child->compute(ctx, allocator);

            if (l_first_child == nullptr) l_first_child = c;
            if (l_children != nullptr) l_children->next = c;
            l_children = c;
          }

          ptr->func = &compute_every_logic_child_value;
          ptr->value = compute_every_logic_child_value(ptr);

          if (first_child == nullptr) first_child = ptr;
          if (children != nullptr) children->next = ptr;
          children = ptr;
        }

        auto percentage = child;
        auto max_count = additional;
        local_state* arg = nullptr;
        if (percentage != nullptr) {
          arg = percentage->compute(ctx, allocator);
          arg->argument_name = "percentage";
        } else if (max_count != nullptr) {
          arg = max_count->compute(ctx, allocator);
          arg->argument_name = "count";
        }

        if (arg != nullptr) {
          arg->next = first_child;
          first_child = arg;
        }

        ptr->children = first_child;
        ptr->func = [] (const local_state* s) -> object {
          size_t final_max_count = SIZE_MAX;
          auto arg = s->children;
          if (arg->argument_name == "count") {
            if (!arg->value.unresolved()) {
              const double v = arg->value.get<double>();
              if (v < 0.0) throw std::runtime_error("'count' cannot be less than zero");
              final_max_count = v;
            } else final_max_count = SIZE_MAX-1;
          } else if (arg->argument_name == "percentage") {
            if (!s->current.unresolved() && !arg->value.unresolved()) {
              const double final_percent = arg->value.get<double>();
              if (final_percent < 0.0) throw std::runtime_error("'percentage' cannot be less than zero");
              size_t counter = 0;
              for (auto c = arg->next; c != nullptr; c = c->next) { ++counter; }
              final_max_count = counter * final_percent;
            } else final_max_count = SIZE_MAX-1;
          }

          if (final_max_count == SIZE_MAX-1) return unresolved_value;

          const bool no_args = arg->argument_name.empty();
          bool unresolved = false;
          size_t val = 0;
          for (auto c = no_args ? arg : arg->next; c != nullptr && !unresolved && val < final_max_count; c = c->next) {
            if (c->value.unresolved()) unresolved = true;
            else val += c->value.get<bool>();
          }

          return unresolved ? unresolved_value : object(double(val));
        };
        ptr->value = ptr->func(ptr);
      }

      return ptr;
    }

//     void has_in_list::draw(context* ctx) const {
//       const auto list_itr = ctx->object_lists.find(name);
//       if (list_itr == ctx->object_lists.end() || list_itr->second.empty()) {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = object(name);
//         ctx->draw(&dd);
//         return;
//       }
//
//       const auto value = process(ctx);
//
//       {
//         object arg_val;
//         if (percentage != nullptr)     arg_val = percentage->process(ctx);
//         else if (max_count != nullptr) arg_val = max_count->process(ctx);
//
//         const bool has_arg = percentage != nullptr || max_count != nullptr;
//         local_state arg(ctx);
//         arg.argument_name = percentage != nullptr ? "percentage" : "count";
//         arg.value = arg_val;
//
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = value;
//         dd.original = object(name);
//         dd.children = has_arg ? &arg : nullptr;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       const object first = list_itr->second[0];
//       if (!first.valid()) return;
//
//       change_nesting cn(ctx, ++ctx->nest_level);
//       change_scope cs(ctx, first, ctx->current);
//       change_function_name cfn(ctx, get_name());
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t has_in_list::get_type_id() const { return type_id<object>(); }
    std::string_view has_in_list::get_name() const { return commands::names[type_index]; }

    const size_t random_in_list::type_index = commands::random_in_list;
    random_in_list::random_in_list(const std::string &name, const size_t &state, const interface* condition, const interface* weight, const interface* childs) noexcept
      : condition_interface(condition), additional_child_interface(weight), children_interface(childs), name(name), state(state)
    {}

    random_in_list::~random_in_list() noexcept {}

    static struct object get_rand_obj(context* ctx, const std::span<object> &view, const interface* condition, const interface* weight, const size_t state, const std::string_view &func_name) {
      change_indices ci(ctx, 0, ctx->index);

      size_t counter = 0;
      double accum_weight = 0.0;
      std::vector<std::pair<struct object, double>> objects;
      objects.reserve(view.size());
      for (size_t i = 0; i < view.size(); ++i) {
        ctx->current = view[i];
        ctx->index = counter;
        ++counter;

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) continue;
        }

        object weight_val(1.0);
        if (weight != nullptr) {
          weight_val = weight->process(ctx);
        }

        const double local = weight_val.get<double>();
        if (local < 0.0) throw std::runtime_error(std::string(func_name) + " weights must not be less than zero");
        objects.emplace_back(view[i], local);
        accum_weight += local;
      }

      if (objects.size() == 0) return object();

      const uint64_t rand_val = ctx->get_random_value(state);
      const double rand = DEVILS_SCRIPT_FULL_NAMESPACE::context::normalize_value(rand_val) * accum_weight;
      double cumulative = 0.0;
      size_t index = 0;
      for (; index < objects.size() && cumulative <= rand; cumulative += objects[index].second, ++index) {}
      index -= 1;

      return objects[index].first;
    }

    struct object random_in_list::process(context* ctx) const {
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) return ignore_value;

      change_scope cs(ctx, object(), ctx->current);
      const auto obj = get_rand_obj(ctx, list_itr->second, condition, additional, state, get_name());
      ctx->current = obj;
      return !obj.ignore() ? childs->process(ctx) : ignore_value;
    }

    static void local_random_func(
      context* ctx,
      local_state_allocator* allocator,
      const std::string_view &name,
      const interface* condition,
      const interface* weight,
      const object &obj,
      local_state** first_child,
      local_state** children
    ) {
      ctx->current = obj;

      local_state* args = nullptr;
      if (condition != nullptr) {
        auto cond = condition->compute(ctx, allocator);
        cond->argument_name = "condition";
        args = cond;
      }

      if (weight != nullptr) {
        auto w = weight->compute(ctx, allocator);
        w->argument_name = "weight";
        if (args != nullptr) args->next = w;
        else args = w;
      }

      auto ptr = allocator->create(ctx, name);
      ptr->value = obj;
      ptr->children = args;
      // функция? ее нет

      if (*first_child == nullptr) *first_child = ptr;
      if (*children != nullptr) (*children)->next = ptr;
      *children = ptr;
    }

    static std::tuple<object, size_t> compute_random_value_raw(const local_state* s) {
      double accum_weight = 0.0;
      std::vector<std::pair<struct object, double>> objects;
      objects.reserve(50);

      for (auto c = s->children; c != nullptr; c = c->next) {
        object cond_obj(true);
        object weight_obj(1.0);
        if (c->children != nullptr && c->children->argument_name == "weight") {
          weight_obj = c->children->value;
        } else if (c->children != nullptr && c->children->argument_name == "condition") {
          cond_obj = c->children->value;
          weight_obj = c->children->next != nullptr ? c->children->next->value : weight_obj;
        }

        //if (cond_obj.ignore() || !cond_obj.get<bool>()) continue;
        if (cond_obj.unresolved()) return std::make_tuple(unresolved_value, 0);
        if (weight_obj.unresolved()) return std::make_tuple(unresolved_value, 0);

        const double w = cond_obj.ignore() || !cond_obj.get<bool>() ? 0.0 : weight_obj.get<double>();
        objects.push_back(std::make_pair(c->value, w));
        accum_weight += w;
      }

      const uint64_t rand_val = s->get_random_value(s->local_rand_state);
      const double rand = DEVILS_SCRIPT_FULL_NAMESPACE::context::normalize_value(rand_val) * accum_weight;
      double cumulative = 0.0;
      size_t index = 0;
      for (; index < objects.size() && cumulative <= rand; cumulative += objects[index].second, ++index) {}
      index -= 1;

      return std::make_tuple(objects[index].first, index);
    }

    static struct object compute_random_value(const local_state* s) {
      auto [ret, idx] = compute_random_value_raw(s);
      return ret;
    }

    local_state* random_in_list::compute(context* ctx, local_state_allocator* allocator) const {
      // нужно как то рандомный индекс сохранить, чтобы пройтись потом по правильному ребенку

      auto ptr = allocator->create(ctx, get_name());
      auto final_state = allocator->create(ctx, get_name());

      local_state* first_child = nullptr;
      local_state* children = nullptr;

      {
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        const auto list_itr = ctx->object_lists.find(name);
        if (list_itr == ctx->object_lists.end()) ptr->value = ignore_value;
        else {
          change_scope cs(ctx, object(), ctx->current);
          change_indices ci(ctx, 0, ctx->index);
          for (size_t i = 0; i < list_itr->second.size(); ++i) {
            const auto &cur = list_itr->second[i];
            ctx->current = cur;
            ctx->index = i;
            local_random_func(ctx, allocator, name, condition, additional, cur, &first_child, &children);
          }
        }
      }

      ptr->children = first_child;
      ptr->local_rand_state = state;
      ptr->func = &compute_random_value;
      const auto [val, idx] = compute_random_value_raw(ptr);
      ptr->value = val;
      ptr->original = object(double(idx));
      ptr->argument_name = "random";

      change_scope cs(ctx, ptr->value, ctx->current);
      auto childs_state = childs->compute(ctx, allocator);

      // мне еще детей посчитать
      final_state->children = ptr;
      final_state->children->next = childs_state;
      final_state->value = childs_state->value;
      return childs_state;
    }

//     void random_in_list::draw(context* ctx) const {
//       const auto list_itr = ctx->object_lists.find(name);
//       if (list_itr == ctx->object_lists.end() || list_itr->second.empty()) {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = object(name);
//         ctx->draw(&dd);
//         return;
//       }
//
//       auto obj = get_rand_obj(ctx, list_itr->second, condition, weight, state, get_name());
//       //if (!obj.valid()) return; // нам бы все равно что то нарисовать желательно
//       if (!obj.valid()) obj = list_itr->second[0];
//
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = obj;
//         dd.original = object(name);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_scope cs(ctx, obj, ctx->current);
//       change_nesting cn(ctx, ctx->nest_level+1);
//       change_function_name cfn(ctx, get_name());
// //       for (auto cur = childs; cur != nullptr; cur = cur->next) {
// //         cur->draw(ctx);
// //       }
//       childs->draw(ctx);
//     }

    size_t random_in_list::get_type_id() const { return type_id<object>(); }
    std::string_view random_in_list::get_name() const { return commands::names[type_index]; }

    const size_t every_in_list_numeric::type_index = commands::every_in_list;
    every_in_list_numeric::every_in_list_numeric(const std::string &name, const interface* condition, const interface* childs) noexcept
      : condition_interface(condition), children_interface(childs), name(name)
    {}

    every_in_list_numeric::~every_in_list_numeric() noexcept {}

    struct object every_in_list_numeric::process(context* ctx) const {
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) return object(0.0);
      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);

      double val = 0.0;
      for (size_t i = 0; i < list_itr->second.size(); ++i) {
        ctx->current = list_itr->second[i];
        ctx->index = i;

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) continue;
        }

        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          const auto &obj = cur->process(ctx);
          val += obj.ignore() ? 0.0 : obj.get<double>();
        }
      }

      return object(val);
    }

    static void local_every_numeric_func(
      context* ctx,
      local_state_allocator* allocator,
      const std::string_view &name,
      const interface* condition,
      const interface* childs,
      const object &obj,
      local_state** first_child,
      local_state** children
    ) {
      local_state* l_cond = nullptr;
      local_state* l_first_child = nullptr;
      local_state* l_children = nullptr;

      bool l_unresolved = false;
      double l_val = 0.0;
      auto ptr = allocator->create(ctx, name);
      if (condition != nullptr) l_cond = condition->compute(ctx, allocator);
      for (auto child = childs; child != nullptr; child = child->next) {
        auto c = child->compute(ctx, allocator);

        if (c->value.unresolved()) l_unresolved = true;
        else l_val += c->value.ignore() ? 0.0 : c->value.get<double>();

        if (l_first_child == nullptr) l_first_child = c;
        if (l_children != nullptr) l_children->next = c;
        l_children = c;
      }

      ptr->value = l_unresolved ? unresolved_value : l_val;
      ptr->original = obj;
      if (condition != nullptr) {
        l_cond->argument_name = "condition";
        l_cond->next = l_first_child;
        l_first_child = l_cond;
      }
      ptr->children = l_first_child;
      ptr->func = &compute_every_numeric_child_value;

      if (*first_child == nullptr) *first_child = ptr;
      if (*children != nullptr) (*children)->next = ptr;
      *children = ptr;
    }

    static void local_every_effect_func(
      context* ctx,
      local_state_allocator* allocator,
      const std::string_view &name,
      const interface* condition,
      const interface* childs,
      const object &obj,
      local_state** first_child,
      local_state** children
    ) {
      local_state* l_cond = nullptr;
      local_state* l_first_child = nullptr;
      local_state* l_children = nullptr;

      auto ptr = allocator->create(ctx, name);
      if (condition != nullptr) l_cond = condition->compute(ctx, allocator);
      for (auto child = childs; child != nullptr; child = child->next) {
        auto c = child->compute(ctx, allocator);

        if (l_first_child == nullptr) l_first_child = c;
        if (l_children != nullptr) l_children->next = c;
        l_children = c;
      }

      ptr->original = obj;
      if (condition != nullptr) {
        l_cond->argument_name = "condition";
        l_cond->next = l_first_child;
        l_first_child = l_cond;
      }
      ptr->children = l_first_child;
      //ptr->func = &compute_every_numeric_child_value; // в эффектах поди ничего

      if (*first_child == nullptr) *first_child = ptr;
      if (*children != nullptr) (*children)->next = ptr;
      *children = ptr;
    }

    static void local_every_logic_func(
      context* ctx,
      local_state_allocator* allocator,
      const std::string_view &name,
      const interface* condition,
      const interface* childs,
      const object &obj,
      local_state** first_child,
      local_state** children
    ) {
      local_state* l_cond = nullptr;
      local_state* l_first_child = nullptr;
      local_state* l_children = nullptr;

      bool l_unresolved = false;
      bool l_val = true;
      auto ptr = allocator->create(ctx, name);
      if (condition != nullptr) l_cond = condition->compute(ctx, allocator);
      for (auto child = childs; child != nullptr; child = child->next) {
        auto c = child->compute(ctx, allocator);

        if (c->value.unresolved()) l_unresolved = true;
        else l_val = l_val && c->value.ignore() ? true : c->value.get<bool>();

        if (l_first_child == nullptr) l_first_child = c;
        if (l_children != nullptr) l_children->next = c;
        l_children = c;
      }

      ptr->value = l_unresolved ? unresolved_value : l_val;
      ptr->original = obj;
      if (condition != nullptr) {
        l_cond->argument_name = "condition";
        l_cond->next = l_first_child;
        l_first_child = l_cond;
      }
      ptr->children = l_first_child;
      ptr->func = &compute_every_logic_child_value;

      if (*first_child == nullptr) *first_child = ptr;
      if (*children != nullptr) (*children)->next = ptr;
      *children = ptr;
    }

    local_state* every_in_list_numeric::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) ptr->value = ignore_value;
      else {
        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        for (size_t i = 0; i < list_itr->second.size(); ++i) {
          const auto &obj = list_itr->second[i];
          ctx->current = obj;
          ctx->index = i;
          local_every_numeric_func(ctx, allocator, name, condition, childs, obj, &first_child, &children);
        }
      }

      ptr->children = first_child;
      ptr->value = compute_every_numeric_value(ptr);
      ptr->func = &compute_every_numeric_value;

      return ptr;
    }

//     void every_in_list_numeric::draw(context* ctx) const {
//       const auto list_itr = ctx->object_lists.find(name);
//       if (list_itr == ctx->object_lists.end() || list_itr->second.empty()) {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = object(name);
//         ctx->draw(&dd);
//         return;
//       }
//
//       const auto val = process(ctx);
//
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = object(name);
//         dd.value = val;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       const object first = list_itr->second[0];
//       assert(first.valid());
//
//       change_scope cs(ctx, first, ctx->current);
//       change_function_name cfn(ctx, get_name());
//
//       if (condition != nullptr) {
//         draw_condition dc(ctx);
//         change_nesting cn(ctx, ctx->nest_level+1);
//         condition->draw(ctx);
//       }
//
//       change_nesting cn(ctx, ctx->nest_level+1);
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t every_in_list_numeric::get_type_id() const { return type_id<object>(); }
    std::string_view every_in_list_numeric::get_name() const { return commands::names[type_index]; }

    const size_t every_in_list_logic::type_index = commands::every_in_list;
    every_in_list_logic::every_in_list_logic(const std::string &name, const interface* condition, const interface* childs) noexcept :
      condition_interface(condition), children_interface(childs), name(name)
    {}

    every_in_list_logic::~every_in_list_logic() noexcept {}

    struct object every_in_list_logic::process(context* ctx) const {
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) return object(0.0);
      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);

      bool val = true;
      for (size_t i = 0; i < list_itr->second.size(); ++i) {
        ctx->current = list_itr->second[i];
        ctx->index = i;

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) continue;
        }

        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          const auto &obj = cur->process(ctx);
          const bool ret = obj.ignore() ? true : obj.get<bool>();
          val = val && ret;
        }
      }

      return object(val);
    }

    local_state* every_in_list_logic::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) ptr->value = ignore_value;
      else {
        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        for (size_t i = 0; i < list_itr->second.size(); ++i) {
          const auto &obj = list_itr->second[i];
          ctx->current = obj;
          ctx->index = i;
          local_every_logic_func(ctx, allocator, name, condition, childs, obj, &first_child, &children);
        }
      }

      ptr->children = first_child;
      ptr->value = compute_every_logic_value(ptr);
      ptr->func = &compute_every_logic_value;
      return ptr;
    }

//     void every_in_list_logic::draw(context* ctx) const {
//       const auto list_itr = ctx->object_lists.find(name);
//       if (list_itr == ctx->object_lists.end() || list_itr->second.empty()) {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = object(name);
//         ctx->draw(&dd);
//         return;
//       }
//
//       const auto val = process(ctx);
//
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = object(name);
//         dd.value = val;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       const object first = list_itr->second[0];
//       assert(first.valid());
//
//       change_scope cs(ctx, first, ctx->current);
//       change_function_name cfn(ctx, get_name());
//
//       if (condition != nullptr) {
//         draw_condition dc(ctx);
//         change_nesting cn(ctx, ++ctx->nest_level);
//         condition->draw(ctx);
//       }
//
//       change_nesting cn(ctx, ++ctx->nest_level);
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t every_in_list_logic::get_type_id() const { return type_id<object>(); }
    std::string_view every_in_list_logic::get_name() const { return commands::names[type_index]; }

    const size_t every_in_list_effect::type_index = commands::every_in_list;
    every_in_list_effect::every_in_list_effect(const std::string &name, const interface* condition, const interface* childs) noexcept :
      condition_interface(condition), children_interface(childs), name(name)
    {}

    every_in_list_effect::~every_in_list_effect() noexcept {}

    struct object every_in_list_effect::process(context* ctx) const {
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) return object(0.0);
      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);

      for (size_t i = 0; i < list_itr->second.size(); ++i) {
        ctx->current = list_itr->second[i];
        ctx->index = i;

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) continue;
        }

        for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->process(ctx); }
      }

      return object();
    }

    local_state* every_in_list_effect::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) ptr->value = ignore_value;
      else {
        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        for (size_t i = 0; i < list_itr->second.size(); ++i) {
          const auto &obj = list_itr->second[i];
          ctx->current = obj;
          ctx->index = i;
          local_every_effect_func(ctx, allocator, name, condition, childs, obj, &first_child, &children);
        }
      }

      ptr->children = first_child;
      return ptr;
    }

//     void every_in_list_effect::draw(context* ctx) const {
//       const auto list_itr = ctx->object_lists.find(name);
//       if (list_itr == ctx->object_lists.end() || list_itr->second.empty()) {
//         local_state dd(ctx);
//         dd.function_name = name;
//         dd.original = object(name);
//         ctx->draw(&dd);
//         return;
//       }
//
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = object(name);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       const object first = list_itr->second[0];
//       assert(first.valid());
//
//       change_scope cs(ctx, first, ctx->current);
//       change_function_name cfn(ctx, get_name());
//
//       if (condition != nullptr) {
//         draw_condition dc(ctx);
//         change_nesting cn(ctx, ++ctx->nest_level);
//         condition->draw(ctx);
//       }
//
//       change_nesting cn(ctx, ++ctx->nest_level);
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    size_t every_in_list_effect::get_type_id() const { return type_id<object>(); }
    std::string_view every_in_list_effect::get_name() const { return commands::names[type_index]; }

    const size_t list_view::type_index = commands::list_view;
    list_view::list_view(const std::string &name, const interface* default_value, const interface* childs) noexcept
      : one_child_interface(default_value), children_interface(childs), name(name) {}
    list_view::~list_view() noexcept {}

    struct object list_view::process(context* ctx) const {
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) return ignore_value;

      const auto def_val = child != nullptr ? child->process(ctx) : object();

      change_scope cs(ctx, object(), ctx->current);
      change_reduce_value crv(ctx, def_val);
      change_indices ci(ctx, 0, ctx->index);

      size_t counter = 0;
      object cur_ret = ignore_value;
      for (size_t i = 0; i < list_itr->second.size(); ++i) {
        ctx->current = list_itr->second[i];
        ctx->index = counter;
        for (auto child = childs; child != nullptr && !ctx->current.ignore(); child = child->next) {
          const auto ret = child->process(ctx);
          ctx->current = ret;
        }

        counter += size_t(!ctx->current.ignore());
        cur_ret = !ctx->current.ignore() ? ctx->current : cur_ret;
      }

      return cur_ret;
    }

    // берем последнее значение
//     static struct object compute_view_value(const local_state* s) {
//       object l_val;
//       for (auto child = s->children; child != nullptr; child = child->next) {
//         l_val = child->value;
//       }
//       return l_val;
//     }

    static void local_view_func(
      context* ctx,
      local_state_allocator* allocator,
      const std::string_view &name,
      const interface* childs,
      const object &obj,
      local_state** first_child,
      local_state** children
    ) {
      local_state* l_first_child = nullptr;
      local_state* l_children = nullptr;

      bool l_unresolved = false;
      object l_val;
      auto ptr = allocator->create(ctx, name);
      change_scope cs(ctx, ctx->current, ctx->prev);
      for (auto child = childs; child != nullptr; child = child->next) {
        auto c = child->compute(ctx, allocator);

        if (c->value.unresolved()) l_unresolved = true;
        else l_val = c->value;
        ctx->current = l_unresolved ? unresolved_value : l_val;

        if (l_first_child == nullptr) l_first_child = c;
        if (l_children != nullptr) l_children->next = c;
        l_children = c;
      }

      ptr->value = l_unresolved ? unresolved_value : l_val;
      ptr->original = obj;
      ptr->children = l_first_child;
      //ptr->func = &compute_view_child_value;

      if (*first_child == nullptr) *first_child = ptr;
      if (*children != nullptr) (*children)->next = ptr;
      *children = ptr;
    }

    local_state* list_view::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      auto d_v = child != nullptr ? child->compute(ctx, allocator) : nullptr;
      change_reduce_value crv(ctx, d_v != nullptr ? d_v->value : object());
      local_state* first_child = nullptr;
      local_state* children = nullptr;
      const auto list_itr = ctx->object_lists.find(name);
      if (list_itr == ctx->object_lists.end()) ptr->value = ignore_value;
      else {
        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        for (size_t i = 0; i < list_itr->second.size(); ++i) {
          const auto &obj = list_itr->second[i];
          ctx->current = obj;
          ctx->index = i;
          local_view_func(ctx, allocator, name, childs, obj, &first_child, &children);
        }
      }

      if (d_v != nullptr) {
        d_v->argument_name = "default_value";
        d_v->next = first_child;
      }

      ptr->children = d_v == nullptr ? first_child : d_v;
      ptr->value = get_last_value(ptr);
      ptr->func = &get_last_value;
      return ptr;
    }

//     void list_view::draw(context* ctx) const {
//       {
//         const auto obj = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = obj.ignore() ? object() : obj;
//         dd.original = name;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       //change_scope cs(ctx, obj, ctx->current);
//       change_nesting cn(ctx, ctx->nest_level+1);
//       change_function_name cfn(ctx, get_name());
//       for (auto child = childs; child != nullptr; child = child->next) { child->draw(ctx); }
//     }

    size_t list_view::get_type_id() const { return type_id<object>(); }
    std::string_view list_view::get_name() const { return commands::names[type_index]; }

    const size_t transform::type_index = commands::transform;
    transform::transform(const interface* changes) noexcept : one_child_interface(changes) {}
    transform::~transform() noexcept {}
    struct object transform::process(context* ctx) const { return child->process(ctx); }
    local_state* transform::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      ptr->children = child->compute(ctx, allocator);
      ptr->value = ptr->children->value;
      return ptr;
    }
//     void transform::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//       changes->draw(ctx);
//     }
    size_t transform::get_type_id() const { return type_id<object>(); }
    std::string_view transform::get_name() const { return commands::names[type_index]; }

    const size_t filter::type_index = commands::filter;
    filter::filter(const interface* condition) noexcept : condition_interface(condition) {}
    filter::~filter() noexcept { condition->~interface(); }
    struct object filter::process(context* ctx) const {
      const auto obj = condition->process(ctx);
      return obj.ignore() || !obj.get<bool>() ? ignore_value : ctx->current;
    }
    local_state* filter::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      ptr->children = condition->compute(ctx, allocator);
      ptr->func = [] (const local_state* s) -> object {
        const auto &obj = s->children->value;
        if (obj.unresolved()) return unresolved_value;
        return obj.ignore() || !obj.get<bool>() ? ignore_value : s->current;
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }
//     void filter::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = condition->process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//       condition->draw(ctx);
//     }
    size_t filter::get_type_id() const { return type_id<object>(); }
    std::string_view filter::get_name() const { return commands::names[type_index]; }

    const size_t reduce::type_index = commands::reduce;
    reduce::reduce(const interface* value) noexcept : one_child_interface(value) {}
    reduce::~reduce() noexcept {}
    struct object reduce::process(context* ctx) const {
      const auto obj = child->process(ctx);
      assert(!obj.ignore());
      ctx->reduce_value = obj;
      return obj;
    }
    local_state* reduce::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      ptr->children = child->compute(ctx, allocator);
      ctx->reduce_value = ptr->children->value;
      ptr->value = ptr->children->value;
      return ptr;
    }
//     void reduce::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.value = process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//       value->draw(ctx);
//     }
    size_t reduce::get_type_id() const { return type_id<object>(); }
    std::string_view reduce::get_name() const { return commands::names[type_index]; }

    const size_t take::type_index = commands::take;
    take::take(const size_t count) noexcept : count(count) {}
    take::~take() noexcept {}
    struct object take::process(context* ctx) const { return ctx->index < count ? ctx->current : ignore_value; }
    local_state* take::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->original = object(double(count));
      ptr->func = [] (const local_state* s) -> object {
        const double count = s->original.get<double>();
        return s->index < count ? s->current : ignore_value;
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }
//     void take::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = object(double(count));
//       ctx->draw(&dd);
//     }
    size_t take::get_type_id() const { return type_id<object>(); }
    std::string_view take::get_name() const { return commands::names[type_index]; }

    const size_t drop::type_index = commands::drop;
    drop::drop(const size_t count) noexcept : count(count) {}
    drop::~drop() noexcept {}
    struct object drop::process(context* ctx) const { return ctx->index < count ? ignore_value : ctx->current; }
    local_state* drop::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      ptr->original = object(double(count));
      ptr->func = [] (const local_state* s) -> object {
        const double count = s->original.get<double>();
        return s->index < count ? ignore_value : s->current;
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }
//     void drop::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = object(double(count));
//       ctx->draw(&dd);
//     }
    size_t drop::get_type_id() const { return type_id<object>(); }
    std::string_view drop::get_name() const { return commands::names[type_index]; }

    // как сюда передать данные для скрипта, нужна строка + интерфейс и это только для локалов
    // видимо нужно тащить здесь std::vector<std::string, const interface*> в двух экземплярах
    //
    const size_t execute::type_index = commands::execute;
    execute::execute(const std::string_view name, const script_data* script, std::vector<const interface*> new_locals) noexcept :
      name(name), script(script), new_locals(std::move(new_locals))
    {}

    // скрипт НЕЛЬЗЯ удалять
    execute::~execute() noexcept {
      for (const auto p : new_locals) { if (p != nullptr) p->~interface(); }
    }

    struct object execute::process(context* ctx) const {
      // может ли скрипт как то получить инфу с локалов основного скрипта?
      // для этого придется запомнить как расположены локалы
      // вообще понимание какие локалы задействованы в скрипте решает несколько проблем разом
      // но это означает что нужно создать std::vector<std::string> дополнительный
      allocate_additional_locals aal(ctx, script->max_locals);

      assert(script->locals.size() >= new_locals.size());
      for (size_t i = 0; i < new_locals.size(); ++i) {
        auto ptr = new_locals[i];
        if (ptr == nullptr) continue;
        ctx->save_local(i, ptr->process(ctx));
      }

      return script->begin->process(ctx);
    }

    local_state* execute::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      assert(script->locals.size() >= new_locals.size());
      allocate_additional_locals aal(ctx, script->max_locals);
      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);
      local_state* first_child = nullptr;
      local_state* children = nullptr;
      for (size_t i = 0; i < new_locals.size(); ++i) {
        if (new_locals[i] == nullptr) continue;
        auto s = new_locals[i]->compute(ctx, allocator);
        s->argument_name = script->locals[i];
        ctx->save_local(i, s->value);

        if (first_child == nullptr) first_child = s;
        if (children != nullptr) children->next = s;
        children = s;
      }

      auto state = script->begin->compute(ctx, allocator);
      state->next = first_child;
      ptr->children = state;
      ptr->value = state->value;
      ptr->original = name;
      ptr->func = [] (const local_state* s) -> object { return s->children->value; };
      return ptr;
    }

    //void draw(context* ctx) const override;
    size_t execute::get_type_id() const { return type_id<object>(); }
    std::string_view execute::get_name() const { return commands::names[type_index]; }

    static void draw_nesting(const size_t &nest_level) {
      for (size_t i = 0; i < nest_level; ++i) { std::cout << "  "; }
    }

    const size_t assert_condition::type_index = commands::values::assert_condition;
    assert_condition::assert_condition(const interface* condition, const interface* str) noexcept
      : condition_interface(condition), one_child_interface(str) {}
    assert_condition::~assert_condition() noexcept {}
    object assert_condition::process(context* ctx) const {
      const auto obj = condition->process(ctx);
      if (obj.is<bool>() && obj.get<bool>()) return ignore_value;

      // ошибка, что нужно сделать? вывести кондишен?
      const auto func = [] (const local_state* data) -> bool {
        draw_nesting(data->nest_level);
        if (data->value.is<bool>() || data->value.is<double>()) {
          std::cout << "func " << data->function_name <<
                       " current type " << data->current.type <<
                       " value " << (data->value.is<bool>() ? data->value.get<bool>() : data->value.get<double>()) << '\n';
        } else {
          std::cout << "func " << data->function_name <<
                       " current type " << data->current.type <<
                       " value type " << data->value.type << '\n';
        }

        return true;
      };

      ctx->draw_function = func;
      // наверное тут создадим дефолтный аллокатор и вычислим кондишен
//       condition->draw(ctx);
      if (child != nullptr) {
        const auto str_obj = child->process(ctx);
        const auto hint = str_obj.get<std::string_view>();
        throw std::runtime_error("Assertion failed in entity " + std::string(ctx->id) + " method " + std::string(ctx->method_name) + " hint: " + std::string(hint));
      } else {
        throw std::runtime_error("Assertion failed in entity " + std::string(ctx->id) + " method " + std::string(ctx->method_name));
      }
      return ignore_value;
    }

    local_state* assert_condition::compute(context* ctx, local_state_allocator* allocator) const {
      // что тут? тут мы в общем то просто создадим стейты у всех объектов
      // и функция сделает примерно тоже самое что и в process
    }

//     void assert_condition::draw(context*) const {
//       // надо ли тут рисовать? недумаю
//     }

    size_t assert_condition::get_type_id() const { return type_id<object>(); }
    std::string_view assert_condition::get_name() const { return commands::names[type_index]; }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
