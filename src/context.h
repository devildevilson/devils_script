#ifndef DEVILS_ENGINE_SCRIPT_CONTEXT_H
#define DEVILS_ENGINE_SCRIPT_CONTEXT_H

#include <array>
#include <string_view>
#include <functional>
#include <bitset>
#include "parallel_hashmap/phmap.h"
#include "object.h"
#include "common.h"
#include "linear_rng.h"
#include "memory_pool.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    struct context;

    // название может поменять? local_state, local_context, наверное лучше первое
    // value и original нужны для вычисленных данных и дополнительных данных функций
    // остальные переменные задаются из контекста,
    // было бы неплохо избавиться от id_hash, method_hash, seed, но их откуда то нужно тогда брать
    struct local_state {
//       static const size_t arguments_count = 16;

//       std::string_view id;
//       std::string_view method_name;
      std::string_view function_name;
      std::string_view prev_function_name;
      std::string_view argument_name;
      size_t id_hash, method_hash, seed, type, operator_type, nest_level; // что такое type?
      size_t index, prev_index, local_rand_state;
      object current, prev, value, original;
      // аргументов строго говоря может быть больше чем 16 да и их проще задать через указатели
      //std::array<std::pair<std::string_view, object>, arguments_count> arguments;
      local_state* children;
      local_state* next;
      void* user_data;

      // что тут? по идее тоже самое что процесс, только берем аргументы у детей в local_state::value
      // в некоторых функциях повторить полностью стейт как он был невозможно: например проверка уникальности
      // для нее требуется использовать save_set_on_stack в функции keep_set
      // это запоминание может пригодиться только в ограниченном количестве сценариев, в основном при вызове эффектов
      // это нужно больше для дебага (можно например чутка поменять значения и покрутить функцию с новыми значениями)
      std::function<object(const local_state*)> func;

      local_state() noexcept;
      local_state(context* ctx) noexcept;
      local_state(context* ctx, const std::string_view &function_name) noexcept;
      local_state(context* ctx, const std::string_view &function_name, const object &value) noexcept;
      //void set_arg(const uint32_t &index, const std::string_view &name, const object &obj);

      local_state(const local_state &copy) noexcept = default;
      local_state(local_state &&move) noexcept = default;
      local_state & operator=(const local_state &copy) noexcept = default;
      local_state & operator=(local_state &&move) noexcept = default;

      uint64_t get_random_value(const size_t &static_state) const noexcept;
    };

    class local_state_allocator {
    public:
      virtual ~local_state_allocator() noexcept = default;
      virtual local_state* create(context* ctx) = 0;
      virtual local_state* create(context* ctx, const std::string_view &fn) { auto ptr = create(ctx); ptr->function_name = fn; return ptr; }
      virtual local_state* create(context* ctx, const std::string_view &fn, const object &v) { auto ptr = create(ctx); ptr->function_name = fn; ptr->value = v; return ptr; }
      virtual void free(local_state* ptr) = 0;
      // реализация тут скорее всего только одна
      void traverse_and_free(local_state* data);
    };

    struct default_local_state_allocator final : public local_state_allocator {
      static const size_t default_local_state_pool_element_count = 100;

      utils::memory_pool<local_state, sizeof(local_state)*default_local_state_pool_element_count> local_state_pool;

      local_state* create(context* ctx) override;
      local_state* create(context* ctx, const std::string_view &fn) override;
      local_state* create(context* ctx, const std::string_view &fn, const object &v) override;
      void free(local_state* ptr) override;
    };

    using draw_function_t = std::function<bool(const local_state* data)>;
    void add_to_list_temp(void*, const object &) {}
    using add_to_list_t = decltype(&add_to_list_temp);

    // размер 472 мне нравится больше
    struct context {
      static const size_t locals_container_size = 64;
      static const size_t lists_container_size = 64;

      enum {
        current_context_is_undefined,
        attributes_count
      };

      std::string_view id;
      std::string_view method_name;
      std::string_view function_name; // нужно ли это здесь? в context наверное больше не понадобится
      std::string_view prev_function_name; // вот это пригодится
      size_t type, operator_type, nest_level;
      object root, prev, current, rvalue, reduce_value;

      // данные для рандомизации, хэш поди нужно использовать std, использую свой
      size_t id_hash, method_hash, seed;
      size_t index, prev_index;

      // можем ли мы использовать string_view в качестве ключа? откуда строки берутся?
      // из скрипта (хранятся в контейнерах) + из on_action (константные строки?) + из вызова метода (константные строки?)
      // возможно мы тут легко можем обойтись string_view
      phmap::flat_hash_map<std::string_view, object> map;
      phmap::flat_hash_map<std::string_view, std::vector<object>> object_lists;
      //std::array<std::vector<object>, lists_container_size> object_lists;
      phmap::flat_hash_set<object> unique_objects;

      // теперь мы можем использовать std vector для этого, более того это наверное даже предпочтительно (у array получается слишком большой размер)
      //std::array<object, locals_container_size> locals;
      size_t locals_offset; // используем оффсет для того чтобы аккуратно войти в новый скрипт
      std::vector<object> locals;

      draw_function_t draw_function;
      std::bitset<attributes_count> attributes;
      void* vector_ptr;
      add_to_list_t add_func_ptr;
      void* user_data;

      static double normalize_value(const uint64_t value);

      context() noexcept;
      context(const std::string_view &id, const std::string_view &method_name, const size_t &seed) noexcept;

      context(const context &copy) noexcept = default;
      context(context &&move) noexcept = default;
      context & operator=(const context &copy) noexcept = default;
      context & operator=(context &&move) noexcept = default;

      void set_data(const std::string_view &id, const std::string_view &method_name, const size_t &seed) noexcept;
      void set_data(const std::string_view &id, const std::string_view &method_name) noexcept;

      inline bool get_attribute(const size_t &index) const noexcept { return attributes.test(index); }
      inline void set_attribute(const size_t &index, const bool value) noexcept { attributes.set(index, value); }

      bool draw_state() const noexcept;
      bool draw(const local_state* data) const;
      void traverse_and_draw(const local_state* data) const;

      uint64_t get_random_value(const size_t &static_state) const noexcept;

      object get_local(const size_t index) const;
      void save_local(const size_t index, const object obj);
      void remove_local(const size_t index) noexcept;

      void clear() noexcept;
    };

    // классы смены скоупа
    struct change_scope {
      context* ctx;
      object prev, current;

      inline change_scope(context* ctx, const object &new_scope, const object &prev_scope) noexcept : ctx(ctx), prev(ctx->prev), current(ctx->current) {
        ctx->prev = prev_scope;
        ctx->current = new_scope;
      }

      inline ~change_scope() noexcept { ctx->prev = prev; ctx->current = current; }
    };

    struct change_rvalue {
      context* ctx;
      object rvalue;
      size_t operator_type;

      inline change_rvalue(context* ctx, const object &new_rvalue, const size_t &new_operator_type) noexcept : ctx(ctx), rvalue(ctx->rvalue), operator_type(ctx->operator_type) {
        ctx->rvalue = new_rvalue;
        ctx->operator_type = new_operator_type;
      }

      inline ~change_rvalue() noexcept { ctx->rvalue = rvalue; ctx->operator_type = operator_type; }
    };

    struct change_nesting {
      context* ctx;
      size_t nesting;
      inline change_nesting(context* ctx, const size_t &new_nesting) noexcept : ctx(ctx), nesting(ctx->nest_level) { ctx->nest_level = new_nesting; }
      inline ~change_nesting() noexcept { ctx->nest_level = nesting; }
    };

    struct change_attribute {
      context* ctx;
      size_t attrib;
      bool value;
      inline change_attribute(context* ctx, const size_t &attrib, const bool new_value) noexcept :
        ctx(ctx), attrib(attrib), value(ctx->get_attribute(attrib))
      { ctx->set_attribute(attrib, new_value); }
      inline ~change_attribute() noexcept { ctx->set_attribute(attrib, value); }
    };

//     struct draw_condition {
//       context* ctx;
//       inline draw_condition(context* ctx) : ctx(ctx) {
//         local_state dd(ctx);
//         dd.function_name = "condition";
//         ctx->draw(&dd);
//       }
//
//       inline ~draw_condition() {
//         local_state dd(ctx);
//         dd.function_name = "condition_end";
//         ctx->draw(&dd);
//       }
//     };

    struct change_function_name {
      context* ctx;
      std::string_view function_name;
      inline change_function_name(context* ctx, const std::string_view &new_function_name) : ctx(ctx), function_name(ctx->prev_function_name) {
        ctx->prev_function_name = ctx->function_name; ctx->function_name = new_function_name;
      }
      inline ~change_function_name() { ctx->function_name = ctx->prev_function_name; ctx->prev_function_name = function_name; }
    };

    struct change_indices {
      context* ctx;
      size_t index, prev_index;
      inline change_indices(context* ctx, const size_t new_index, const size_t new_prev_index) :
        ctx(ctx), index(ctx->index), prev_index(ctx->prev_index)
      {
        ctx->index = new_index;
        ctx->prev_index = new_prev_index;
      }
      inline ~change_indices() { ctx->index = index; ctx->prev_index = index; }
    };

    struct change_reduce_value {
      context* ctx;
      object reduce_value;

      inline change_reduce_value(context* ctx, const object &new_reduce_value) noexcept : ctx(ctx), reduce_value(ctx->reduce_value) {
        ctx->reduce_value = new_reduce_value;
      }

      inline ~change_reduce_value() noexcept { ctx->reduce_value = reduce_value; }
    };

    struct change_vector_and_func {
      context* ctx;
      void* old_ptr;
      add_to_list_t old_func;
      inline change_vector_and_func(context* ctx, void* ptr, add_to_list_t func) noexcept : ctx(ctx), old_ptr(ctx->vector_ptr), old_func(ctx->add_func_ptr) {
        ctx->vector_ptr = ptr;
        ctx->add_func_ptr = func;
      }

      inline ~change_vector_and_func() {
        ctx->vector_ptr = old_ptr;
        ctx->add_func_ptr = old_func;
      }
    };
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
