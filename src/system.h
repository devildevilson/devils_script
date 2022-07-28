#ifndef DEVILS_SCRIPT_SYSTEM_H
#define DEVILS_SCRIPT_SYSTEM_H

#include <functional>
#include <string>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <any>

// before c++20
#if __cplusplus >= 202002L
#  include <span>
#else
#  define TCB_SPAN_NAMESPACE_NAME std
#  include "tcb/span.hpp"
#endif

#include "parallel_hashmap/phmap.h"

#include "memory_pool.h"
#include "sol.h"
#include "linear_rng.h"

#include "templates.h"
#include "type_info.h"
#include "interface.h"
#include "container.h"
#include "core.h"
#include "header.h"
#include "common.h"
#include "forward_list.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    // до c++20 будем вот такое городить
#define MAKE_STR_PARAMETERS_1(a)        decltype(a##_create)::value
#define MAKE_STR_PARAMETERS_2(a,b)      decltype(a##_create)::value, decltype(b##_create)::value
#define _MAKE_STR_PARAMETERS_2(a,b)     decltype(a##_create)::value, b
#define MAKE_STR_PARAMETERS_3(a,...)    _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_2(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_4(a,...)    _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_3(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_5(a,...)    _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_4(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_6(a,...)    _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_5(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_7(a,...)    _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_6(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_8(a,...)    _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_7(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_9(a,...)    _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_8(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_10(a,...)   _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_9(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_11(a,...)   _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_10(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_12(a,...)   _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_11(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_13(a,...)   _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_12(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_14(a,...)   _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_13(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_15(a,...)   _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_14(__VA_ARGS__))
#define MAKE_STR_PARAMETERS_16(a,...)   _MAKE_STR_PARAMETERS_2(a,MAKE_STR_PARAMETERS_15(__VA_ARGS__))

// NUM_ARGS(...) evaluates to the literal number of the passed-in arguments.
#define _NUM_ARGS2(X,X64,X63,X62,X61,X60,X59,X58,X57,X56,X55,X54,X53,X52,X51,X50,X49,X48,X47,X46,X45,X44,X43,X42,X41,X40,X39,X38,X37,X36,X35,X34,X33,X32,X31,X30,X29,X28,X27,X26,X25,X24,X23,X22,X21,X20,X19,X18,X17,X16,X15,X14,X13,X12,X11,X10,X9,X8,X7,X6,X5,X4,X3,X2,X1,N,...) N
#define NUM_ARGS(...) _NUM_ARGS2(0, __VA_ARGS__ ,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define _MAKE_STR_PARAMETERS_N3(N, ...)  MAKE_STR_PARAMETERS_ ## N(__VA_ARGS__)
#define _MAKE_STR_PARAMETERS_N2(N, ...) _MAKE_STR_PARAMETERS_N3(N, __VA_ARGS__)
#define MAKE_STR_PARAMETERS_N(...)      _MAKE_STR_PARAMETERS_N2(NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

#define SAME_TYPE(type1, type2) (type_id<type1>() == type_id<type2>())

#define SCRIPT_SYSTEM_FUNCTION_ITERATOR (SIZE_MAX-1)
#define SCRIPT_SYSTEM_UNDEFINED_ARGS_COUNT (SIZE_MAX-2)
#define SCRIPT_SYSTEM_EVERY_RETURN_TYPE (SIZE_MAX-3)

#define SCRIPT_SYSTEM_DEFAULT_ALIGMENT (8)

#define SCRIPT_SYSTEM_ANY_TYPE (DEVILS_SCRIPT_FULL_NAMESPACE::type_id<DEVILS_SCRIPT_FULL_NAMESPACE::object>())

    // для скриптов эффектов мы должны ожидать ТОЛЬКО void
    size_t get_script_type(const size_t &type_id);

    template <typename R>
    constexpr size_t get_script_type() {
      if constexpr (SAME_TYPE(R, void)) { return script_types::effect; }
      else if constexpr (SAME_TYPE(R, bool)) { return script_types::condition; }
      else if constexpr (SAME_TYPE(R, double)) { return script_types::numeric; }
      else if constexpr (SAME_TYPE(R, std::string_view)) { return script_types::string; }
      else if constexpr (SAME_TYPE(R, object)) { return SIZE_MAX; } // any script type
      else return script_types::object;
      return SIZE_MAX;
    }

    constexpr bool compare_script_types(const size_t &input, const size_t &expected) {
      if (expected == SIZE_MAX) return true;
      if (input == SIZE_MAX) return true;
      if (input == expected) return true;
      if ((input    == script_types::condition || input    == script_types::numeric) &&
          (expected == script_types::condition || expected == script_types::numeric)) return true;
      return false;
    }

    void check_script_types(const std::string_view &name, const size_t &input, const size_t &expected);
    //static bool is_complex_object(const std::string_view &lvalue);

    namespace detail {
      extern const phmap::flat_hash_set<std::string_view> view_allowed_funcs;
      std::string_view get_sol_type_name(const sol::type &t);
    }

    class system {
      friend class view;
    public:
      struct init_func_data;
      struct init_context;
      using init_function_t = std::function<interface*(system*, init_context*, const sol::object &, container*)>;
      using get_info_func_t = std::function<std::string()>;

      // type_id<void>() не возвращает 0, желательно переделать
      // current_type может быть 0 (void), тогда нужно найти все переопределения функции и пихнуть их в overload
      // когда такие происходит? когда у нас в лвалуе берется значение из контекста, причем больше ничего не происходит
      // в этом случае остается только предполагать
      struct init_context {
        size_t root, current_type, prev_type, script_type, current_size, current_count;
        size_t computed_type, compare_operator, current_local_var_id, expected_type;
        const init_func_data* current_block;
        std::string_view script_name;
        std::string_view block_name;
        std::string_view function_name;
        phmap::flat_hash_map<std::string, std::pair<size_t, size_t>> local_var_ids;
        phmap::flat_hash_set<std::string> lists;

        // флаги?
        inline init_context() :
          root(0), current_type(0), prev_type(0), script_type(SIZE_MAX),
          current_size(0), current_count(0), computed_type(SCRIPT_SYSTEM_ANY_TYPE), compare_operator(compare_operators::more_eq),
          current_local_var_id(0), expected_type(SCRIPT_SYSTEM_ANY_TYPE), current_block(nullptr)
        {}

        size_t save_local(const std::string &name, const size_t &type);
        std::tuple<size_t, size_t> get_local(const std::string_view &name) const;
        size_t remove_local(const std::string_view &name);

        template <typename T>
        inline void add_function() {
          static_assert(alignof(T) <= SCRIPT_SYSTEM_DEFAULT_ALIGMENT, "Type aligment must be less or eq than SCRIPT_SYSTEM_DEFAULT_ALIGMENT");
          current_size += align_to(sizeof(T), SCRIPT_SYSTEM_DEFAULT_ALIGMENT);
          current_count += 1;
        }

        void clear();
      };

      struct init_func_data {
        size_t script_type;
        size_t input_type;
        size_t return_type;
        size_t arguments_count;
        init_function_t func;
        get_info_func_t info;
      };

      struct change_expected_type {
        init_context* ctx;
        size_t expected_type;
        inline change_expected_type(init_context* ctx, const size_t &new_expected_type) : ctx(ctx), expected_type(ctx->expected_type) { ctx->expected_type = new_expected_type; }
        inline ~change_expected_type() { ctx->expected_type = expected_type; }
      };

      struct change_computed_type {
        init_context* ctx;
        size_t computed_type;
        inline change_computed_type(init_context* ctx, const size_t &new_computed_type) : ctx(ctx), computed_type(ctx->computed_type) { ctx->computed_type = new_computed_type; }
        inline ~change_computed_type() { ctx->computed_type = computed_type; }
      };

      class view {
      public:
        inline view(system* sys, init_context* ctx, container* cont, const bool is_iterator) : sys(sys), ctx(ctx), cont(cont), is_iterator(is_iterator) {}

        interface* make_scripted_conditional(const sol::object &obj);
        interface* make_scripted_numeric(const sol::object &obj);
        interface* make_scripted_string(const sol::object &obj);
        interface* make_scripted_effect(const sol::object &obj);
        interface* make_scripted_object(const size_t &id, const sol::object &obj);

        template <typename T>
        interface* make_scripted_object(const sol::object &obj) {
          return make_scripted_object(type_id<T>(), obj);
        }

        interface* any_scripted_object(const sol::object &obj);

        interface* traverse_children(const sol::object &obj);
        interface* traverse_children_numeric(const sol::object &obj);
        interface* traverse_children_condition(const sol::object &obj);

        size_t get_random_state();

        // это нинужно, в контексте лежит какбы любой тип
        //size_t get_context_value(const std::string_view &name);
        //void set_context_value(const std::string_view &name, const size_t &type);

        size_t save_local(const std::string &name, const size_t &type);
        std::tuple<size_t, size_t> get_local(const std::string_view &name) const;
        size_t remove_local(const std::string_view &name);

        void add_list(const std::string &name);
        bool list_exists(const std::string_view &name) const; // вряд ли нужно проверять, можно добавить в несуществующий лист

        std::tuple<std::string_view, const script_data*, size_t> find_script(const std::string_view name);

        void set_return_type(const size_t &type); // скорее только для локала

        inline const init_context* get_context() const { return ctx; }
      private:
        system* sys;
        init_context* ctx;
        container* cont;
        bool is_iterator;
      };

      struct change_block_function {
        init_context* ctx;
        const init_func_data* current_block;
        inline change_block_function(init_context* ctx, const init_func_data* new_block) : ctx(ctx), current_block(ctx->current_block) { ctx->current_block = new_block; }
        inline ~change_block_function() { ctx->current_block = current_block; }
      };

      struct change_context_types {
        init_context* ctx;
        size_t current_type;
        size_t prev_type;
        inline change_context_types(init_context* ctx, const size_t new_current_type, const size_t new_prev_type) : ctx(ctx), current_type(ctx->current_type), prev_type(ctx->prev_type) {
          ctx->current_type = new_current_type;
          ctx->prev_type = new_prev_type;
        }
        inline ~change_context_types() {
          ctx->current_type = current_type;
          ctx->prev_type = prev_type;
        }
      };

      struct change_script_type {
        init_context* ctx;
        size_t script_type;
        inline change_script_type(init_context* ctx, const size_t new_script_type) : ctx(ctx), script_type(ctx->script_type) { ctx->script_type = new_script_type; }
        inline ~change_script_type() { ctx->script_type = script_type; }
      };

      struct change_block_name {
        init_context* ctx;
        std::string_view block_name;
        inline change_block_name(init_context* ctx, const std::string_view new_block_name) : ctx(ctx), block_name(ctx->block_name) { ctx->block_name = new_block_name; }
        inline ~change_block_name() { ctx->block_name = block_name; }
      };

      struct change_current_function_name {
        init_context* ctx;
        std::string_view function_name;
        inline change_current_function_name(init_context* ctx, const std::string_view new_function_name) : ctx(ctx), function_name(ctx->function_name) { ctx->function_name = new_function_name; }
        inline ~change_current_function_name() { ctx->function_name = function_name; }
      };

      struct change_compare_op {
        init_context* ctx;
        size_t compare_operator;
        inline change_compare_op(init_context* ctx, const size_t new_compare_operator) : ctx(ctx), compare_operator(ctx->compare_operator) { ctx->compare_operator = new_compare_operator; }
        inline ~change_compare_op() { ctx->compare_operator = compare_operator; }
      };

      system(const size_t &seed = 1);
      ~system();
      system(const system &copy) = delete;
      system(system &&move) = default;
      system & operator=(const system &copy) = delete;
      system & operator=(system &&move) = default;

      void copy_init_funcs_to(system &another);

      size_t get_next_random_state();

      // тут по идее что то еще должно быть, например зарегистрировать проверку типа
      // для этого имя нужно задать самостоятельно, потребуется его задавать в темплейте
      // что то еще? нет, регистрировать проверку типа должен сам пользователь, я не могу заранее узнать как проверять да и больше нигде это особо нинужно
      template <typename T>
      void register_usertype();

      template <typename T>
      interface* create_argument(const std::string_view &name, init_context* ctx, const sol::object &obj, container* cont);
      template <typename T>
      interface* create_default_argument(const std::string_view &name, const std::string_view &arg_name, const std::any &default_val, container* cont);

      // к нам тут должна приходить таблица с входными данными
      // из нее по именам мы должны забрать нужные нам скрипты
      // только по именам? нет, может быть еще и по индексам, тогда все более менее просто
      // если по именам то нужно будет передавать список имен куда то
//       template <typename F, size_t N, size_t cur>
//       interface* make_arguments(
//         const std::string_view &name, const std::vector<std::string> &args_name,
//         init_context* ctx, const sol::table &t, container* cont,
//         interface* begin = nullptr, interface* current = nullptr
//       );
//
//       template <typename F, size_t N, size_t cur>
//       interface* make_arguments(const std::string_view &name, init_context* ctx, const sol::table &t, container* cont, interface* begin = nullptr, interface* current = nullptr);
//       template <typename F, size_t N, size_t cur, const char* cur_arg, const char*... args_names>
//       interface* make_arguments(const std::string_view &name, init_context* ctx, const sol::table &t, container* cont, interface* begin = nullptr, interface* current = nullptr);

      // зачем std::any если есть script::object? наверное не имеет смысла пытаться делать какие то особые входные данные которые не помещаются в script::object
      template <typename F, size_t N, size_t cur, bool first_arg>
      interface* make_arguments(
        const std::string_view &name, const std::vector<std::string> &args_name, const std::vector<std::any> &args_default_val,
        init_context* ctx, const sol::object &obj, container* cont,
        interface* begin = nullptr, interface* current = nullptr
      );
      template <typename F, size_t N, size_t cur, bool first_arg>
      interface* make_arguments(
        const std::string_view &name, const std::vector<std::any> &args_default_val,
        init_context* ctx, const sol::object &obj, container* cont,
        interface* begin = nullptr, interface* current = nullptr
      );
      template <typename F, size_t N, size_t cur, bool first_arg, const char* cur_arg, const char*... args_names>
      interface* make_arguments(
        const std::string_view &name, const std::vector<std::any> &args_default_val,
        init_context* ctx, const sol::object &obj, container* cont,
        interface* begin = nullptr, interface* current = nullptr
      );

      // вообще теперь можно убрать дефолтную функцию register_function и оставить только register_function_with_args (переименовав их собственно в register_function)

      // можем ли мы это сделать для произвольного числа входных данных? по крайней мере для нескольких первых
      // register_function - количество аргументов строго не больше одного (может быть или метод или первый аргумент Th), register_function_with_args - произвольное количество аргументов
//       template <typename Th, typename F, F f, const char* name, on_effect_func_t react = nullptr>
//       void register_function();
      template <typename Th, typename F, F f, const char* name, on_effect_func_t react = nullptr>
      void register_function(const std::vector<std::string> &args_names, const std::vector<std::any> &args_default_val = {});
      template <typename Th, typename F, F f, const char* name, on_effect_func_t react = nullptr>
      void register_function(const std::vector<std::any> &args_default_val = {});
      template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_arg, const char*... args_names>
      void register_function(const std::vector<std::any> &args_default_val = {});

      // тут по идее нужно добавить регистрацию с пользовательской функцией, но это приведет
      // к тому что эту функцию мы будем копировать по миллиону раз, а это глупо

      // так мы сделаем общие функции
      // register_function - количество аргументов строго не больше одного, register_function_with_args - произвольное количество аргументов
//       template <typename F, F f, const char* name, on_effect_func_t react = nullptr>
//       void register_function();
      template <typename F, F f, const char* name, on_effect_func_t react = nullptr>
      void register_function(const std::vector<std::string> &args_name, const std::vector<std::any> &args_default_val = {});
      template <typename F, F f, const char* name, on_effect_func_t react = nullptr>
      void register_function(const std::vector<std::any> &args_default_val = {});
      template <typename F, F f, const char* name, on_effect_func_t react, const char* first_arg, const char*... args_names>
      void register_function(const std::vector<std::any> &args_default_val = {});

      // функция с произвольным количеством константных аргументов (или первый аргумент это Th или это метод)
      template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t head, int64_t... args>
      void register_function();
      // функция с произвольным количеством заданных аргументов (или первый аргумент это Th или это метод)
      template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
      void register_function(Args&&... args);

      // пользовательский тип
      template <typename T, typename F, typename R, const char* name>
      void register_function();
      template <typename T, typename F, typename R, const char* name>
      void register_function(const std::function<interface*(system::view, const sol::object &, container::delayed_initialization<F>)> &user_func);

      // итераторы
      // нужно ли реакцию добавить итераторам?
      template <typename T, typename F, F f, const char* name>
      void register_iterator();
      template <typename T, typename F, typename R, const char* name>
      void register_iterator();
      template <typename T, typename F, typename R, const char* name>
      void register_iterator(const std::function<interface*(system::view, const sol::object &, container::delayed_initialization<F>)> &user_func);
      template <typename Th, typename F, F f, const char* name>
      void register_every();
      template <typename Th, typename F, F f, const char* name>
      void register_has();
      template <typename Th, typename F, F f, const char* name>
      void register_random();
      template <typename Th, typename F, F f, const char* name>
      void register_view();

      template <typename T>
      void register_enum(std::vector<std::pair<std::string, T>> enums);

      // возвращать будем легкие обертки вокруг interface* (тип script::number и проч)
      // + нужно будет возвращать скрипты по названию, скрипты которые вставляются в другие скрипты вызываются иначе
      // + могут пригодится флаги
      // нужно сначала создать а потом сохранить (разделить это поведение)
      template <typename T>
      condition create_condition(const sol::object &obj, const std::string_view &script_name);

      template <typename T>
      number create_number(const sol::object &obj, const std::string_view &script_name);

      template <typename T>
      string create_string(const sol::object &obj, const std::string_view &script_name);

      template <typename T>
      effect create_effect(const sol::object &obj, const std::string_view &script_name);

      void save_condition(const condition script, const std::string_view name);
      void save_number(const number script,       const std::string_view name);
      void save_string(const string script,       const std::string_view name);
      void save_effect(const effect script,       const std::string_view name);
      condition get_condition(const std::string_view name) const;
      number get_number(const std::string_view name) const;
      string get_string(const std::string_view name) const;
      effect get_effect(const std::string_view name) const;

      void clear_enums();
      void clear_funcs_init();

      template <typename T>
      const init_func_data* get_init_function(const std::string_view &name) const;

      // мне нужно еще вернуть список всех функций по типам: обычные функции, итераторы, эффекты
      std::string get_function_data_dump() const;
    private:
      bool function_exists(const std::string_view &name, size_t* return_type = nullptr, size_t* arg_count = nullptr);
      const init_func_data* get_init_function(const size_t &type_id, const std::string_view &name) const;

      interface* make_raw_script_boolean(init_context* ctx, const sol::object &obj, container* cont);
      interface* make_raw_script_number(init_context* ctx, const sol::object &obj, container* cont);
      interface* make_raw_script_string(init_context* ctx, const sol::object &obj, container* cont);
      interface* make_raw_script_effect(init_context* ctx, const sol::object &obj, container* cont);
      interface* make_raw_script_object(init_context* ctx, const sol::object &obj, container* cont);
      interface* make_raw_script_any(init_context* ctx, const sol::object &obj, container* cont);

      interface* find_common_function(init_context* ctx, const std::string_view &func_name, const sol::object &obj, container* cont);
      interface* create_overload(init_context* ctx, const std::string_view &func_name, const sol::object &obj, container* cont);

      interface* make_effect_context(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont);
      interface* make_string_context(init_context* ctx, const sol::object &rvalue, container* cont);
      interface* make_object_context(init_context* ctx, const sol::object &rvalue, container* cont);
      interface* make_context_change(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont);

      interface* make_raw_number_compare(init_context* ctx, const interface* lvalue, const interface* rvalue, container* cont);
      interface* make_raw_boolean_compare(init_context* ctx, const interface* lvalue, const interface* rvalue, container* cont);

      interface* make_number_compare(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont);
      interface* make_boolean_compare(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont);
      interface* make_compare(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont, const size_t &return_type);

      interface* make_complex_object(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont);
      interface* make_table_rvalue(init_context* ctx, const sol::object &obj, container* cont);
      interface* make_table_lvalue(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont);

      interface* condition_table_traverse(init_context* ctx, const sol::object &obj, container* cont);
      interface* numeric_table_traverse(init_context* ctx, const sol::object &obj, container* cont);
      interface* string_table_traverse(init_context* ctx, const sol::object &obj, container* cont);
      interface* object_table_traverse(init_context* ctx, const sol::object &obj, container* cont);
      interface* effect_table_traverse(init_context* ctx, const sol::object &obj, container* cont);
      interface* table_traverse(init_context* ctx, const sol::object &obj, container* cont);

      interface* make_boolean_container(const bool val, container* cont);
      interface* make_number_container(const double val, container* cont);
      interface* make_string_container(const std::string_view val, container* cont);
      interface* make_object_container(const object val, container* cont);
      interface* make_invalid_producer(container* cont);
      interface* make_ignore_producer(container* cont);

      // наверное имеет смысл добавить предыдущий тип, если он есть
      using make_script_func_t = decltype(&system::make_raw_script_boolean);
      script_data* create_script_raw(const size_t root_type, const sol::object &obj, const std::string_view name, const make_script_func_t func);

      void register_every_list();

      template <typename T, const char* name>
      void register_function(init_func_data&& data);

      void register_function(const size_t &id, const std::string_view &type_name, const std::string_view &name, init_func_data&& data);

      SCRIPT_SYSTEM_DEFAULT_GENERATOR_NAMESPACE::state random_state;
      utils::memory_pool<container, sizeof(container)*100> containers_pool;
      std::vector<std::pair<const script_data*, container*>> containers;
      phmap::flat_hash_map<std::string, std::pair<size_t, const script_data*>> scripts; // проверяем чтобы возвращаемые типы совпадали
      phmap::flat_hash_map<size_t, phmap::flat_hash_map<std::string_view, init_func_data>> func_map;
      phmap::flat_hash_map<size_t, phmap::flat_hash_map<std::string, int64_t>> enum_map;

      void init(); // default functions init
    };

    // я чет не подумал что довольно большая часть функций не принадлежит типу, и тип наверное лучше бы указать отдельно
#define REG_FUNC(handle_type, func, name) register_function<handle_type, decltype(&func), &func, decltype(name##_create)::value>
#define REG_BASIC(func, name) register_function<decltype(&func), &func, decltype(name##_create)::value>
#define REG_CONST_ARGS(handle_type, func, name, ...) register_function<handle_type, decltype(&func), &func, decltype(name##_create)::value, nullptr, __VA_ARGS__>
//#define REG_ARGS(handle_type, func, name) register_function_with_args<handle_type, decltype(&func), &func, decltype(name##_create)::value, nullptr>
//#define REG_BASIC_ARGS(func, name) register_function_with_args<decltype(&func), &func, decltype(name##_create)::value, nullptr>
#define REG_NAMED_ARGS(handle_type, func, name, ...) register_function<handle_type, decltype(&func), &func, decltype(name##_create)::value, nullptr, MAKE_STR_PARAMETERS_N(__VA_ARGS__)>
#define REG_BASIC_NAMED_ARGS(func, name, ...) register_function<decltype(&func), &func, decltype(name##_create)::value, nullptr, MAKE_STR_PARAMETERS_N(__VA_ARGS__)>
#define REG_BASIC_TYPE(type, func, ret, name) register_function<type, func, ret, decltype(name##_create)::value>
#define REG_USER(type, func_type, ret_type, name) register_function<type, func_type, ret_type, decltype(name##_create)::value>
#define REG_ITR(type, func_type, ret_type, name) register_iterator<type, func_type, ret_type, decltype(name##_create)::value>
#define REG_EVERY(handle_type, func, name) register_every<handle_type, decltype(&func), &func, decltype(name##_create)::value>
#define REG_HAS(handle_type, func, name) register_has<handle_type, decltype(&func), &func, decltype(name##_create)::value>
#define REG_RANDOM(handle_type, func, name) register_random<handle_type, decltype(&func), &func, decltype(name##_create)::value>
#define REG_VIEW(handle_type, func, name) register_view<handle_type, decltype(&func), &func, decltype(name##_create)::value>

#define REG_FUNC_R(handle_type, func, name, react) register_function<handle_type, decltype(&func), &func, decltype(name##_create)::value, react>
#define REG_BASIC_R(func, name, react) register_function<decltype(&func), &func, decltype(name##_create)::value, react>
#define REG_CONST_ARGS_R(handle_type, func, name, react, ...) register_function<handle_type, decltype(&func), &func, decltype(name##_create)::value, react, __VA_ARGS__>
//#define REG_ARGS_R(handle_type, func, name, react) register_function_with_args<handle_type, decltype(&func), &func, decltype(name##_create)::value, react>
//#define REG_BASIC_ARGS_R(func, name, react) register_function_with_args<decltype(&func), &func, decltype(name##_create)::value, react>
#define REG_NAMED_ARGS_R(handle_type, func, name, react, ...) register_function<handle_type, decltype(&func), &func, decltype(name##_create)::value, react, MAKE_STR_PARAMETERS_N(__VA_ARGS__)>
#define REG_BASIC_NAMED_ARGS_R(func, name, react, ...) register_function<decltype(&func), &func, decltype(name##_create)::value, react, MAKE_STR_PARAMETERS_N(__VA_ARGS__)>
#define REG_BASIC_TYPE_R(type, func, ret, name, react) register_function<type, func, ret, decltype(name##_create)::value, react>

#define FUNCTION_ARGS_T(handle_type, func, name, ...) script::scripted_function_args<handle_type, decltype(&func), &func, decltype(name##_create)::value, nullptr, ##__VA_ARGS__>
#define FUNCTION_ARGS_RT(handle_type, func, name, react, ...) script::scripted_function_args<handle_type, decltype(&func), &func, decltype(name##_create)::value, react, ##__VA_ARGS__>

#define BASIC_FUNCTION_ARGS_T(func, name, ...) script::scripted_function_args<decltype(&func), &func, decltype(name##_create)::value, nullptr, ##__VA_ARGS__>
#define BASIC_FUNCTION_ARGS_RT(func, name, react, ...) script::scripted_function_args<decltype(&func), &func, decltype(name##_create)::value, react, ##__VA_ARGS__>

    /* ====================================================================================================================================================================== */
    /* ====================================================================================================================================================================== */
    /* ====================================================================================================================================================================== */
    /* ====================================================================================================================================================================== */
    /* ====================================================================================================================================================================== */

//     sizeof(T) <= object::mem_size && alignof(T) <= alignof(object) &&
//     std::is_trivially_destructible_v<T> && std::is_copy_constructible_v<T> &&
//     !(std::is_pointer_v<T> && std::is_fundamental_v<std::remove_reference_t<std::remove_pointer_t<T>>>) &&
//     !std::is_same_v<T, std::string_view*> &&
//     !std::is_same_v<T, const std::string_view*>

    template <typename T> // , const char* name
    void system::register_usertype() {
      if constexpr (!std::is_same_v<T, void>) {
        static_assert(is_valid_type_for_object_v<T>,
          "Type must be placeable in object::mem_size, must be trivially destructible, copy constructible, must not be pointer to fundamental type, must not be pointer to std::string_view"
        );
      }
      const auto itr = func_map.find(type_id<T>());
      if (itr != func_map.end()) throw std::runtime_error("Type " + std::string(type_name<T>()) + " is already registered");
      func_map[type_id<T>()];
      //register_function<void, type_checker<T, name>, bool, name>();
    }

    template <typename F, size_t N, size_t cur>
    static std::string make_function_args_string(std::string str = "") {
      if constexpr (N == cur) return str;
      else {
        using cur_arg = final_arg_type<F, cur>;
        return make_function_args_string<F, N, cur+1>(str + std::string(type_name<cur_arg>()) + " ");
      }
      return "";
    }

    template <typename F, size_t N, size_t cur, const char* cur_name, const char*... args_names>
    static std::string make_function_args_string(std::string str = "") {
      if constexpr (N == cur) return str;
      else {
        //using cur_arg = final_arg_type<F, cur>;
        return make_function_args_string<F, N, cur+1, args_names...>(str + std::string(cur_name) + " "); // только имя добавить? думаю что да
      }
      return "";
    }

    template <typename F>
    static std::string make_function_const_args_string() {
      return "";
    }

    template <typename F, size_t head, size_t... args>
    static std::string make_function_const_args_string() {
      return std::to_string(head) + " " + make_function_const_args_string<F, args...>();
    }

    template <typename T>
    interface* system::create_argument(const std::string_view &name, init_context* ctx, const sol::object &obj, container* cont) {
      interface* local = nullptr;
      static_assert(!std::is_void_v<T>, "Bad input type for function");
      // Santa Claus please bring static reflection to c++, I beg you
      if constexpr (std::is_enum_v<T>) {
        // что тут? ожидаем строку, и пытаемся ее как то преобразовать в число
        // мы можем задать просто мапу строка-число, но нам она нужна только при создании скриптов
        // (строго говоря нам и класс система нужен только при создании, если отсюда забрать контейнеры)
        // нужно тогда сделать возможность подчистить энумы после создания
        if (obj.get_type() != sol::type::string) throw std::runtime_error("Enum input expects string with enum value name for enum type '" + std::string(type_name<T>()) + "'");
        const auto str = obj.as<std::string_view>();
        const auto itr = enum_map.find(type_id<T>());
        if (itr == enum_map.end()) throw std::runtime_error("Could not find enum type '" + std::string(type_name<T>()) + "'");
        const auto val_itr = itr->second.find(str);
        if (val_itr == itr->second.end()) throw std::runtime_error("Could not find enum value '" + std::string(str) + "' in enum type '" + std::string(type_name<T>()) + "'");
        // nearly impossible to tell what enum type the object has, better to use int64_t as general container
        if (cont != nullptr) local = cont->add<general_container<int64_t>>(val_itr->second);
      } else
      if constexpr (is_forward_list_v<T> || is_forward_list_v<optional_type<T>>) {
        using final_arg = typename std::conditional_t<is_optional_v<T>, forward_list_type<optional_type<T>>, forward_list_type<T>>;
        // мы ожидаем во первых что это последний аргумент (проверяем не тут)
        // и ожидаем здесь таблицу (можно ли здесь ожидать строку? по идее можно)
        if (obj.get_type() == sol::type::string) return create_argument<final_arg>(name, ctx, obj, cont);
        else if (obj.get_type() == sol::type::table) {
          const auto t = obj.as<sol::table>();
          interface* child = nullptr;
          for (const auto &p : t) {
            // ожидаем только перечисление
            if (p.first.get_type() != sol::type::number) continue;

            auto cur = create_argument<final_arg>(name, ctx, p.second, cont);
            if (local == nullptr) local = cur;
            if (child != nullptr) child->next = cur;
            child = cur;
          }
        } else throw std::runtime_error("Expected string or table for list argument");
      } else {
        if constexpr (std::is_same_v<T, bool>) local = make_raw_script_boolean(ctx, obj, cont);
        else if constexpr (std::is_same_v<T, double>) local = make_raw_script_number(ctx, obj, cont);
        else if constexpr (std::is_same_v<T, std::string_view>) local = make_raw_script_string(ctx, obj, cont);
        else {
          if (func_map.find(type_id<T>()) == func_map.end()) throw std::runtime_error("Type '" + std::string(type_name<T>()) + "' was not registered");
          change_expected_type cet(ctx, type_id<T>());
          local = make_raw_script_object(ctx, obj, cont);
        }
      }

      return local;
    }

    template <typename T>
    interface* system::create_default_argument(const std::string_view &, const std::string_view &arg_name, const std::any &default_val, container* cont) {
      interface* local = nullptr;
      if (!default_val.has_value()) throw std::runtime_error("Function argument '" + std::string(arg_name) + "' must be specified");
      static_assert(!std::is_void_v<T>, "Bad input type for function");
//       if constexpr (utils::is_array_view_v<T>) {
//         using final_arg = utils::array_view_type<T>;
//         // тут по идее мы должны получить std vector<T>, аллокатор?
//         // и засунуть этот array_view в функцию, которая просто его вернет
//         assert(false && "Not supported yet");
//       } else
      // по идее конкретно тут НЕ ДОЛЖНО быть optional
      if constexpr (is_forward_list_v<T> || is_forward_list_v<optional_type<T>>) {
        using final_arg = typename std::conditional_t<is_optional_v<T>, forward_list_type<optional_type<T>>, forward_list_type<T>>;
        // как тут? ожидаем видимо массив
        const auto val = std::any_cast<std::vector<final_arg>>(&default_val);
        if (val == nullptr) throw std::runtime_error("Expected '" + std::string(type_name<std::vector<final_arg>>()) + "' as default value");
        interface* child = nullptr;
        for (auto &obj : *val) {
          auto cur = make_object_container(object(obj), cont);
          if (local == nullptr) local = cur;
          if (child != nullptr) child->next = cur;
          child = cur;
        }
      } else {
        // нужно еще сделать дефолтный контейнер для array_view
        if constexpr (std::is_same_v<T, bool>) {
          const bool val = std::any_cast<bool>(default_val);
          local = make_boolean_container(val, cont);
        } else if constexpr (std::is_same_v<T, double>) {
          const double val = std::any_cast<double>(default_val);
          local = make_number_container(val, cont);
        } else if constexpr (std::is_same_v<T, std::string_view>) {
          if (const auto ptr = std::any_cast<std::string_view>(&default_val); ptr != nullptr) {
            const auto &val = *ptr;
            local = make_string_container(val, cont);
          } else if (const auto ptr = std::any_cast<std::string>(&default_val); ptr != nullptr) {
            const auto &val = *ptr;
            local = make_string_container(val, cont);
          } else if (const auto ptr = std::any_cast<const char*>(&default_val); ptr != nullptr) {
            const auto &val = *ptr;
            local = make_string_container(val, cont);
          } else throw std::runtime_error("Function argument default value '" + std::string(arg_name) + "' is not a string");
        }
        // else if constexpr (utils::is_array_view_v<T>) {
        //   using final_type = utils::array_view_type<T>;
        //   // нужно наверное скопировать туда целый массив? вообще по идее в конкретном случае мы можем хранить собственно только array_view
        //   // по идее default_val не исчезает никуда и мы можем сохранить только view
        //   auto ptr = std::any_cast<std::vector<final_type>>(&default_val);
        //   if (ptr == nullptr) throw std::runtime_error("Expected '" + std::string(type_name<std::vector<final_type>>()) + "' as default value");
        //   if (cont != nullptr) local = cont->add<view_container>(*ptr);
        // }
        else if constexpr (is_span_v<T>) {
          using final_type = span_type<T>;
          auto ptr = std::any_cast<std::vector<final_type>>(&default_val);
          if (ptr == nullptr) throw std::runtime_error("Expected '" + std::string(type_name<std::vector<final_type>>()) + "' as default value");
          if (cont != nullptr) local = cont->add<span_container>(*ptr);
        } else if constexpr (std::is_enum_v<T>) {
          const auto &val = std::any_cast<T>(default_val);
          if (cont != nullptr) local = cont->add<general_container<int64_t>>(static_cast<int64_t>(val));
        } else {
          const auto &val = std::any_cast<T>(default_val);
          local = make_object_container(object(val), cont);
        }
      }
      return local;
    }

    // можем ли мы это сделать для произвольного числа входных данных? по крайней мере для нескольких первых
//     template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
//     void system::register_function() {
//       using output_type = final_output_type<F>;
//       constexpr size_t script_type = get_script_type<output_type>();
//       constexpr size_t arg_count = get_function_argument_count<F>() - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>);
//       //std::is_same_v<function_member_of<F>, T> &&
//       //constexpr bool no_args = (arg_count == 0) || (!is_member && arg_count == 1 && std::is_invocable_v<F, Th>);
//       //static_assert(no_args);
//
//       init_func_data ifd{
//         script_type, type_id<Th>(), type_id<output_type>(), arg_count,
//         [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
//           constexpr size_t arg_count = get_function_argument_count<F>() - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>);
//           constexpr size_t script_type = get_script_type<output_type>();
//           check_script_types(name, ctx->script_type, script_type);
//           change_current_function_name ccfn(ctx, name);
//           change_compare_op cco(ctx, compare_operators::more_eq);
//
//           if constexpr (arg_count == 0) {
//             using function_type = scripted_function_no_arg<Th, F, f, name, react>;
//
//             //using input_type = function_argument_type<F, 0>;
//             //static_assert(std::is_same_v<std::remove_reference_t<std::remove_cv_t<input_type>>, Th>, "Registering a handle function is not allowed");
//             // тут мы можем создать смену контекста если функция возвращает какой то объект
//
//             ctx->add_function<function_type>();
//             // теперь можно передать container по-умолчанию и избавиться от проверок указателя
//             interface* cur = nullptr;
//             if (cont != nullptr) { cur = cont->add<function_type>(); }
//             if (obj.valid()) {
//               if constexpr (std::is_same_v<output_type, bool>) {
//                 auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
//                 return sys->make_raw_boolean_compare(ctx, cur, rvalue, cont);
//               } else if constexpr (std::is_same_v<output_type, double>) {
//                 change_compare_op cco(ctx, compare_operators::more_eq);
//                 auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
//                 return sys->make_raw_number_compare(ctx, cur, rvalue, cont);
//               } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
//               else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
//             }
//
//             return cur;
//           } else {
//             //using function_type = scripted_function_one_arg<T, Th, F, f, name>;
//             //using function_type = scripted_function_scripted_args<T, Th, F, f, name>;
//             //constexpr bool first_is_handle = std::is_same_v<Th, final_arg_type<F, 0>>;
//             using function_type = scripted_function_one_arg<Th, F, f, name, react>;
//             using input_type = typename std::conditional< !is_member_v<Th, F>, final_arg_type<F, 1>, final_arg_type<F, 0> >::type;
//             static_assert(arg_count == 1 || (!is_member_v<Th, F> && arg_count == 2), "If function has more then 1 args use another function");
//             static_assert(is_valid_type_for_object_v<input_type>, "Argument type must be suitable for script::object");
//
//             if (!obj.valid()) throw std::runtime_error("Function '" + std::string(name) + "' expects input");
//
//             size_t offset = SIZE_MAX;
//             if (cont != nullptr) offset = cont->add_delayed<function_type>();
//             ctx->add_function<function_type>();
//
//             interface* inter = sys->create_argument<input_type>(name, ctx, obj, cont);
//             interface* cur = nullptr;
//             if (cont != nullptr) {
//               auto init = cont->get_init<function_type>(offset);
//               cur = init.init(inter);
//             }
//
//             return cur;
//           }
//         }, [] () {
//           constexpr bool is_independent_func = !is_member_v<Th, F>;
//           if constexpr (arg_count == 0) return std::string(name) + " I: " + std::string(type_name<Th>()) + " O: " + std::string(type_name<output_type>());
//           else return std::string(name) + " I: " + std::string(type_name<Th>()) +
//                                           " O: " + std::string(type_name<output_type>()) +
//                                           " Args: " + make_function_args_string<F, get_function_argument_count<F>(), is_independent_func>();
//         }
//       };
//
//       register_function<Th, name>(std::move(ifd));
//     }

    // к нам тут должна приходить таблица с входными данными
    // из нее по именам мы должны забрать нужные нам скрипты
    // только по именам? нет, может быть еще и по индексам, тогда все более менее просто
    // если по именам то нужно будет передавать список имен куда то
//     template <typename F, size_t N, size_t cur>
//     interface* system::make_arguments(
//       const std::string_view &name, const std::vector<std::string> &args_name,
//       init_context* ctx, const sol::table &t, container* cont,
//       interface* begin, interface* current
//     ) {
//       if constexpr (cur == N) return begin;
//       else {
//         const auto proxy = t[args_name[cur]];
//         if (!proxy.valid()) throw std::runtime_error("Could not find argument " + args_name[cur] + " for function '" + std::string(name) + "'");
//
//         using input_type = final_arg_type<F, cur>;
//         static_assert(is_valid_type_for_object_v<input_type>, "Argument type must be suitable for script::object");
//         interface* local = create_argument<input_type>(name, ctx, proxy, cont);
//         if (current != nullptr) current->next = local;
//         return make_arguments<F, N, cur+1>(name, args_name, ctx, t, cont, begin == nullptr ? local : begin, local);
//       }
//
//       return nullptr;
//     }
//
//     template <typename F, size_t N, size_t cur>
//     interface* system::make_arguments(const std::string_view &name, init_context* ctx, const sol::table &t, container* cont, interface* begin, interface* current) {
//       if constexpr (cur == N) return begin;
//       else {
//         const auto proxy = t[TO_LUA_INDEX(cur)];
//         if (!proxy.valid()) throw std::runtime_error("Could not find argument " + std::to_string(TO_LUA_INDEX(cur)) + " for function " + std::string(name));
//
//         using input_type = final_arg_type<F, cur>;
//         static_assert(is_valid_type_for_object_v<input_type>, "Argument type must be suitable for script::object");
//         interface* local = create_argument<input_type>(name, ctx, proxy, cont);
//         if (current != nullptr) current->next = local;
//         return make_arguments<F, N, cur+1>(name, ctx, t, cont, begin == nullptr ? local : begin, local);
//       }
//
//       return nullptr;
//     }
//
//     template <typename F, size_t N, size_t cur, const char* cur_arg, const char*... args_names>
//     interface* system::make_arguments(const std::string_view &name, init_context* ctx, const sol::table &t, container* cont, interface* begin, interface* current) {
//       static_assert(cur + sizeof...(args_names) + 1 == N);
//       if constexpr (cur == N) return begin;
//       else {
//         const auto proxy = t[cur_arg];
//         if (!proxy.valid()) throw std::runtime_error("Could not find argument " + std::to_string(TO_LUA_INDEX(cur)) + " for function " + std::string(name));
//
//         using input_type = final_arg_type<F, cur>;
//         static_assert(is_valid_type_for_object_v<input_type>, "Argument type must be suitable for script::object");
//         interface* local = create_argument<input_type>(name, ctx, proxy, cont);
//         if (current != nullptr) current->next = local;
//         return make_arguments<F, N, cur+1, args_names...>(name, ctx, t, cont, begin == nullptr ? local : begin, local);
//       }
//
//       return nullptr;
//     }

    template <typename F, size_t N, size_t cur, bool first_arg>
    interface* system::make_arguments(
      const std::string_view &name,
      const std::vector<std::string> &args_name,
      const std::vector<std::any> &args_default_val,
      init_context* ctx,
      const sol::object &obj,
      container* cont,
      interface* begin,
      interface* current
    ) {
      if constexpr (cur == N) return begin;
      else {
        using input_type = final_arg_type<F, cur>;
        constexpr bool first_and_only = (N - cur == 1) && first_arg;

        sol::object current_obj;
        const bool input_is_table = obj.valid() && obj.get_type() == sol::type::table;
        if constexpr (first_and_only) {
          current_obj = obj;
        } else {
          if (input_is_table) {
            const auto t = obj.as<sol::table>();
            current_obj = t[args_name[cur]];
          } else {
            current_obj = obj;
          }
        }

        // фовард лист должен быть ПОСЛЕДНИМ
        if constexpr (is_forward_list_v<input_type> || is_forward_list_v<optional_type<input_type>>) {
          static_assert(cur+1 == N, "Forward list must be last argument");
        }

        interface* local = nullptr;
        if constexpr (is_optional_v<input_type>) {
          using final_type = optional_type<input_type>;
          static_assert(is_valid_type_for_object_v<final_type>, "Argument type must be suitable for script::object");
          if (current_obj.valid()) local = create_argument<final_type>(name, ctx, current_obj, cont);
          else local = make_invalid_producer(cont);
        } else {
          static_assert(is_valid_type_for_object_v<input_type>, "Argument type must be suitable for script::object");
          const size_t offset = N >= args_default_val.size() ? (!args_default_val.empty() ? N - args_default_val.size() : SIZE_MAX) : 0;
          if (current_obj.valid()) local = create_argument<input_type>(name, ctx, current_obj, cont);
          else if (cur >= offset) local = create_default_argument<input_type>(name, args_name[cur], args_default_val[cur-offset], cont);
          else throw std::runtime_error("Could not find argument " + args_name[cur] + " for function '" + std::string(name) + "'");
        }

        if (current != nullptr) current->next = local;
        return make_arguments<F, N, cur+1, false>(name, args_name, args_default_val, ctx, input_is_table ? obj : sol::object(), cont, begin == nullptr ? local : begin, local);
      }

      return nullptr;
    }

    template <typename F, size_t N, size_t cur, bool first_arg>
    interface* system::make_arguments(
      const std::string_view &name, const std::vector<std::any> &args_default_val,
      init_context* ctx, const sol::object &obj, container* cont,
      interface* begin, interface* current
    ) {
      if constexpr (cur == N) return begin;
      else {
        using input_type = final_arg_type<F, cur>;
        constexpr bool first_and_only = (N - cur == 1) && first_arg;

        sol::object current_obj;
        const bool input_is_table = obj.valid() && obj.get_type() == sol::type::table;
        if constexpr (first_and_only) {
          current_obj = obj;
        } else {
          if (input_is_table) {
            const auto t = obj.as<sol::table>();
            current_obj = t[TO_LUA_INDEX(cur)];
          } else {
            current_obj = obj;
          }
        }

        // фовард лист должен быть ПОСЛЕДНИМ
        if constexpr (is_forward_list_v<input_type> || is_forward_list_v<optional_type<input_type>>) {
          static_assert(cur+1 == N, "Forward list must be last argument");
        }

        interface* local = nullptr;
        if constexpr (is_optional_v<input_type>) {
          using final_type = optional_type<input_type>;
          static_assert(is_valid_type_for_object_v<final_type>, "Argument type must be suitable for script::object");
          if (current_obj.valid()) local = create_argument<final_type>(name, ctx, current_obj, cont);
          else local = make_invalid_producer(cont);
        } else {
          static_assert(is_valid_type_for_object_v<input_type>, "Argument type must be suitable for script::object");
          const size_t offset = N >= args_default_val.size() ? (!args_default_val.empty() ? N - args_default_val.size() : SIZE_MAX) : 0;
          if (current_obj.valid()) local = create_argument<input_type>(name, ctx, current_obj, cont);
          else if (cur >= offset) local = create_default_argument<input_type>(name, std::to_string(TO_LUA_INDEX(cur)), args_default_val[cur-offset], cont);
          else throw std::runtime_error("Could not find argument " + std::to_string(TO_LUA_INDEX(cur)) + " for function '" + std::string(name) + "'");
        }

        if (current != nullptr) current->next = local;
        return make_arguments<F, N, cur+1, false>(name, args_default_val, ctx, input_is_table ? obj : sol::object(), cont, begin == nullptr ? local : begin, local);
      }

      return nullptr;
    }

    // может быть таблица для одного аргумента, здесь у нас нет возможности проверить является ли первый аргумент тут объектом для которого запустится функция F
    // по идее достаточно только передать флаг первый ли это аргумент, любой рекурсивный вызов - false
    template <typename F, size_t N, size_t cur, bool first_arg, const char* cur_arg, const char*... args_names>
    interface* system::make_arguments(
      const std::string_view &name, const std::vector<std::any> &args_default_val,
      init_context* ctx, const sol::object &obj, container* cont,
      interface* begin, interface* current
    ) {
      static_assert(cur + sizeof...(args_names) + 1 == N);
      if constexpr (cur == N) return begin;
      else {
        using input_type = final_arg_type<F, cur>;
        constexpr bool first_and_only = (N - cur == 1) && first_arg;

        sol::object current_obj;
        const bool input_is_table = obj.valid() && obj.get_type() == sol::type::table;
        if constexpr (first_and_only) {
          current_obj = obj;
        } else {
          if (input_is_table) {
            const auto t = obj.as<sol::table>();
            current_obj = t[cur_arg];
          } else {
            current_obj = obj;
          }
        }

        // фовард лист должен быть ПОСЛЕДНИМ
        if constexpr (is_forward_list_v<input_type> || is_forward_list_v<optional_type<input_type>>) {
          static_assert(cur+1 == N, "Forward list must be last argument");
        }

        interface* local = nullptr;
        if constexpr (is_optional_v<input_type>) {
          using final_type = optional_type<input_type>;
          static_assert(is_valid_type_for_object_v<final_type>, "Argument type must be suitable for script::object");
          if (current_obj.valid()) local = create_argument<final_type>(name, ctx, current_obj, cont);
          else local = make_invalid_producer(cont);
        } else {
          static_assert(is_valid_type_for_object_v<input_type>, "Argument type must be suitable for script::object");
          const size_t offset = N >= args_default_val.size() ? (!args_default_val.empty() ? N - args_default_val.size() : SIZE_MAX) : 0;
          if (current_obj.valid()) local = create_argument<input_type>(name, ctx, current_obj, cont);
          else if (cur >= offset) local = create_default_argument<input_type>(name, cur_arg, args_default_val[cur-offset], cont);
          else throw std::runtime_error("Could not find argument '" + std::string(cur_arg) + "' for function '" + std::string(name) + "'");
        }

        if (current != nullptr) current->next = local;
        return make_arguments<F, N, cur+1, false, args_names...>(name, args_default_val, ctx, input_is_table ? obj : sol::object(), cont, begin == nullptr ? local : begin, local);
      }

      return nullptr;
    }

    // как сюда добавить аргументы по умолчанию? теперь поддерживается std optional
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    void system::register_function(const std::vector<std::string> &args_name, const std::vector<std::any> &args_default_val) {
      constexpr size_t func_arg_count = get_function_argument_count<F>() - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>);

      //if constexpr (func_arg_count == 0) { register_function<Th, F, f, name, react>(); return; }

      if (args_name.size() < func_arg_count)
        throw std::runtime_error("Function expects " + std::to_string(func_arg_count) + " arguments, "
                                  "but receive only " + std::to_string(args_name.size()) + " arguments names");

      // по идее необязательно, в текущем виде аккуратно выкинет ошибку если я чего то забыл
      // вообще наверное имеет смысл сделать так чтобы если дефолтные значения меньше размером, чем количество аргументов
      // то мы на разницу между количеством и дефолтными значениями, пропускаем вначале аргументы, потому что обычно
      // последние аргументы - ненужные
//       if (!args_default_val.empty() && args_name.size() != args_default_val.size())
//         throw std::runtime_error("Please, provide array with default values with same size as array with arguments names");

      using output_type = final_output_type<F>;
      constexpr size_t script_type = get_script_type<output_type>();

      init_func_data ifd{
        script_type, type_id<Th>(), type_id<output_type>(), func_arg_count,
        [args_name, args_default_val] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          change_current_function_name ccfn(ctx, name);
          change_compare_op cco(ctx, compare_operators::more_eq);

          constexpr size_t final_arg_count = get_function_argument_count<F>(); //func_arg_count; //  - std::is_same_v<function_member_of<F>, void>
          constexpr bool no_args = final_arg_count - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>) == 0;
          // нам нужно это дело друг от друга отделить потому что, в функции без аргументов может придти сравнение
          if constexpr (no_args) {
            using function_type = scripted_function_no_arg<Th, F, f, name, react>;

            ctx->add_function<function_type>();
            // теперь можно передать container по-умолчанию и избавиться от проверок указателя
            interface* cur = nullptr;
            if (cont != nullptr) { cur = cont->add<function_type>(); }
            if (obj.valid()) {
              if constexpr (std::is_same_v<output_type, bool>) {
                auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
                return sys->make_raw_boolean_compare(ctx, cur, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, double>) {
                change_compare_op cco(ctx, compare_operators::more_eq);
                auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
                return sys->make_raw_number_compare(ctx, cur, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
              else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
            }

            return cur;
          } else {
            using function_type = scripted_function_scripted_args<Th, F, f, name, react>;
            // если это не метод, то первый слот искать не нужно
            constexpr bool is_independent_func = !is_member_v<Th, F>;
            if constexpr (is_independent_func) { static_assert(std::is_same_v<final_arg_type<F, 0>, Th>); }

            // всегда тут ожидаем таблицу?
            constexpr size_t func_arg_count = get_function_argument_count<F>() - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>);
            const size_t diff = func_arg_count > args_default_val.size() ? func_arg_count - args_default_val.size() : 0;
            if (diff > 1 && obj.get_type() != sol::type::table) throw std::runtime_error("Function " + std::string(name) + " expects table with arguments");

            size_t offset = SIZE_MAX;
            if (cont != nullptr) offset = cont->add_delayed<function_type>();
            ctx->add_function<function_type>();

            auto args = sys->make_arguments<F, final_arg_count, is_independent_func, true>(name, args_name, args_default_val, ctx, obj, cont);

            interface* cur = nullptr;
            if (cont != nullptr) {
              auto init = cont->get_init<function_type>(offset);
              cur = init.init(args);
            }

            return cur;
          }
        }, [] () {
          constexpr bool is_independent_func = !is_member_v<Th, F>;
          if constexpr (func_arg_count == 0) return std::string(name) + " I: " + std::string(type_name<Th>()) + " O: " + std::string(type_name<output_type>());
          else return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                          " O: " + std::string(type_name<output_type>()) +
                                          " Args: " + make_function_args_string<F, get_function_argument_count<F>(), is_independent_func>();
        }
      };

      register_function<Th, name>(std::move(ifd));
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    void system::register_function(const std::vector<std::any> &args_default_val) {
      constexpr size_t func_arg_count = get_function_argument_count<F>() - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>);
      //if constexpr (func_arg_count < 2) { register_function<Th, F, f, name, react>();  return; }

      using output_type = final_output_type<F>;
      constexpr size_t script_type = get_script_type<output_type>();

      init_func_data ifd{
        script_type, type_id<Th>(), type_id<output_type>(), func_arg_count,
        [args_default_val] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          change_current_function_name ccfn(ctx, name);
          change_compare_op cco(ctx, compare_operators::more_eq);

          constexpr size_t final_arg_count = get_function_argument_count<F>();
          constexpr bool no_args = final_arg_count - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>) == 0;
          // нам нужно это дело друг от друга отделить потому что, в функции без аргументов может придти сравнение
          if constexpr (no_args) {
            using function_type = scripted_function_no_arg<Th, F, f, name, react>;

            ctx->add_function<function_type>();
            // теперь можно передать container по-умолчанию и избавиться от проверок указателя
            interface* cur = nullptr;
            if (cont != nullptr) { cur = cont->add<function_type>(); }
            if (obj.valid()) {
              if constexpr (std::is_same_v<output_type, bool>) {
                auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
                return sys->make_raw_boolean_compare(ctx, cur, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, double>) {
                change_compare_op cco(ctx, compare_operators::more_eq);
                auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
                return sys->make_raw_number_compare(ctx, cur, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
              else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
            }

            return cur;
          } else {
            using function_type = scripted_function_scripted_args<Th, F, f, name, react>;
            constexpr bool is_independent_func = std::is_same_v<function_member_of<F>, void>;
            if constexpr (is_independent_func) { static_assert(std::is_same_v<final_arg_type<F, 0>, Th>); }

            // всегда тут ожидаем таблицу?
            constexpr size_t func_arg_count = get_function_argument_count<F>() - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>);
            const size_t diff = func_arg_count > args_default_val.size() ? func_arg_count - args_default_val.size() : 0;
            if (diff > 1 && obj.get_type() != sol::type::table) throw std::runtime_error("Function " + std::string(name) + " expects table with arguments");

            size_t offset = SIZE_MAX;
            if (cont != nullptr) offset = cont->add_delayed<function_type>();
            ctx->add_function<function_type>();

            auto args = sys->make_arguments<F, final_arg_count, is_independent_func, true>(name, args_default_val, ctx, obj, cont);

            interface* cur = nullptr;
            if (cont != nullptr) {
              auto init = cont->get_init<function_type>(offset);
              cur = init.init(args);
            }

            return cur;
          }
        }, [] () {
          constexpr bool is_independent_func = !is_member_v<Th, F>;
          if constexpr (func_arg_count == 0) return std::string(name) + " I: " + std::string(type_name<Th>()) + " O: " + std::string(type_name<output_type>());
          else return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                          " O: " + std::string(type_name<output_type>()) +
                                          " Args: " + make_function_args_string<F, get_function_argument_count<F>(), is_independent_func>();
        }
      };

      register_function<Th, name>(std::move(ifd));
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_arg, const char*... args_names>
    void system::register_function(const std::vector<std::any> &args_default_val) {
      constexpr size_t func_arg_count = get_function_argument_count<F>() - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>);
      //if constexpr (func_arg_count == 0) { register_function<Th, F, f, name, react>();  return; }
      constexpr size_t names_count = sizeof...(args_names) + 1;
      static_assert(func_arg_count == names_count, "Arguments names count must be equal function arguments count");

      using output_type = final_output_type<F>;
      constexpr size_t script_type = get_script_type<output_type>();

      init_func_data ifd{
        script_type, type_id<Th>(), type_id<output_type>(), func_arg_count,
        [args_default_val] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          change_current_function_name ccfn(ctx, name);
          change_compare_op cco(ctx, compare_operators::more_eq);

          constexpr size_t final_arg_count = get_function_argument_count<F>();
          constexpr bool no_args = final_arg_count - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>) == 0;
          // нам нужно это дело друг от друга отделить потому что, в функции без аргументов может придти сравнение
          if constexpr (no_args) {
            using function_type = scripted_function_no_arg<Th, F, f, name, react>;

            ctx->add_function<function_type>();
            // теперь можно передать container по-умолчанию и избавиться от проверок указателя
            interface* cur = nullptr;
            if (cont != nullptr) { cur = cont->add<function_type>(); }
            if (obj.valid()) {
              if constexpr (std::is_same_v<output_type, bool>) {
                auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
                return sys->make_raw_boolean_compare(ctx, cur, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, double>) {
                change_compare_op cco(ctx, compare_operators::more_eq);
                auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
                return sys->make_raw_number_compare(ctx, cur, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
              else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
            }

            return cur;
          } else {
            using function_type = scripted_function_scripted_named_args<Th, F, f, name, react, first_arg, args_names...>;

            constexpr bool is_independent_func = std::is_same_v<function_member_of<F>, void>;
            if constexpr (is_independent_func) { static_assert(std::is_same_v<final_arg_type<F, 0>, Th>); }

            // всегда тут ожидаем таблицу?
            constexpr size_t func_arg_count = get_function_argument_count<F>() - size_t(!is_member_v<Th, F> && std::is_same_v<final_arg_type<F, 0>, Th>);
            const size_t diff = func_arg_count > args_default_val.size() ? func_arg_count - args_default_val.size() : 0;
            if (diff > 1 && obj.get_type() != sol::type::table) throw std::runtime_error("Function " + std::string(name) + " expects table with arguments");

            size_t offset = SIZE_MAX;
            if (cont != nullptr) offset = cont->add_delayed<function_type>();
            ctx->add_function<function_type>();

            auto args = sys->make_arguments<F, final_arg_count, is_independent_func, true, first_arg, args_names...>(name, args_default_val, ctx, obj, cont);

            interface* cur = nullptr;
            if (cont != nullptr) {
              auto init = cont->get_init<function_type>(offset);
              cur = init.init(args);
            }

            return cur;
          }
        }, [] () {
          constexpr bool is_independent_func = !is_member_v<Th, F>;
          if constexpr (func_arg_count == 0) return std::string(name) + " I: " + std::string(type_name<Th>()) + " O: " + std::string(type_name<output_type>());
          else return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                          " O: " + std::string(type_name<output_type>()) +
                                          " Args: " + make_function_args_string<F, get_function_argument_count<F>(), is_independent_func, first_arg, args_names...>();
        }
      };

      register_function<Th, name>(std::move(ifd));
    }

    // тут по идее нужно добавить регистрацию с пользовательской функцией, но это приведет
    // к тому что эту функцию мы будем копировать по миллиону раз, а это глупо

    // функции которые принимают на вход объект и возвращают другой без вызова методов
//     template <typename F, F f, const char* name, on_effect_func_t react>
//     void system::register_function() {
//       constexpr size_t func_arg_count = get_function_argument_count<F>();
//       using output_type = final_output_type<F>;
//       constexpr size_t script_type = get_script_type<output_type>();
//       using register_type = void; // это тип по которому зарегистрируем функцию
//
//       static_assert(type_id<std::string_view>() == type_id<std::basic_string_view<char>>()); // sanity
//
//       init_func_data ifd{
//         script_type, SCRIPT_SYSTEM_ANY_TYPE, type_id<output_type>(), func_arg_count,
//         [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
//           constexpr size_t func_arg_count = get_function_argument_count<F>();
//           constexpr size_t script_type = get_script_type<output_type>();
//           check_script_types(name, ctx->script_type, script_type);
//           change_current_function_name ccfn(ctx, name);
//           change_compare_op cco(ctx, compare_operators::more_eq);
//
//           if constexpr (func_arg_count == 0) {
//             using function_type = basic_function_no_arg<F, f, name, react>;
//             // создадим сравнение?
//             ctx->add_function<function_type>();
//             interface* ret = nullptr;
//             if (cont != nullptr) ret = cont->add<function_type>();
//             if (obj.valid()) {
//               if constexpr (std::is_same_v<output_type, bool>) {
//                 auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
//                 return sys->make_raw_boolean_compare(ctx, ret, rvalue, cont);
//               } else if constexpr (std::is_same_v<output_type, double>) {
//                 change_compare_op cco(ctx, compare_operators::more_eq);
//                 auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
//                 return sys->make_raw_number_compare(ctx, ret, rvalue, cont);
//               } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
//               else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
//             }
//             return ret;
//           } else {
//             using function_type = basic_function_scripted_arg<F, f, name, react>;
//
//             if (!obj.valid()) throw std::runtime_error("Function '" + std::string(name) + "' expects input");
//
//             size_t offset = SIZE_MAX;
//             if (cont != nullptr) offset = cont->add_delayed<function_type>();
//             ctx->add_function<function_type>();
//
//             using input_type = final_arg_type<F, 0>;
//             static_assert(is_valid_type_for_object_v<input_type>, "Argument type must be suitable for script::object");
//
//             interface* inter = sys->create_argument<input_type>(name, ctx, obj, cont);
//             interface* cur = nullptr;
//             if (cont != nullptr) {
//               auto init = cont->get_init<function_type>(offset);
//               cur = init.init(inter);
//             }
//
//             return cur;
//           }
//
//           return nullptr;
//         }, [] () {
//           if constexpr (func_arg_count == 0) return std::string(name) + " I: " + std::string(type_name<void>()) + " O: " + std::string(type_name<output_type>());
//           else return std::string(name) + " I: " + std::string(type_name<void>()) +
//                                           " O: " + std::string(type_name<output_type>()) +
//                                           " Args: " + make_function_args_string<F, func_arg_count, 0>();
//         }
//       };
//
//       register_function<register_type, name>(std::move(ifd));
//     }

    // тут нужно еще указать массив std::vector<std::any> optionals
    template <typename F, F f, const char* name, on_effect_func_t react>
    void system::register_function(const std::vector<std::string> &args_name, const std::vector<std::any> &args_default_val) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      using output_type = final_output_type<F>;
      constexpr size_t script_type = get_script_type<output_type>();
      using register_type = void; // это тип по которому зарегистрируем функцию

      //if constexpr (func_arg_count == 0) { register_function<F, f, name, react>();  return; }

      if (args_name.size() < func_arg_count)
        throw std::runtime_error("Function expects " + std::to_string(func_arg_count) + " arguments, "
                                  "but receive only " + std::to_string(args_name.size()) + " arguments names");

      init_func_data ifd{
        script_type, SCRIPT_SYSTEM_ANY_TYPE, type_id<output_type>(), func_arg_count,
        [args_name, args_default_val] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t func_arg_count = get_function_argument_count<F>();
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          change_current_function_name ccfn(ctx, name);
          change_compare_op cco(ctx, compare_operators::more_eq);

          if constexpr (func_arg_count == 0) {
            using function_type = basic_function_no_arg<F, f, name, react>;
            // создадим сравнение?
            ctx->add_function<function_type>();
            interface* ret = nullptr;
            if (cont != nullptr) ret = cont->add<function_type>();
            if (obj.valid()) {
              if constexpr (std::is_same_v<output_type, bool>) {
                auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
                return sys->make_raw_boolean_compare(ctx, ret, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, double>) {
                change_compare_op cco(ctx, compare_operators::more_eq);
                auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
                return sys->make_raw_number_compare(ctx, ret, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
              else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
            }
            return ret;
          } else {
            using function_type = basic_function_scripted_args<F, f, name, react>;
            const size_t diff = func_arg_count > args_default_val.size() ? func_arg_count - args_default_val.size() : 0;
            if (diff > 1 && obj.get_type() != sol::type::table) throw std::runtime_error("Function " + std::string(name) + " expects table with arguments");

            size_t offset = SIZE_MAX;
            if (cont != nullptr) offset = cont->add_delayed<function_type>();
            ctx->add_function<function_type>();

            auto args = sys->make_arguments<F, func_arg_count, 0, true>(name, args_name, args_default_val, ctx, obj, cont);

            interface* cur = nullptr;
            if (cont != nullptr) {
              auto init = cont->get_init<function_type>(offset);
              cur = init.init(args);
            }

            return cur;
          }
        }, [] () {
          if constexpr (func_arg_count == 0) return std::string(name) + " I: " + std::string(type_name<void>()) + " O: " + std::string(type_name<output_type>());
          else return std::string(name) + " I: " + std::string(type_name<void>()) +
                                          " O: " + std::string(type_name<output_type>()) +
                                          " Args: " + make_function_args_string<F, func_arg_count, 0>();
        }
      };

      register_function<register_type, name>(std::move(ifd));
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    void system::register_function(const std::vector<std::any> &args_default_val) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      using output_type = final_output_type<F>;
      constexpr size_t script_type = get_script_type<output_type>();
      using register_type = void; // это тип по которому зарегистрируем функцию

      //if constexpr (func_arg_count < 2) { register_function<F, f, name, react>();  return; }

      init_func_data ifd{
        script_type, SCRIPT_SYSTEM_ANY_TYPE, type_id<output_type>(), func_arg_count,
        [args_default_val] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t func_arg_count = get_function_argument_count<F>();
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          change_current_function_name ccfn(ctx, name);
          change_compare_op cco(ctx, compare_operators::more_eq);

          if constexpr (func_arg_count == 0) {
            using function_type = basic_function_no_arg<F, f, name, react>;
            // создадим сравнение?
            ctx->add_function<function_type>();
            interface* ret = nullptr;
            if (cont != nullptr) ret = cont->add<function_type>();
            if (obj.valid()) {
              if constexpr (std::is_same_v<output_type, bool>) {
                auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
                return sys->make_raw_boolean_compare(ctx, ret, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, double>) {
                change_compare_op cco(ctx, compare_operators::more_eq);
                auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
                return sys->make_raw_number_compare(ctx, ret, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
              else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
            }
            return ret;
          } else {
            using function_type = basic_function_scripted_args<F, f, name, react>;
            const size_t diff = func_arg_count > args_default_val.size() ? func_arg_count - args_default_val.size() : 0;
            if (diff > 1 && obj.get_type() != sol::type::table) throw std::runtime_error("Function " + std::string(name) + " expects table with arguments");

            size_t offset = SIZE_MAX;
            if (cont != nullptr) offset = cont->add_delayed<function_type>();
            ctx->add_function<function_type>();

            auto args = sys->make_arguments<F, func_arg_count, 0, true>(name, args_default_val, ctx, obj, cont);

            interface* cur = nullptr;
            if (cont != nullptr) {
              auto init = cont->get_init<function_type>(offset);
              cur = init.init(args);
            }

            return cur;
          }
        }, [] () {
          if constexpr (func_arg_count == 0) return std::string(name) + " I: " + std::string(type_name<void>()) + " O: " + std::string(type_name<output_type>());
          else return std::string(name) + " I: " + std::string(type_name<void>()) +
                                          " O: " + std::string(type_name<output_type>()) +
                                          " Args: " + make_function_args_string<F, func_arg_count, 0>();
        }
      };

      register_function<register_type, name>(std::move(ifd));
    }

    template <typename F, F f, const char* name, on_effect_func_t react, const char* first_arg, const char*... args_names>
    void system::register_function(const std::vector<std::any> &args_default_val) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      using output_type = final_output_type<F>;
      constexpr size_t script_type = get_script_type<output_type>();
      using register_type = void; // это тип по которому зарегистрируем функцию
      constexpr size_t names_count = sizeof...(args_names) + 1;
      static_assert(func_arg_count == names_count, "Arguments names count must be equal function arguments count");

      if constexpr (func_arg_count == 0) { register_function<F, f, name, react>();  return; }

      init_func_data ifd{
        script_type, SCRIPT_SYSTEM_ANY_TYPE, type_id<output_type>(), func_arg_count,
        [args_default_val] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t func_arg_count = get_function_argument_count<F>();
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          change_current_function_name ccfn(ctx, name);
          change_compare_op cco(ctx, compare_operators::more_eq);

          if constexpr (func_arg_count == 0) {
            using function_type = basic_function_no_arg<F, f, name, react>;
            // создадим сравнение?
            ctx->add_function<function_type>();
            interface* ret = nullptr;
            if (cont != nullptr) ret = cont->add<function_type>();
            if (obj.valid()) {
              if constexpr (std::is_same_v<output_type, bool>) {
                auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
                return sys->make_raw_boolean_compare(ctx, ret, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, double>) {
                change_compare_op cco(ctx, compare_operators::more_eq);
                auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
                return sys->make_raw_number_compare(ctx, ret, rvalue, cont);
              } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
              else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
            }
            return ret;
          } else {
            using function_type = basic_function_scripted_named_args<F, f, name, react, first_arg, args_names...>;
            const size_t diff = func_arg_count > args_default_val.size() ? func_arg_count - args_default_val.size() : 0;
            if (diff > 1 && obj.get_type() != sol::type::table) throw std::runtime_error("Function " + std::string(name) + " expects table with arguments");

            size_t offset = SIZE_MAX;
            if (cont != nullptr) offset = cont->add_delayed<function_type>();
            ctx->add_function<function_type>();

            auto args = sys->make_arguments<F, func_arg_count, 0, true, first_arg, args_names...>(name, args_default_val, ctx, obj, cont);

            interface* cur = nullptr;
            if (cont != nullptr) {
              auto init = cont->get_init<function_type>(offset);
              cur = init.init(args);
            }

            return cur;
          }
        }, [] () {
          if constexpr (func_arg_count == 0) return std::string(name) + " I: " + std::string(type_name<void>()) + " O: " + std::string(type_name<output_type>());
          else return std::string(name) + " I: " + std::string(type_name<void>()) +
                                          " O: " + std::string(type_name<output_type>()) +
                                          " Args: " + make_function_args_string<F, func_arg_count, 0, first_arg, args_names...>();
        }
      };

      register_function<register_type, name>(std::move(ifd));
    }

    // тут может добавиться некий аргумент (константный? скорее всего)
    // который мы передадим в специальную функцию
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t head, int64_t... args>
    void system::register_function() {
      using output_type = final_output_type<F>;
      constexpr size_t script_type = get_script_type<output_type>();
      constexpr size_t arg_count = 0; // такие функции считаются за 0 аргументов

      init_func_data ifd{
        script_type, type_id<Th>(), type_id<output_type>(), arg_count,
        [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          using function_type = scripted_function_const_args<Th, F, f, name, react, head, args...>;

          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);

          ctx->add_function<function_type>();
          interface* cur = nullptr;
          if (cont != nullptr) cur = cont->add<function_type>();
          if (obj.valid()) {
            if constexpr (std::is_same_v<output_type, bool>) {
              auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
              return sys->make_raw_boolean_compare(ctx, cur, rvalue, cont);
            } else if constexpr (std::is_same_v<output_type, double>) {
              change_compare_op cco(ctx, compare_operators::more_eq);
              auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
              return sys->make_raw_number_compare(ctx, cur, rvalue, cont);
            } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
            else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
          }

          return cur;
        }, [] () -> std::string {
          return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                     " O: " + std::string(type_name<output_type>()) +
                                     " CArgs: " + make_function_const_args_string<F, head, args...>();
        }
      };

      register_function<Th, name>(std::move(ifd));
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    void system::register_function(Args&&... args) {
      static_assert(std::conjunction_v<std::is_copy_assignable<Args>...>);
      using output_type = final_output_type<F>;
      constexpr size_t script_type = get_script_type<output_type>();
      constexpr size_t arg_count = 0; // такие функции считаются за 0 аргументов

      init_func_data ifd{
        script_type, type_id<Th>(), type_id<output_type>(), arg_count,
        [func_args = std::make_tuple(std::forward<Args>(args) ...)] (system* sys, init_context* ctx, const sol::object &obj, container* cont) {
          using function_type = scripted_function_args<Th, F, f, name, react, Args...>;

          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);

          ctx->add_function<function_type>();
          interface* cur = nullptr;
          if (cont != nullptr) cur = cont->add<function_type>(func_args);
          if (obj.valid()) {
            if constexpr (std::is_same_v<output_type, bool>) {
              auto rvalue = sys->make_raw_script_boolean(ctx, obj, cont);
              return sys->make_raw_boolean_compare(ctx, cur, rvalue, cont);
            } else if constexpr (std::is_same_v<output_type, double>) {
              change_compare_op cco(ctx, compare_operators::more_eq);
              auto rvalue = sys->make_raw_script_number(ctx, obj, cont);
              return sys->make_raw_number_compare(ctx, cur, rvalue, cont);
            } else if constexpr (std::is_same_v<output_type, void>) throw std::runtime_error("Could not compare effect function '" + std::string(name) + "'");
            else throw std::runtime_error("Found compare pattern but return type '" + std::string(type_name<output_type>()) + "' is not supported, for objects and strings use function 'equals_to'");
          }

          return cur;
        }, [] () {
          constexpr bool is_independent_func = !is_member_v<Th, F>;
          return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                     " O: " + std::string(type_name<output_type>()) +
                                     " CArgs: " + make_function_args_string<F, get_function_argument_count<F>(), is_independent_func>();
        }
      };

      register_function<Th, name>(std::move(ifd));
    }

    template <typename T, typename F, typename R, const char* name>
    void system::register_function() {
      using output_type = R;
      using input_type = T;
      constexpr size_t script_type = get_script_type<output_type>();
      //using register_type = void; // это тип по которому зарегистрируем функцию

      init_func_data ifd{
        script_type, type_id<input_type>(), type_id<output_type>(), SCRIPT_SYSTEM_UNDEFINED_ARGS_COUNT,
        [] (system*, init_context* ctx, const sol::object &, container* cont) -> interface* {
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          change_current_function_name ccfn(ctx, name);
          using function_type = F;

          if (cont != nullptr) return cont->add<function_type>();
          return nullptr;
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<input_type>()) +
                                     " O: " + std::string(type_name<output_type>()) +
                                     " F: " + std::string(type_name<F>());
        }
      };

      register_function<input_type, name>(std::move(ifd));
    }

    template <typename T, typename F, typename R, const char* name> // тут по идее мы свой тип должны создать
    void system::register_function(const std::function<interface*(system::view, const sol::object &, container::delayed_initialization<F>)> &user_func) {
      static_assert(std::is_base_of_v<interface, F>);
      using output_type = R;
      using input_type = T;
      constexpr size_t script_type = get_script_type<output_type>();
      constexpr bool no_iterator = false;

      init_func_data ifd{
        script_type, type_id<T>(), type_id<output_type>(), SCRIPT_SYSTEM_UNDEFINED_ARGS_COUNT,
        [user_func] (system* sys, init_context* ctx, const sol::object &obj, container* cont) {
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          ctx->add_function<F>();
          container::delayed_initialization<F> init(nullptr);
          if (cont != nullptr) {
            const size_t offset = cont->add_delayed<F>();
            init = cont->get_init<F>(offset);
          }
          // тут у нас тоже может быть вполне блок с итератором, что если его не будет?
          // функция траверс сломается, надо бы соединить следующую и эту функции
          // а что если мы укажем дополнительную блочную функцию? она вызывается только при смене контекста
          // что должно происходить только в других блочных функциях, блочные функции зависят от приенения *_raw_* функций
          //change_block_function cbf(ctx, sys->get_init_function<T>(name));
          change_current_function_name ccfn(ctx, name);
          change_compare_op cco(ctx, compare_operators::more_eq);
          system::view v(sys, ctx, cont, no_iterator);
          return user_func(v, obj, std::move(init));
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<input_type>()) +
                                     " O: " + std::string(type_name<output_type>()) +
                                     " F: " + std::string(type_name<F>());
        }
      };

      register_function<input_type, name>(std::move(ifd));
    }

    // у меня строгое условие что мы возвращаем тот же тип что и принимаем? не могу придумать когда может быть иначе
    // случайный тип может приходить, но это крайне не желательно, тут в любом случае нужно указать возвращаемое значение
    template <typename T, typename F, F f, const char* name>
    void system::register_iterator() {
      //using ret_t = std::conditional_t< std::is_same_v< final_arg_type<F, 0>, T>, final_arg_type<F, 1>, final_arg_type<F, 0>>;
      using ret_t = std::tuple_element_t<0, final_output_type<F>>;
      static_assert(is_valid_type_for_object_v<ret_t>, "Return type must be suitable for script::object");
      // может быть если возвращаем void*, то тогда ретюрн тайп void (то есть это итератор эффект)?
      static_assert(
        std::is_invocable_v<F, T, ret_t, object, size_t> || std::is_invocable_v<F, ret_t, object, size_t>,
        "Function must be invocable with arguments: current script object (optional), accumulated return value and next script object. "
      );
      static_assert(
        std::is_same_v< std::conditional_t<
        std::is_invocable_v <F, T, ret_t, object, size_t>,
        std::invoke_result_t<F, T, ret_t, object, size_t>,
        std::invoke_result_t<F,    ret_t, object, size_t> >, std::tuple<ret_t, iterator_continue_status> >,
        "Function must return new current object and enum wich indicates early exit from iterator"
      );

      using output_type = ret_t;
      constexpr size_t script_type = get_script_type<output_type>();

      init_func_data ifd{
        script_type, type_id<T>(), type_id<output_type>(), SCRIPT_SYSTEM_FUNCTION_ITERATOR,
        [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          using function_type = scripted_iterator<T, F, f, name>;

          const auto sol_type = obj.get_type();
          if (sol_type != sol::type::table) throw std::runtime_error("iterator function " + std::string(name) + " expected table as input");

          ctx->add_function<function_type>();
          container::delayed_initialization<function_type> init(nullptr);
          if (cont != nullptr) {
            const size_t offset = cont->add_delayed<function_type>();
            init = cont->get_init<function_type>(offset);
          }

          // указываем текущую блочную функцию (возможно вообще имеет смысл это дело както отдельно от всего сделать)
          // блочная функция имеет смысл только для арифметических и логических итераторов (и то возможно не для всех)
          change_block_function cbf(ctx, sys->get_init_function<T>(name));
          change_block_name cbn(ctx, name);
          change_current_function_name ccfn(ctx, name);
          auto childs = sys->table_traverse(ctx, obj, cont);
          return init.init(childs);
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<T>()) +
                                     " O: " + std::string(type_name<output_type>()) +
                                     " F: " + std::string(type_name<F>());
        }
      };

      register_function<T, name>(std::move(ifd));
    }

    template <typename T, typename F, typename R, const char* name>
    void system::register_iterator() {
      static_assert(std::is_base_of_v<interface, F>);
      using output_type = R;
      constexpr size_t script_type = get_script_type<output_type>();

      init_func_data ifd{
        script_type, type_id<T>(), type_id<output_type>(), SCRIPT_SYSTEM_FUNCTION_ITERATOR,
        [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);
          using function_type = F;

          const auto sol_type = obj.get_type();
          if (sol_type != sol::type::table) throw std::runtime_error("iterator function " + std::string(name) + " expected table as input");

          ctx->add_function<function_type>();
          container::delayed_initialization<function_type> init(nullptr);
          if (cont != nullptr) {
            const size_t offset = cont->add_delayed<function_type>();
            init = cont->get_init<function_type>(offset);
          }

          // указываем текущую блочную функцию (возможно вообще имеет смысл это дело както отдельно от всего сделать)
          // блочная функция имеет смысл только для арифметических и логических итераторов (и то возможно не для всех)
          change_block_function cbf(ctx, sys->get_init_function<T>(name));
          change_block_name cbn(ctx, name);
          change_current_function_name ccfn(ctx, name);
          auto childs = sys->table_traverse(ctx, obj, cont);
          return init.init(childs);
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<T>()) +
                                     " O: " + std::string(type_name<output_type>()) +
                                     " F: " + std::string(type_name<F>());
        }
      };

      register_function<T, name>(std::move(ifd));
    }

    template <typename T, typename F, typename R, const char* name>
    void system::register_iterator(const std::function<interface*(system::view, const sol::object &, container::delayed_initialization<F>)> &user_func) {
      static_assert(std::is_base_of_v<interface, F>);
      using output_type = R;
      constexpr size_t script_type = get_script_type<output_type>();
      constexpr bool iterator = true;

      init_func_data ifd{
        script_type, type_id<T>(), type_id<output_type>(), SCRIPT_SYSTEM_FUNCTION_ITERATOR,
        [user_func] (system* sys, init_context* ctx, const sol::object &obj, container* cont) -> interface* {
          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);

          using function_type = F;
          ctx->add_function<function_type>();
          container::delayed_initialization<function_type> init(nullptr);
          if (cont != nullptr) {
            const size_t offset = cont->add_delayed<function_type>();
            init = cont->get_init<function_type>(offset);
          }
//           change_block_function cbf(ctx, sys->get_init_function<T>(name));
//           change_block_name cbn(ctx, name);
          change_current_function_name ccfn(ctx, name);
          change_compare_op cco(ctx, ctx->compare_operator);
          system::view v(sys, ctx, cont, iterator);
          return user_func(v, obj, std::move(init));
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<T>()) +
                                     " O: " + std::string(type_name<output_type>()) +
                                     " F: " + std::string(type_name<F>());
        }
      };

      register_function<T, name>(std::move(ifd));
    }

#define EVERY_FUNC_INIT                              \
  ctx->add_function<function_type>();                \
  container::delayed_initialization<function_type> init(nullptr); \
  if (cont != nullptr) {                             \
    const size_t offset = cont->add_delayed<function_type>(); \
    init = cont->get_init<function_type>(offset);    \
  }                                                  \
  interface* cond = nullptr;                         \
  const sol::table t = obj.as<sol::table>();         \
  if (const auto proxy = t["condition"]; proxy.valid()) { \
    cond = sys->make_raw_script_boolean(ctx, proxy, cont); \
  }                                                  \
  auto childs = sys->table_traverse(ctx, obj, cont); \
  if (init.valid()) final_int = init.init(cond, childs); \

    // какое название?
    template <typename Th, typename F, F f, const char* name>
    void system::register_every() {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      static_assert(std::is_same_v<final_output_type<predicate_type>, bool>, "Predicate function would return bool");
      static_assert(get_function_argument_count<predicate_type>() == 1, "Predicate function does support only one argument");
      using predicate_arg_type = final_arg_type<predicate_type, 0>;
      static_assert(is_valid_type_for_object_v<predicate_arg_type>, "Argument for predicate function must be valid for script::object");
      // необязательно
      //static_assert(function_class_e<predicate_type> == function_class::std_function, "Predicate must be std function");
      using first_arg = typename std::conditional<get_function_argument_count<F>() == 1, void, final_arg_type<F, 0>>::type;

      //using func_t = std::function<bool(const object &obj)>;

      static_assert(std::is_invocable_v<F, Th, predicate_type> || std::is_invocable_v<F, predicate_type>,
                    "Function must be invocable with arguments: script object (optional) and predicate function. "
                    "The function must enterupt execution when predicate function returns 'false'");

      if constexpr (!std::is_void_v<first_arg>) {
        if constexpr (std::is_pointer_v<Th>) {
          using non_const_ptr = std::remove_cv_t< std::remove_reference_t < std::remove_pointer_t<first_arg> > > *;
          static_assert(std::is_same_v<first_arg, Th> || std::is_same_v<non_const_ptr, Th>, "Function must accept script object for the first argument");
        } else {
          static_assert(std::is_same_v<first_arg, Th>, "Function must accept script object for the first argument");
        }
      }

      using output_type = object;
      constexpr size_t script_type = get_script_type<output_type>();
      static_assert(script_type != script_types::object && script_type != script_types::string);

      init_func_data ifd{
        script_type, type_id<Th>(), SCRIPT_SYSTEM_EVERY_RETURN_TYPE, SCRIPT_SYSTEM_FUNCTION_ITERATOR,
        [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) {
//           using function_type = typename std::conditional<
//             script_type == script_types::effect,
//             scripted_iterator_every_effect<T, Th, F, f, name>,
//             typename std::conditional<
//               script_type == script_types::condition,
//               scripted_iterator_every_logic<T, Th, F, f, name>,
//               scripted_iterator_every_numeric<T, Th, F, f, name>
//             >::type
//           >::type;
//
//           constexpr size_t script_type = get_script_type<output_type>();
//           check_script_types(name, ctx->script_type, script_type);

          const auto sol_type = obj.get_type();
          if (sol_type != sol::type::table) throw std::runtime_error("Function '" + std::string(name) + "' expected table as input");
          const sol::table t = obj.as<sol::table>();

//           change_block_function cbf(ctx, sys->get_init_function<T>(name));
//           change_block_name cbn(ctx, name);
          change_current_function_name ccfn(ctx, name);

          interface* final_int = nullptr;
          if (ctx->script_type == script_types::effect) {
            using function_type = scripted_iterator_every_effect<Th, F, f, name>;

            EVERY_FUNC_INIT

          } else if (ctx->script_type == script_types::condition) {
            using function_type = scripted_iterator_every_logic<Th, F, f, name>;

            EVERY_FUNC_INIT

          } else if (ctx->script_type == script_types::numeric) {
            using function_type = scripted_iterator_every_numeric<Th, F, f, name>;

            EVERY_FUNC_INIT

          } else throw std::runtime_error("Cannot use '" + std::string(name) + "' in string or object script");

          return final_int;
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                     " O: bool, double or nothing ";
        }
      };

      using final_Th = typename std::conditional<std::is_same_v<clear_type_t<Th>, void>, void, Th>::type;
      register_function<final_Th, name>(std::move(ifd));
    }

#undef EVERY_FUNC_INIT

    template <typename Th, typename F, F f, const char* name>
    void system::register_has() {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      static_assert(std::is_same_v<final_output_type<predicate_type>, bool>, "Predicate function would return bool");
      static_assert(get_function_argument_count<predicate_type>() == 1, "Predicate function does support only one argument");
      using predicate_arg_type = final_arg_type<predicate_type, 0>;
      static_assert(is_valid_type_for_object_v<predicate_arg_type>, "Argument for predicate function must be valid for script::object");
      // необязательно
      //static_assert(function_class_e<predicate_type> == function_class::std_function, "Predicate must be std function");
      using first_arg = typename std::conditional<get_function_argument_count<F>() == 1, void, final_arg_type<F, 0>>::type;

      //using func_t = std::function<bool(const object &obj)>;

      static_assert(std::is_invocable_v<F, Th, predicate_type> || std::is_invocable_v<F, predicate_type>,
                    "Function must be invocable with arguments: script object (optional) and predicate function. "
                    "The function must enterupt execution when predicate function returns 'false'");

      if constexpr (!std::is_void_v<first_arg>) {
        if constexpr (std::is_pointer_v<Th>) {
          using non_const_ptr = std::remove_cv_t< std::remove_reference_t < std::remove_pointer_t<first_arg> > > *;
          static_assert(std::is_same_v<first_arg, Th> || std::is_same_v<non_const_ptr, Th>, "Function must accept script object for the first argument");
        } else {
          static_assert(std::is_same_v<first_arg, Th>, "Function must accept script object for the first argument");
        }
      }

      using output_type = double;
      constexpr size_t script_type = get_script_type<output_type>();

      init_func_data ifd{
        script_type, type_id<Th>(), type_id<output_type>(), SCRIPT_SYSTEM_FUNCTION_ITERATOR,
        [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) {
          using function_type = scripted_iterator_has<Th, F, f, name>;

          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);

          const auto sol_type = obj.get_type();
          if (sol_type != sol::type::table) throw std::runtime_error("Function '" + std::string(name) + "' expected table as input");

          ctx->add_function<function_type>();
          container::delayed_initialization<function_type> init(nullptr);
          if (cont != nullptr) {
            const size_t offset = cont->add_delayed<function_type>();
            init = cont->get_init<function_type>(offset);
          }

//           change_block_function cbf(ctx, sys->get_init_function<T>(name));
//           change_block_name cbn(ctx, name);
          change_current_function_name ccfn(ctx, name);

          interface* percentage = nullptr;
          interface* max_count = nullptr;
          const sol::table t = obj.as<sol::table>();
          if (const auto proxy = t["percentage"]; proxy.valid()) {
            percentage = sys->make_raw_script_number(ctx, proxy, cont);
          } else if (const auto proxy = t["max_count"]; proxy.valid()) {
            max_count = sys->make_raw_script_number(ctx, proxy, cont);
          }

          auto childs = sys->table_traverse(ctx, obj, cont);

          interface* cur = nullptr;
          if (init.valid()) cur = init.init(max_count, percentage, childs);
          return cur;
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                     " O: " + std::string(type_name<output_type>());
        }
      };

      using final_Th = typename std::conditional<std::is_same_v<clear_type_t<Th>, void>, void, Th>::type;
      register_function<final_Th, name>(std::move(ifd));
    }

    template <typename Th, typename F, F f, const char* name>
    void system::register_random() {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      static_assert(std::is_same_v<final_output_type<predicate_type>, bool>, "Predicate function would return bool");
      static_assert(get_function_argument_count<predicate_type>() == 1, "Predicate function does support only one argument");
      using predicate_arg_type = final_arg_type<predicate_type, 0>;
      static_assert(is_valid_type_for_object_v<predicate_arg_type>, "Argument for predicate function must be valid for script::object");
      // необязательно
      //static_assert(function_class_e<predicate_type> == function_class::std_function, "Predicate must be std function");
      using first_arg = typename std::conditional<get_function_argument_count<F>() == 1, void, final_arg_type<F, 0>>::type;

      //using func_t = std::function<bool(const object &obj)>;

      static_assert(std::is_invocable_v<F, Th, predicate_type> || std::is_invocable_v<F, predicate_type>,
                    "Function must be invocable with arguments: script object (optional) and predicate function. "
                    "The function must enterupt execution when predicate function returns 'false'");

      if constexpr (!std::is_void_v<first_arg>) {
        if constexpr (std::is_pointer_v<Th>) {
          using non_const_ptr = std::remove_cv_t< std::remove_reference_t < std::remove_pointer_t<first_arg> > > *;
          static_assert(std::is_same_v<first_arg, Th> || std::is_same_v<non_const_ptr, Th>, "Function must accept script object for the first argument");
        } else {
          static_assert(std::is_same_v<first_arg, Th>, "Function must accept script object for the first argument");
        }
      }

      using output_type = object;
      constexpr size_t script_type = get_script_type<output_type>();
      static_assert(script_type != script_types::object && script_type != script_types::string);

      init_func_data ifd{
        script_type, type_id<Th>(), type_id<output_type>(), SCRIPT_SYSTEM_FUNCTION_ITERATOR,
        [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) {
          using function_type = scripted_iterator_random<Th, F, f, name>;

          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);

          const auto sol_type = obj.get_type();
          if (sol_type != sol::type::table) throw std::runtime_error("Function '" + std::string(name) + "' expected table as input");

          ctx->add_function<function_type>();
          container::delayed_initialization<function_type> init(nullptr);
          if (cont != nullptr) {
            const size_t offset = cont->add_delayed<function_type>();
            init = cont->get_init<function_type>(offset);
          }

//           change_block_function cbf(ctx, sys->get_init_function<T>(name));
//           change_block_name cbn(ctx, name);
          change_current_function_name ccfn(ctx, name);

          interface* condition = nullptr;
          interface* weight = nullptr;
          sol::table t = obj.as<sol::table>();
          if (const auto proxy = t["condition"]; proxy.valid()) {
            condition = sys->make_raw_script_boolean(ctx, proxy, cont);
          }

          if (const auto proxy = t["weight"]; proxy.valid()) {
            weight = sys->make_raw_script_number(ctx, proxy, cont);
          }

          // костыль, он вообще будет работать?
          const sol::object cond_obj = t["condition"];
          t["condition"] = sol::nil;

          //auto childs = sys->table_traverse(ctx, obj, cont);
          // проблема в том что мне нужно отключить в методах проверку condition, иначе он еще раз проверит
          // возможно имеет смысл просто удолить этот кондишен, можно использовать другую функцию
          auto childs = sys->make_raw_script_any(ctx, obj, cont); // возвращает одного ребенка

          t["condition"] = cond_obj;

          const size_t state = sys->get_next_random_state();
          interface* cur = nullptr;
          if (init.valid()) cur = init.init(state, condition, weight, childs);
          return cur;
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                     " O: " + std::string(type_name<output_type>());
        }
      };

      using final_Th = typename std::conditional<std::is_same_v<clear_type_t<Th>, void>, void, Th>::type;
      register_function<final_Th, name>(std::move(ifd));
    }

    template <typename Th, typename F, F f, const char* name>
    void system::register_view() {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      static_assert(std::is_same_v<final_output_type<predicate_type>, bool>, "Predicate function would return bool");
      static_assert(get_function_argument_count<predicate_type>() == 1, "Predicate function does support only one argument");
      using predicate_arg_type = final_arg_type<predicate_type, 0>;
      static_assert(is_valid_type_for_object_v<predicate_arg_type>, "Argument for predicate function must be valid for script::object");
      // необязательно
      //static_assert(function_class_e<predicate_type> == function_class::std_function, "Predicate must be std function");
      using first_arg = typename std::conditional<get_function_argument_count<F>() == 1, void, final_arg_type<F, 0>>::type;

      //using func_t = std::function<bool(const object &obj)>;

      static_assert(std::is_invocable_v<F, Th, predicate_type> || std::is_invocable_v<F, predicate_type>,
                    "Function must be invocable with arguments: script object (optional) and predicate function. "
                    "The function must enterupt execution when predicate function returns 'false'");

      if constexpr (!std::is_void_v<first_arg>) {
        if constexpr (std::is_pointer_v<Th>) {
          using non_const_ptr = std::remove_cv_t< std::remove_reference_t < std::remove_pointer_t<first_arg> > > *;
          static_assert(std::is_same_v<first_arg, Th> || std::is_same_v<non_const_ptr, Th>, "Function must accept script object for the first argument");
        } else {
          static_assert(std::is_same_v<first_arg, Th>, "Function must accept script object for the first argument");
        }
      }

      using output_type = object;
      constexpr size_t script_type = get_script_type<output_type>();
      static_assert(script_type != script_types::string);

      init_func_data ifd{
        script_type, type_id<Th>(), type_id<output_type>(), SCRIPT_SYSTEM_FUNCTION_ITERATOR,
        [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) {
          using function_type = scripted_iterator_view<Th, F, f, name>;

          constexpr size_t script_type = get_script_type<output_type>();
          check_script_types(name, ctx->script_type, script_type);

          const auto sol_type = obj.get_type();
          if (sol_type != sol::type::table) throw std::runtime_error("Function '" + std::string(name) + "' expected table as input");

          ctx->add_function<function_type>();
          container::delayed_initialization<function_type> init(nullptr);
          if (cont != nullptr) {
            const size_t offset = cont->add_delayed<function_type>();
            init = cont->get_init<function_type>(offset);
          }

          // что мы тут ожидаем? ожидаем таблицу с таблицами с одним элементом + наверное "reduce_value" для значения по умолчанию
          // может вообще везде сократить количество ошибок? все таки это не ФАТАЛ эррор, хотя с другой стороны
          // банальная ошибка которую мы можем не заметить
          change_current_function_name ccfn(ctx, name);
          interface* default_value = nullptr;
          interface* begin_childs = nullptr;
          interface* cur_child = nullptr;
          const auto t = obj.as<sol::table>();
          for (const auto &pair : t) {
            if (pair.first.get_type() == sol::type::string) {
              const auto str = obj.as<std::string_view>();
              if (str != "reduce_value") { throw std::runtime_error("Unexpected lvalue string '" + std::string(str) + "' in function '" + std::string(name) + "', expected only 'reduce_value'"); }
              change_expected_type cet(ctx, SCRIPT_SYSTEM_ANY_TYPE);
              default_value = sys->make_raw_script_object(ctx, pair.second, cont);
              continue;
            }

            if (pair.first.get_type() == sol::type::number) {
              if (pair.second.get_type() != sol::type::table)
                throw std::runtime_error("Unexpected rvalue of type '" + std::string(detail::get_sol_type_name(pair.second.get_type())) + "', function '" + std::string(name) + "' expected table with tables");

              // тут ожидаем строго слева название функции
              size_t counter = 0;
              const auto inner_t = pair.second.as<sol::table>();
              for (const auto &pair : inner_t) {
                if (ctx->script_type == script_types::effect) {
                  // если не строка то в эффекте ожидаем просто таблицу с перечислением действий вместо reduce
                }

                if (counter >= 1) throw std::runtime_error("Function '" + std::string(name) + "' expects inner table to consist of one function ");

                interface* local = nullptr;
                if (ctx->script_type != script_types::effect && pair.first.get_type() != sol::type::string)
                  throw std::runtime_error("Unexpected lvalue of type '" + std::string(detail::get_sol_type_name(pair.first.get_type())) + "' in function '" + std::string(name) + "', expected string ");

                if (ctx->script_type == script_types::effect && pair.first.get_type() != sol::type::string) {
                  // ождаем таблицу с эффектами
                  if (pair.second.get_type() != sol::type::table)
                    throw std::runtime_error("If current script type is effect script, then instead of function 'reduce' the table with effect functions is expected, context '" + std::string(name) + "'");

                  local = sys->make_raw_script_effect(ctx, pair.second, cont);
                } else {
                  const auto str = pair.first.as<std::string_view>();
                  if (const auto itr = detail::view_allowed_funcs.find(str); itr == detail::view_allowed_funcs.end())
                    throw std::runtime_error("Function '" + std::string(name) + "' expects only transform, filter, reduce, take and drop functions in inner table, got '" + std::string(str) + "'");

                  if (str == "reduce" && ctx->script_type == script_types::effect)
                    throw std::runtime_error("Function 'reduce' is not allowed in inner table in effect scripts, context '" + std::string(name) + "'");

                  const auto type_itr = sys->func_map.find(type_id<void>());
                  assert(type_itr != sys->func_map.end());

                  const auto itr = type_itr->second.find(str);
                  if (itr == type_itr->second.end()) throw std::runtime_error("Could not find function '" + std::string(str) + "', context '" + std::string(name) + "'");

                  local = itr->second.func(sys, ctx, pair.second, cont);
                }

                ++counter;
                if (begin_childs == nullptr) begin_childs = local;
                if (cur_child != nullptr) cur_child->next = local;
                cur_child = local;
              }
              continue;
            }

            throw std::runtime_error("Unexpected lvalue of type '" + std::string(detail::get_sol_type_name(pair.first.get_type())) + "', function '" + std::string(name) + "' expected table or 'reduce_value'");
          }

          return init.init(default_value, begin_childs);
        }, [] () {
          return std::string(name) + " I: " + std::string(type_name<Th>()) +
                                     " O: " + std::string(type_name<output_type>());
        }
      };

      using final_Th = typename std::conditional< std::is_same_v<clear_type_t<Th>, void>, void, Th >::type;
      register_function<final_Th, name>(std::move(ifd));
    }

    template <typename T>
    void system::register_enum(std::vector<std::pair<std::string, T>> enums) {
      const auto itr = enum_map.find(type_id<T>());
      if (itr != enum_map.end()) throw std::runtime_error("Enum type " + std::string(type_name<T>()) + " is already registered");

      auto cur = enum_map.emplace(type_id<T>(), phmap::flat_hash_map<std::string, int64_t>{}).first;
      for (auto &p : enums) {
        cur->second.emplace(p.first, static_cast<int64_t>(p.second));
      }
    }

    static void populate_data_locals(script_data* data, const system::init_context* ctx);

    // возвращать будем легкие обертки вокруг interface* (тип script::number и проч)
    // + нужно будет возвращать скрипты по названию, скрипты которые вставляются в другие скрипты вызываются иначе
    // + могут пригодится флаги
    template <typename T>
    condition system::create_condition(const sol::object &obj, const std::string_view &script_name) {
      return condition(create_script_raw(type_id<T>(), obj, script_name, &system::make_raw_script_boolean));
    }

    template <typename T>
    number system::create_number(const sol::object &obj, const std::string_view &script_name) {
      return number(create_script_raw(type_id<T>(), obj, script_name, &system::make_raw_script_number));
    }

    template <typename T>
    string system::create_string(const sol::object &obj, const std::string_view &script_name) {
      return string(create_script_raw(type_id<T>(), obj, script_name, &system::make_raw_script_string));
    }

    template <typename T>
    effect system::create_effect(const sol::object &obj, const std::string_view &script_name) {
      return effect(create_script_raw(type_id<T>(), obj, script_name, &system::make_raw_script_effect));
    }

    template <typename T>
    const system::init_func_data* system::get_init_function(const std::string_view &name) const {
      return get_init_function(type_id<T>(), name);
    }

    template <typename T, const char* name>
    void system::register_function(init_func_data&& data) {
      register_function(type_id<T>(), type_name<T>(), name, std::move(data));
    }
  }
#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
