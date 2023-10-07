#ifndef DEVILS_ENGINE_SCRIPT_COMMON_COMMANDS_H
#define DEVILS_ENGINE_SCRIPT_COMMON_COMMANDS_H

#include "core_interface.h"

#define MAXIMUM_OVERLOADS 16

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    class change_scope_condition final : public scope_interface, public condition_interface, public one_child_interface  {
    public:
      change_scope_condition(const interface* scope, const interface* condition, const interface* child) noexcept;
      ~change_scope_condition() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* scope;
      //const interface* condition;
      //const interface* child;
    };

    class change_scope_effect final : public scope_interface, public condition_interface, public children_interface {
    public:
      change_scope_effect(const interface* scope, const interface* condition, const interface* childs) noexcept;
      ~change_scope_effect() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
//      const interface* scope;
//      const interface* condition;
//      const interface* childs;
    };

    class compute_string final : public condition_interface, public children_interface {
    public:
      compute_string(const interface* condition, const interface* childs) noexcept;
      ~compute_string() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* condition;
      //const interface* childs;
    };

    class compute_object final : public condition_interface, public children_interface {
    public:
      compute_object(const interface* condition, const interface* childs) noexcept;
      ~compute_object() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* condition;
      //const interface* childs;
    };

    class compute_number final : public scope_interface, public condition_interface, public one_child_interface {
    public:
      compute_number(const interface* scope, const interface* condition, const interface* child) noexcept;
      ~compute_number() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* scope;
      //const interface* condition;
      //const interface* child;
    };

    class selector final : public children_interface {
    public:
      static const size_t type_index;
      selector(const interface* childs) noexcept;
      ~selector() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* childs;
    };

    class sequence final : public additional_child_interface, public children_interface {
    public:
      static const size_t type_index;
      sequence(const interface* count, const interface* childs) noexcept;
      ~sequence() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
//      const interface* count;
//      const interface* childs;
    };

    class overload final : public children_interface {
    public:
      overload(const std::array<size_t, MAXIMUM_OVERLOADS> &overload_types, const interface* childs) noexcept;
      ~overload() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::array<size_t, MAXIMUM_OVERLOADS> overload_types;
      //const interface* childs;
    };

    class chance final : public one_child_interface {
    public:
      static const size_t type_index;
      chance(const size_t &state, const interface* value) noexcept;
      ~chance() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      size_t state;
      //const interface* value;
    };

    // нужно сделать рандом по весам, как он должен выглядеть?
    class weighted_random final : public children_interface, public additional_children_interface {
    public:
      static const size_t type_index;
      weighted_random(const size_t &state, const interface* childs, const interface* weights) noexcept;
      ~weighted_random() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      size_t state;
      //const interface* childs;
      //const interface* weights;
    };

    class random_value final : public one_child_interface {
    public:
      static const size_t type_index;
      random_value(const size_t &state, const interface* maximum) noexcept;
      ~random_value() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      size_t state;
      //const interface* maximum;
    };

    class boolean_container final : public interface {
    public:
      boolean_container(const bool value) noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      bool value;
    };

    class number_container final : public interface {
    public:
      number_container(const double &value) noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      double value;
    };

    class string_container final : public interface {
    public:
      explicit string_container(const std::string &value) noexcept;
      explicit string_container(const std::string_view &value) noexcept;
      ~string_container() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string value;
    };

    class object_container final : public interface {
    public:
      object_container(const object &value) noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      object value;
    };

    // нужно еще сравниватель объектов сделать

    class number_comparator final : public additional_child_interface, public one_child_interface {
    public:
      number_comparator(const uint8_t op, const interface* lvalue, const interface* rvalue) noexcept;
      ~number_comparator() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      uint8_t op;
      //const interface* lvalue;
      //const interface* rvalue;
    };

    class boolean_comparator final : public additional_child_interface, public one_child_interface {
    public:
      boolean_comparator(const interface* lvalue, const interface* rvalue) noexcept;
      ~boolean_comparator() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* lvalue;
      //const interface* rvalue;
    };

    class equals_to final : public one_child_interface {
    public:
      static const size_t type_index;
      equals_to(const interface* get_obj) noexcept;
      ~equals_to() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* get_obj;
    };

    class not_equals_to final : public one_child_interface {
    public:
      static const size_t type_index;
      not_equals_to(const interface* get_obj) noexcept;
      ~not_equals_to() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* get_obj;
    };

    // если добавится функция compare, то тут наверное было бы неплохо сделать по умолчанию объект
    class equality final : public children_interface {
    public:
      static const size_t type_index;
      equality(const interface* childs) noexcept;
      ~equality() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* childs;
    };

    class type_equality final : public children_interface {
    public:
      static const size_t type_index;
      type_equality(const interface* childs) noexcept;
      ~type_equality() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* childs;
    };

    // эта функция может ответить на вопрос является ли первое число самым маленьким
    // или быстрая проверка нескольких чисел на то что они больше нуля
    class compare final : public children_interface {
    public:
      static const size_t type_index;
      compare(const uint8_t op, const interface* childs) noexcept;
      ~compare() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      uint8_t op;
      //const interface* childs;
    };

    class keep_set final : public one_child_interface {
    public:
      static const size_t type_index;
      keep_set(const interface* child) noexcept;
      ~keep_set() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* child;
    };

    class is_unique final : public one_child_interface {
    public:
      static const size_t type_index;
      is_unique(const interface* value) noexcept;
      ~is_unique() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* value;
    };

    class place_in_set final : public one_child_interface {
    public:
      static const size_t type_index;
      place_in_set(const interface* value) noexcept;
      ~place_in_set() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* value;
    };

    class place_in_set_if_unique final : public one_child_interface {
    public:
      static const size_t type_index;
      place_in_set_if_unique(const interface* value) noexcept;
      ~place_in_set_if_unique() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* value;
    };

    class complex_object final : public children_interface {
    public:
      complex_object(const interface* childs) noexcept;
      ~complex_object() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* childs;
    };

    class invalid final : public interface {
    public:
      static const size_t type_index;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      std::string_view get_name() const;
    };

    class ignore final : public interface {
    public:
      static const size_t type_index;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      std::string_view get_name() const;
    };

    class root final : public interface {
    public:
      static const size_t type_index;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    class prev final : public interface {
    public:
      static const size_t type_index;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    class current final : public interface {
    public:
      static const size_t type_index;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    class index final : public interface {
    public:
      static const size_t type_index;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    class prev_index final : public interface {
    public:
      static const size_t type_index;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    // в этом классе необходимость отсутствует, это должен быть просто "указатель" что дальше пойдет вычисление числа
    class value final : public one_child_interface {
    public:
      static const size_t type_index;
      value(const interface* child) noexcept;
      ~value() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* child;
    };

    class get_context final : public interface {
    public:
      static const size_t type_index;
      explicit get_context(const std::string str) noexcept;
      explicit get_context(const std::string_view str) noexcept;
      ~get_context() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
    };

    class save_local final : public one_child_interface {
    public:
      static const size_t type_index;
      explicit save_local(const std::string name, const size_t index, const interface* var) noexcept;
      explicit save_local(const std::string_view name, const size_t index, const interface* var) noexcept;
      ~save_local() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
      size_t index;
      //const interface* var; // если вар не задан, то сохраняем куррент
    };

    class has_local final : public interface {
    public:
      static const size_t type_index;
      has_local(const size_t &index) noexcept;
      ~has_local() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      size_t index;
    };

    class remove_local final : public interface {
    public:
      static const size_t type_index;
      explicit remove_local(const std::string name, const size_t index) noexcept;
      explicit remove_local(const std::string_view name, const size_t index) noexcept;
      ~remove_local() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
      size_t index;
    };

    class get_local final : public interface {
    public:
      static const size_t type_index;
      explicit get_local(const std::string name, const size_t index) noexcept;
      explicit get_local(const std::string_view name, const size_t index) noexcept;
      ~get_local() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
      size_t index;
    };

    class save final : public one_child_interface {
    public:
      static const size_t type_index;
      explicit save(const std::string str, const interface* var) noexcept;
      explicit save(const std::string_view str, const interface* var) noexcept;
      ~save() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
      //const interface* var;
    };

    class remove final : public interface {
    public:
      static const size_t type_index;
      explicit remove(const std::string str) noexcept;
      explicit remove(const std::string_view str) noexcept;
      ~remove() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
    };

    class has_context final : public interface {
    public:
      static const size_t type_index;
      explicit has_context(const std::string str) noexcept;
      explicit has_context(const std::string_view str) noexcept;
      ~has_context() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
    };

    class get_list final : public interface {
    public:
      static const size_t type_index;
      explicit get_list(const std::string name) noexcept;
      explicit get_list(const std::string_view name) noexcept;
      ~get_list() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
    };

    class add_to_list final : public interface {
    public:
      static const size_t type_index;
      add_to_list(const std::string &name) noexcept;
      ~add_to_list() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
    };

    class is_in_list final : public interface {
    public:
      static const size_t type_index;
      is_in_list(const std::string &name) noexcept;
      ~is_in_list() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
    };

    class has_in_list final : public additional_child_interface, public one_child_interface, public children_interface {
    public:
      static const size_t type_index;
      has_in_list(const std::string &name, const interface* max_count, const interface* percentage, const interface* childs) noexcept;
      ~has_in_list() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
//      const interface* max_count;
//      const interface* percentage;
//      const interface* childs;
    };

    class random_in_list final : public condition_interface, public additional_child_interface, public children_interface {
    public:
      static const size_t type_index;
      random_in_list(const std::string &name, const size_t &state, const interface* condition, const interface* weight, const interface* childs) noexcept;
      ~random_in_list() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
      size_t state;
      //const interface* condition;
      //const interface* weight;
      //const interface* childs;
    };

    class every_in_list_numeric final : public condition_interface, public children_interface {
    public:
      static const size_t type_index;
      every_in_list_numeric(const std::string &name, const interface* condition, const interface* childs) noexcept;
      ~every_in_list_numeric() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
//      const interface* condition;
//      const interface* childs;
    };

    class every_in_list_logic final : public condition_interface, public children_interface {
    public:
      static const size_t type_index;
      every_in_list_logic(const std::string &name, const interface* condition, const interface* childs) noexcept;
      ~every_in_list_logic() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
//      const interface* condition;
//      const interface* childs;
    };

    class every_in_list_effect final : public condition_interface, public children_interface {
    public:
      static const size_t type_index;
      every_in_list_effect(const std::string &name, const interface* condition, const interface* childs) noexcept;
      ~every_in_list_effect() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
//      const interface* condition;
//      const interface* childs;
    };

    class list_view final : public one_child_interface, public children_interface {
    public:
      static const size_t type_index;
      list_view(const std::string &name, const interface* default_value, const interface* childs) noexcept;
      ~list_view() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::string name;
      //const interface* default_value;
      //const interface* childs;
    };

    // по идее тут просто все: делаем действия возращаем объект, это скорее со стороны инициализации вопрсо
    class transform final : public one_child_interface {
    public:
      static const size_t type_index;
      transform(const interface* changes) noexcept;
      ~transform() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* changes;
    };

    class filter final : public condition_interface {
    public:
      static const size_t type_index;
      filter(const interface* condition) noexcept;
      ~filter() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* condition;
    };

    class reduce final : public one_child_interface {
    public:
      static const size_t type_index;
      reduce(const interface* value) noexcept;
      ~reduce() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* value;
    };

    class take final : public interface {
    public:
      static const size_t type_index;
      take(const size_t count) noexcept;
      ~take() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      size_t count;
    };

    class drop final : public interface {
    public:
      static const size_t type_index;
      drop(const size_t count) noexcept;
      ~drop() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      size_t count;
    };

    class execute final : public interface {
    public:
      static const size_t type_index;
      execute(const std::string_view name, const script_data* script, std::vector<const interface*> new_locals) noexcept;
      ~execute() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const std::string_view name;
      const script_data* script;
      // лист из интерфейсов?
      std::vector<const interface*> new_locals;
    };

    class assert_condition final : public condition_interface, public one_child_interface {
    public:
      static const size_t type_index;
      assert_condition(const interface* condition, const interface* str) noexcept;
      ~assert_condition() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      //const interface* condition;
      //const interface* str;
    };

    // TODO: add 'debug_log'
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
