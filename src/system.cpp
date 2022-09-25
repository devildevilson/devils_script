#include "system.h"

#include <cassert>
#include <charconv>

#include "common_commands.h"
#include "numeric_functions.h"
#include "logic_commands.h"
//#include "re2/re2.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    //static const std::string_view number_matcher = "[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)";
    // блен плохая идея для одного regex'а, может стд использовать?
    // наверное даже charconv справится
    //static const RE2 regex_number_matcher(number_matcher);

    static bool table_has_condition(const sol::object &obj) {
      if (const auto type = obj.get_type(); type != sol::type::table) return false;
      const auto table = obj.as<sol::table>();
      const auto proxy = table["condition"];
      return proxy.valid();
    }

    size_t get_script_type(const size_t &id) {
      if (id == type_id<void>()) { return script_types::effect; }
      else if (id == type_id<bool>()) { return script_types::condition; }
      else if (id == type_id<double>()) { return script_types::numeric; }
      else if (id == type_id<std::string_view>()) { return script_types::string; }
      else if (id == type_id<object>()) { return SIZE_MAX; }
      return script_types::object;
    }

    void check_script_types(const std::string_view &name, const size_t &input, const size_t &expected) {
      if (!compare_script_types(input, expected))
        throw std::runtime_error("Function " + std::string(name) + " is expected to be in " + std::string(script_types::names[expected]) + " script type, "
                                 "but input is " + std::string(script_types::names[input]) + " script type");
    }

    static bool is_complex_object(const std::string_view &lvalue) {
      return lvalue.find('.') != std::string_view::npos || lvalue.find(':') != std::string_view::npos;
    }

    void populate_data_locals(script_data* data, const system::init_context* ctx) {
      data->locals.resize(ctx->current_local_var_id);
      for (const auto &p : ctx->local_var_ids) {
        const auto [index, type] = p.second;
        assert(index < data->locals.size());
        data->locals[index] = p.first;
      }
    }

#define ANY_TYPE SCRIPT_SYSTEM_ANY_TYPE
    size_t system::init_context::save_local(const std::string &name, const size_t &type) {
      auto itr = local_var_ids.find(name);
      if (itr == local_var_ids.end()) {
        const size_t cur = current_local_var_id;
        ++current_local_var_id;
        local_var_ids.emplace(name, std::make_pair(cur, type));
        return cur;
      }

      if (itr->second.second == SIZE_MAX) {
        itr->second.second = type;
        return itr->second.first;
      }

      if (itr->second.second == ANY_TYPE && type == ANY_TYPE) return itr->second.first;

      if (itr->second.second != ANY_TYPE && type != ANY_TYPE && itr->second.second != type) throw std::runtime_error("Trying to save object in slot" + name + " with different type");

      if (itr->second.second == ANY_TYPE) {
        itr->second.second = type;
        return itr->second.first;
      }

      return itr->second.first;
    }

    std::tuple<size_t, size_t> system::init_context::get_local(const std::string_view &name) const {
      const auto itr = local_var_ids.find(name);
      if (itr == local_var_ids.end()) return std::make_tuple(SIZE_MAX, SIZE_MAX);
      return std::make_tuple(itr->second.first, itr->second.second);
    }

    size_t system::init_context::remove_local(const std::string_view &name) {
      auto itr = local_var_ids.find(name);
      if (itr == local_var_ids.end()) return SIZE_MAX;
      itr->second.second = SIZE_MAX;
      return itr->second.first;
    }

    void system::init_context::clear() {
      current_size = 0;
      current_count = 0;
      current_local_var_id = 0;
      local_var_ids.clear();
      lists.clear();
    }

    interface* system::view::make_scripted_conditional(const sol::object &obj) {
      return sys->make_raw_script_boolean(ctx, obj, cont);
    }

    interface* system::view::make_scripted_numeric(const sol::object &obj) {
      return sys->make_raw_script_number(ctx, obj, cont);
    }

    interface* system::view::make_scripted_string(const sol::object &obj) {
      return sys->make_raw_script_string(ctx, obj, cont);
    }

    interface* system::view::make_scripted_effect(const sol::object &obj) {
      return sys->make_raw_script_effect(ctx, obj, cont);
    }

    interface* system::view::make_scripted_object(const size_t &id, const sol::object &obj) {
      change_expected_type cet(ctx, id);
      return sys->make_raw_script_object(ctx, obj, cont);
    }

    interface* system::view::any_scripted_object(const sol::object &obj) {
      return sys->make_raw_script_any(ctx, obj, cont);
    }

    interface* system::view::traverse_children(const sol::object &obj) {
      if (!is_iterator) throw std::runtime_error("Table traverse can be done only in iterator functions");
      return sys->table_traverse(ctx, obj, cont);
    }

    interface* system::view::traverse_children_numeric(const sol::object &obj) {
      if (!is_iterator) throw std::runtime_error("Table traverse can be done only in iterator functions");
      change_script_type cst(ctx, script_types::numeric);
      return sys->numeric_table_traverse(ctx, obj, cont);
    }

    interface* system::view::traverse_children_condition(const sol::object &obj) {
      if (!is_iterator) throw std::runtime_error("Table traverse can be done only in iterator functions");
      change_script_type cst(ctx, script_types::condition);
      return sys->condition_table_traverse(ctx, obj, cont);
    }

    size_t system::view::get_random_state() {
      return sys->get_next_random_state();
    }

    size_t system::view::save_local(const std::string &name, const size_t &type) {
      return ctx->save_local(name, type);
    }

    std::tuple<size_t, size_t> system::view::get_local(const std::string_view &name) const {
      return ctx->get_local(name);
    }

    size_t system::view::remove_local(const std::string_view &name) {
      return ctx->remove_local(name);
    }

    void system::view::add_list(const std::string &name) {
      ctx->lists.emplace(name);
    }

    bool system::view::list_exists(const std::string_view &name) const {
      return ctx->lists.find(name) != ctx->lists.end();
    }

    std::tuple<std::string_view, const script_data*, size_t> system::view::find_script(const std::string_view name) {
      const auto itr = sys->scripts.find(name);
      if (itr == sys->scripts.end()) return std::make_tuple(std::string_view(), nullptr, SIZE_MAX);
      return std::make_tuple(std::string_view(itr->first), itr->second.second, itr->second.first);
    }

    void system::view::set_return_type(const size_t &type) {
      assert(ctx->computed_type == SCRIPT_SYSTEM_ANY_TYPE); // ???
      ctx->computed_type = type;
    }

    system::system(const size_t &seed) : random_state(SCRIPT_SYSTEM_DEFAULT_GENERATOR_NAMESPACE::init(seed)) {
      init();
    }

    system::~system() {
      for (const auto &pair : containers) {
        pair.first->begin->~interface();
        pair.second->destroy(pair.first);
        containers_pool.destroy(pair.second);
      }
    }

    void system::copy_init_funcs_to(system &another) {
      for (const auto &pair : func_map) {
        auto itr = another.func_map.find(pair.first);
        if (itr == another.func_map.end()) { itr = another.func_map.insert(std::make_pair(pair.first, pair.second)).first; continue; }

        for (const auto &func_pair : pair.second) {
          const auto func_itr = itr->second.find(func_pair.first);
          // мы тут будем вылетать даже когда передаем дефолтные функции, предполагается что другая система относительно пустая (только дефотные функции заданы)
          //if (func_itr != itr->second.end()) throw std::runtime_error("Function " + std::string(func_pair.first) + "is aready exists in another system");
          if (func_itr != itr->second.end()) continue;

          itr->second.insert(func_pair);
        }
      }
    }

    size_t system::get_next_random_state() {
      random_state = SCRIPT_SYSTEM_DEFAULT_GENERATOR_NAMESPACE::advance(random_state);
      return SCRIPT_SYSTEM_DEFAULT_GENERATOR_NAMESPACE::get_value(random_state);
    }

    void system::save_condition(const condition script, const std::string_view name) {
      if (name.empty()) throw std::runtime_error("Empty script name string");
      // почему нельзя? это скорее зависит от того как мы вызываем скрипт, там поди мы просто можем сделать как с листами
      // то есть название там будет писаться в специальной графе 'name'
      //if (is_complex_object(name)) throw std::runtime_error("Dots '.' and colons ':' is not allowed in script name");

      const auto itr = scripts.find(name);
      if (itr != scripts.end()) throw std::runtime_error("Script with name " + std::string(name) + " is already saved");
      scripts.emplace(std::make_pair(std::string(name), std::make_pair(type_id<bool>(), script.native())));
    }

    void system::save_number(const number script,       const std::string_view name) {
      if (name.empty()) throw std::runtime_error("Empty script name string");
      //if (is_complex_object(name)) throw std::runtime_error("Dots '.' and colons ':' is not allowed in script name");

      const auto itr = scripts.find(name);
      if (itr != scripts.end()) throw std::runtime_error("Script with name " + std::string(name) + " is already saved");
      scripts.emplace(std::make_pair(std::string(name), std::make_pair(type_id<double>(), script.native())));
    }

    void system::save_string(const string script,       const std::string_view name) {
      if (name.empty()) throw std::runtime_error("Empty script name string");
      //if (is_complex_object(name)) throw std::runtime_error("Dots '.' and colons ':' is not allowed in script name");

      const auto itr = scripts.find(name);
      if (itr != scripts.end()) throw std::runtime_error("Script with name " + std::string(name) + " is already saved");
      scripts.emplace(std::make_pair(std::string(name), std::make_pair(type_id<std::string_view>(), script.native())));
    }

    void system::save_effect(const effect script,       const std::string_view name) {
      if (name.empty()) throw std::runtime_error("Empty script name string");
      //if (is_complex_object(name)) throw std::runtime_error("Dots '.' and colons ':' is not allowed in script name");

      const auto itr = scripts.find(name);
      if (itr != scripts.end()) throw std::runtime_error("Script with name " + std::string(name) + " is already saved");
      scripts.emplace(std::make_pair(std::string(name), std::make_pair(type_id<void>(), script.native())));
    }

    condition system::get_condition(const std::string_view name) const {
      const auto itr = scripts.find(name);
      if (itr == scripts.end()) throw std::runtime_error("Could not find script " + std::string(name));
      if (type_id<bool>() != itr->second.first) throw std::runtime_error("Script " + std::string(name) + " is not condition script");
      return condition(itr->second.second);
    }

    number system::get_number(const std::string_view name) const {
      const auto itr = scripts.find(name);
      if (itr == scripts.end()) throw std::runtime_error("Could not find script " + std::string(name));
      if (type_id<double>() != itr->second.first) throw std::runtime_error("Script " + std::string(name) + " is not condition script");
      return number(itr->second.second);
    }

    string system::get_string(const std::string_view name) const {
      const auto itr = scripts.find(name);
      if (itr == scripts.end()) throw std::runtime_error("Could not find script " + std::string(name));
      if (type_id<std::string_view>() != itr->second.first) throw std::runtime_error("Script " + std::string(name) + " is not condition script");
      return string(itr->second.second);
    }

    effect system::get_effect(const std::string_view name) const {
      const auto itr = scripts.find(name);
      if (itr == scripts.end()) throw std::runtime_error("Could not find script " + std::string(name));
      if (type_id<void>() != itr->second.first) throw std::runtime_error("Script " + std::string(name) + " is not condition script");
      return effect(itr->second.second);
    }

    void system::clear_enums() {
      enum_map.clear();
      enum_map.reserve(0);
    }

    void system::clear_funcs_init() {
      func_map.clear();
      func_map.reserve(0);
    }

    std::string system::get_function_data_dump() const {
      static const std::pair<std::string_view, size_t> types[] = {
        std::make_pair("effect", type_id<void>()),
        std::make_pair("condition", type_id<bool>()),
        std::make_pair("numeric", type_id<double>()),
//         std::make_pair("scope", type_id<object>()),
      };

      std::string dump;
      dump.reserve(1024 * 10);
      for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
        const auto &p = types[i];
        dump.append(p.first);
        dump.append(":\n");
        for (const auto &t : func_map) {
          for (const auto &f : t.second) {
            if (f.second.return_type != p.second) continue;
            if (f.second.arguments_count == SCRIPT_SYSTEM_FUNCTION_ITERATOR) continue;
            const auto str = f.second.info();
            dump.append("  ");
            dump.append(str);
            dump.append("\n");
          }
        }
        dump.append("\n");
      }

      dump.append("scope:\n");
      for (const auto &t : func_map) {
        for (const auto &f : t.second) {
          if (f.second.return_type == type_id<void>() || f.second.return_type == type_id<bool>() || f.second.return_type == type_id<double>()) continue;
          if (f.second.arguments_count == SCRIPT_SYSTEM_FUNCTION_ITERATOR) continue;
          const auto str = f.second.info();
          dump.append("  ");
          dump.append(str);
          dump.append("\n");
        }
      }

      dump.append("\n");
      dump.append("iterator:\n");
      for (const auto &t : func_map) {
        for (const auto &f : t.second) {
          if (f.second.arguments_count != SCRIPT_SYSTEM_FUNCTION_ITERATOR) continue;
          const auto str = f.second.info();
          dump.append("  ");
          dump.append(str);
          dump.append("\n");
        }
      }

      return dump;
    }

    bool system::function_exists(const std::string_view &name, size_t* return_type, size_t* arg_count) {
      size_t current_type = SIZE_MAX;
      size_t current_args = SIZE_MAX;
      for (const auto &pair : func_map) {
        auto itr = pair.second.find(name);
        if (itr == pair.second.end()) continue;

        current_type = current_type == SIZE_MAX ? itr->second.return_type : current_type;
        current_args = current_args == SIZE_MAX ? itr->second.arguments_count : current_args;
        //current_type != SIZE_MAX &&      current_args != SIZE_MAX &&
        if (current_type != itr->second.return_type) throw std::runtime_error("Function " + std::string(name) + " has overload with different return type, it is not allowed");
        if (current_args != itr->second.arguments_count) throw std::runtime_error("Function " + std::string(name) + " has overload with different arguments count, it is not allowed");
      }

      if (return_type != nullptr && current_type != SIZE_MAX) *return_type = current_type;
      if (arg_count != nullptr && current_args != SIZE_MAX) *arg_count = current_args;
      return current_type != SIZE_MAX;
    }

    const system::init_func_data* system::get_init_function(const size_t &type_id, const std::string_view &name) const {
      auto itr = func_map.find(type_id);
      if (itr == func_map.end()) throw std::runtime_error("Type 123 is not registered");

      const std::string_view func_name = name;
      auto& map = itr->second;
      const auto func_itr = map.find(name);
      if (func_itr == map.end()) throw std::runtime_error(" Could not find init function " + std::string(func_name));

      return &func_itr->second;
    }

    static interface* create_boolean_container(system::init_context* ctx, const bool value, container* cont) {
      interface* cur = nullptr;
      if (cont != nullptr) cur = cont->add<boolean_container>(value);
      ctx->current_size += sizeof(boolean_container);
      return cur;
    }

    static interface* create_number_container(system::init_context* ctx, const double value, container* cont) {
      interface* cur = nullptr;
      if (cont != nullptr) cur = cont->add<number_container>(value);
      ctx->current_size += sizeof(number_container);
      return cur;
    }

    static interface* create_string_container(system::init_context* ctx, const std::string_view &value, container* cont) {
      interface* cur = nullptr;
      if (cont != nullptr) cur = cont->add<string_container>(value);
      ctx->current_size += sizeof(string_container);
      return cur;
    }

    interface* system::make_raw_script_boolean(init_context* ctx, const sol::object &obj, container* cont) {
      change_script_type cst(ctx, script_types::condition);
      const sol::object nil = sol::make_object(obj.lua_state(), sol::nil);

      interface* cur = nullptr;
      const auto sol_type = obj.get_type();
      switch (sol_type) {
        case sol::type::boolean: cur = create_boolean_container(ctx, obj.as<bool>(), cont); break;
        case sol::type::number : cur = create_number_container(ctx, obj.as<double>(), cont); break;
        case sol::type::string : {
          change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);
          cur = make_complex_object(ctx, obj.as<std::string_view>(), nil, cont);
          if (ctx->computed_type != SCRIPT_SYSTEM_ANY_TYPE && (ctx->computed_type != type_id<bool>() || ctx->computed_type != type_id<double>()))
            throw std::runtime_error("Complex lvalue " + std::string(obj.as<std::string_view>()) +
              " evaluates into object with unexpected type, context " + std::string(ctx->function_name));
          break;
        }
        case sol::type::table: {
          const auto init_func = get_init_function(type_id<void>(), "AND");
          cur = init_func->func(this, ctx, obj, cont);
          break;
        }

        default: throw std::runtime_error("Function " + std::string(ctx->function_name) + " expected boolean, number, string or table");
      }

      return cur;
    }

    interface* system::make_raw_script_number(init_context* ctx, const sol::object &obj, container* cont) {
      change_script_type cst(ctx, script_types::numeric);

      const sol::object nil = sol::make_object(obj.lua_state(), sol::nil);

      interface* cur = nullptr;
      const auto sol_type = obj.get_type();
      switch (sol_type) {
        case sol::type::boolean: cur = create_boolean_container(ctx, obj.as<bool>(), cont); break;
        case sol::type::number : cur = create_number_container(ctx, obj.as<double>(), cont); break;
        case sol::type::string : {
          change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);
          cur = make_complex_object(ctx, obj.as<std::string_view>(), nil, cont);
          if (ctx->computed_type != SCRIPT_SYSTEM_ANY_TYPE && (ctx->computed_type != type_id<bool>() || ctx->computed_type != type_id<double>()))
            throw std::runtime_error("Complex lvalue " + std::string(obj.as<std::string_view>()) +
              " evaluates into object with unexpected type, context " + std::string(ctx->function_name));
          break;
        }
        case sol::type::table: {
          //change_compare_op cco(ctx, ctx->compare_operator);
          const auto t = obj.as<sol::table>();
          if (const auto proxy = t["op"]; proxy.valid()) {
            const auto str = proxy.get<std::string_view>();
            const auto itr = compare_operators::map.find(str);
            if (itr == compare_operators::map.end()) throw std::runtime_error("Could not find compare operator " + std::string(str) + ", context " + std::string(ctx->function_name));
            ctx->compare_operator = itr->second;
          }
          const auto init_func = get_init_function(type_id<void>(), "add");
          cur = init_func->func(this, ctx, obj, cont);
          break;
        }

        default: throw std::runtime_error("Function " + std::string(ctx->function_name) + " expected boolean, number, string or table");
      }

      return cur;
    }

    interface* system::make_raw_script_string(init_context* ctx, const sol::object &obj, container* cont) {
      change_script_type cst(ctx, script_types::string);

      interface* cur = nullptr;
      const auto sol_type = obj.get_type();
      switch (sol_type) {
        case sol::type::string: cur = create_string_container(ctx, obj.as<std::string_view>(), cont); break;
        case sol::type::table: {
          // ожидаем вычисление строки, по идее достаточно пройтись и вернуть последнюю строку
          // смена контекста создаст контейнер если есть кондишен, сюда мы попадаем по ходу
          // только в самом начале, тожно создать контейнер не боясь
          const bool has_condition = table_has_condition(obj);
          if (has_condition) {
            cur = make_string_context(ctx, obj, cont);
            break;
          }

          size_t offset = SIZE_MAX;
          if (cont != nullptr) offset = cont->add_delayed<compute_string>();
          ctx->current_size += sizeof(compute_string);
          auto childs = string_table_traverse(ctx, obj, cont); // make_string_context ?
          if (cont != nullptr) {
            auto init = cont->get_init<compute_string>(offset);
            cur = init.init(nullptr, childs);
          }
          break;
        }

        default: throw std::runtime_error("Function " + std::string(ctx->function_name) + " expected string or table");
      }

      return cur;
    }

    interface* system::make_raw_script_effect(init_context* ctx, const sol::object &obj, container* cont) {
      change_script_type cst(ctx, script_types::effect);

      const sol::object nil = sol::make_object(obj.lua_state(), sol::nil);

      interface* cur = nullptr;
      const auto sol_type = obj.get_type();
      switch (sol_type) {
        case sol::type::string: {
          change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);
          cur = make_complex_object(ctx, obj.as<std::string_view>(), nil, cont);
          // тут наверное строгое соотвествие?
          if (ctx->computed_type != SCRIPT_SYSTEM_ANY_TYPE && ctx->computed_type != type_id<void>())
            throw std::runtime_error("Complex lvalue " + std::string(obj.as<std::string_view>()) +
              " evaluates into object with unexpected type, context " + std::string(ctx->function_name));
          break;
        }

        case sol::type::table: {
          const bool has_condition = table_has_condition(obj);
          if (has_condition) {
            cur = make_effect_context(ctx, "", obj, cont);
            break;
          }

          size_t offset = SIZE_MAX;
          if (cont != nullptr) offset = cont->add_delayed<change_scope_effect>();
          ctx->current_size += sizeof(change_scope_effect);
          auto childs = effect_table_traverse(ctx, obj, cont);
          if (cont != nullptr) {
            auto init = cont->get_init<change_scope_effect>(offset);
            cur = init.init(nullptr, nullptr, childs);
          }
          break;
        }

        default: throw std::runtime_error("Function " + std::string(ctx->function_name) + " expected string or table");
      }

      return cur;
    }

    interface* system::make_raw_script_object(init_context* ctx, const sol::object &obj, container* cont) {
      change_script_type cst(ctx, script_types::object);

      const sol::object nil = sol::make_object(obj.lua_state(), sol::nil);

      interface* cur = nullptr;
      const auto sol_type = obj.get_type();
      switch (sol_type) {
        case sol::type::string: {
          change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);
          cur = make_complex_object(ctx, obj.as<std::string_view>(), nil, cont); break;
          // наверн нужно проверить еще expected_type
          if (ctx->computed_type != SCRIPT_SYSTEM_ANY_TYPE &&
              ctx->expected_type != SCRIPT_SYSTEM_ANY_TYPE &&
              ctx->computed_type != ctx->expected_type)
            throw std::runtime_error("Complex lvalue " + std::string(obj.as<std::string_view>()) +
              " evaluates into object with unexpected type, context " + std::string(ctx->function_name));
          break;
        }

        case sol::type::table: {
          const bool has_condition = table_has_condition(obj);
          if (has_condition) {
            cur = make_object_context(ctx, obj, cont);
            break;
          }

          size_t offset = SIZE_MAX;
          if (cont != nullptr) offset = cont->add_delayed<compute_object>();
          ctx->current_size += sizeof(compute_object);
          auto childs = object_table_traverse(ctx, obj, cont); // make_object_context ?
          if (cont != nullptr) {
            auto init = cont->get_init<compute_object>(offset);
            cur = init.init(nullptr, childs);
          }
          break;
        }

        default: throw std::runtime_error("Function " + std::string(ctx->function_name) + " expected string or table");
      }

      return cur;
    }

    interface* system::make_raw_script_any(init_context* ctx, const sol::object &obj, container* cont) {
      interface* child = nullptr;
      switch (ctx->script_type) {
        case script_types::condition: child = make_raw_script_boolean(ctx, obj, cont); break;
        case script_types::numeric:   child = make_raw_script_number(ctx, obj, cont);  break;
        case script_types::string:    child = make_raw_script_string(ctx, obj, cont);  break;
        case script_types::effect:    child = make_raw_script_effect(ctx, obj, cont);  break;
        case script_types::object:    child = make_raw_script_object(ctx, obj, cont);  break;
        case SIZE_MAX:                child = make_raw_script_object(ctx, obj, cont);  break;
        default: throw std::runtime_error("Add new script type");
      }
      return child;
    }

    interface* system::make_raw_number_compare(init_context* ctx, const interface* lvalue, const interface* rvalue, container* cont) {
      const uint8_t compare_op = ctx->compare_operator;
      ctx->current_size += sizeof(number_comparator);
      if (cont != nullptr) return cont->add<number_comparator>(compare_op, lvalue, rvalue);
      return nullptr;
    }

    interface* system::make_raw_boolean_compare(init_context* ctx, const interface* lvalue, const interface* rvalue, container* cont) {
      ctx->current_size += sizeof(boolean_comparator);
      if (cont != nullptr) return cont->add<boolean_comparator>(lvalue, rvalue);
      return nullptr;
    }

    interface* system::make_number_compare(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont) {
      size_t offset = SIZE_MAX;
      if (cont != nullptr) offset = cont->add_delayed<number_comparator>();
      ctx->add_function<number_comparator>();

      auto l_part = create_overload(ctx, lvalue, sol::object(), cont);
      change_compare_op cco(ctx, compare_operators::more_eq);
      auto r_part = make_raw_script_number(ctx, rvalue, cont);
      const uint8_t compare_op = ctx->compare_operator;

      interface* ret = nullptr;
      if (cont != nullptr) {
        auto init = cont->get_init<number_comparator>(offset);
        ret = init.init(compare_op, l_part, r_part);
      }

      return ret;
    }

    interface* system::make_boolean_compare(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont) {
      size_t offset = SIZE_MAX;
      if (cont != nullptr) offset = cont->add_delayed<boolean_comparator>();
      ctx->add_function<boolean_comparator>();

      auto l_part = create_overload(ctx, lvalue, sol::object(), cont);
      auto r_part = make_raw_script_boolean(ctx, rvalue, cont);

      interface* ret = nullptr;
      if (cont != nullptr) {
        auto init = cont->get_init<boolean_comparator>(offset);
        ret = init.init(l_part, r_part);
      }

      return ret;
    }

    interface* system::make_compare(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont, const size_t &return_type) {
      assert(return_type == type_id<double>() || return_type == type_id<bool>());
      const bool compare_numbers = return_type == type_id<double>();
      if (compare_numbers) return make_number_compare(ctx, lvalue, rvalue, cont);
      return make_boolean_compare(ctx, lvalue, rvalue, cont);
    }

    interface* system::find_common_function(init_context* ctx, const std::string_view &func_name, const sol::object &obj, container* cont) {
      const size_t types[] = { type_id<void>(), type_id<bool>(), type_id<double>(), type_id<std::string_view>(), type_id<object>() };
      const size_t count = sizeof(types) / sizeof(types[0]);

      interface* ret = nullptr;
      for (size_t i = 0; i < count; ++i) {
        const auto func_itr = func_map.find(types[i]);
        assert(func_itr != func_map.end());

        const auto itr = func_itr->second.find(func_name);
        if (itr == func_itr->second.end()) continue;

        if (ret != nullptr) throw std::runtime_error("Found several common functions with same name '" + std::string(func_name) + "'");

        auto local = itr->second.func(this, ctx, obj, cont);
        ret = local;
        ctx->computed_type = itr->second.return_type;
      }

      const bool is_basic_getter_root = func_name == "root";
      const bool is_basic_getter_prev = func_name == "prev";
      const bool is_basic_getter_current = func_name == "current";
      const bool is_basic_getters = is_basic_getter_root || is_basic_getter_prev || is_basic_getter_current;
      const size_t basic_type = is_basic_getter_root ? ctx->root : (is_basic_getter_prev ? ctx->prev_type : (is_basic_getter_current ? ctx->current_type : SCRIPT_SYSTEM_ANY_TYPE));
      ctx->computed_type = is_basic_getters ? basic_type : ctx->computed_type;

      return ret;
    }

    interface* system::create_overload(init_context* ctx, const std::string_view &func_name, const sol::object &data, container* cont) {
      if (ctx->current_type == type_id<object>()) { // ctx->current_type == SIZE_MAX
        size_t new_obj_type = SIZE_MAX;
        size_t new_arg_count = SIZE_MAX;
        interface* o_begin = nullptr;
        interface* o_current = nullptr;
        size_t counter = 0;
        std::array<size_t, MAXIMUM_OVERLOADS> overload_types = {0};
        for (const auto &pair : func_map) {
          const auto itr = pair.second.find(func_name);
          if (itr == pair.second.end()) continue;
          if (counter >= MAXIMUM_OVERLOADS) throw std::runtime_error("Too many overloaded function " + std::string(func_name));

          overload_types[counter] = pair.first;
          ++counter;
          auto local = itr->second.func(this, ctx, data, cont);
          if (o_begin == nullptr) o_begin = local;
          if (o_current != nullptr) o_current->next = local;
          o_current = local;

          new_obj_type = new_obj_type == SIZE_MAX ? itr->second.return_type : new_obj_type;
          new_arg_count = new_arg_count == SIZE_MAX ? itr->second.arguments_count : new_arg_count;
          if (new_obj_type != itr->second.return_type) // new_obj_type != SIZE_MAX &&
            throw std::runtime_error("Function with same name '" + std::string(func_name) + "' have different return types, it is not allowed");
          if (new_arg_count != itr->second.arguments_count) // new_arg_count != SIZE_MAX &&
            throw std::runtime_error("Function with same name '" + std::string(func_name) + "' have different arguments count, it is not allowed");
        }

        if (ctx->computed_type == SIZE_MAX) ctx->computed_type = new_obj_type;
        if (ctx->expected_type != SCRIPT_SYSTEM_ANY_TYPE && ctx->computed_type != SCRIPT_SYSTEM_ANY_TYPE && ctx->computed_type != ctx->expected_type)
          throw std::runtime_error("Function '" + std::string(func_name) + "' returns unexpected type");

        if (counter == 0) throw std::runtime_error("Could not find function '" + std::string(func_name) + "', context " + std::string(ctx->function_name));
        if (counter == 1) return o_begin;

        interface* local = nullptr;
        if (cont != nullptr) { local = cont->add<DEVILS_SCRIPT_FULL_NAMESPACE::overload>(overload_types, o_begin); }
        ctx->add_function<DEVILS_SCRIPT_FULL_NAMESPACE::overload>();

        return local;
      }

      //using itr_t = decltype(func_map.find(0)->second.find("abc"));


      const auto map_itr = func_map.find(ctx->current_type);
      if (map_itr == func_map.end()) throw std::runtime_error("Unregistered type");

      const auto itr = map_itr->second.find(func_name);
      if (itr == map_itr->second.end()) {
        auto ret = find_common_function(ctx, func_name, data, cont);
        if (ret == nullptr) throw std::runtime_error("Could not find function '" + std::string(func_name) + "', context " + std::string(ctx->function_name));
        return ret;
      }

      auto local = itr->second.func(this, ctx, data, cont);
      if (ctx->computed_type == SIZE_MAX) ctx->computed_type = itr->second.return_type;
      if (ctx->expected_type != SCRIPT_SYSTEM_ANY_TYPE && ctx->computed_type != SCRIPT_SYSTEM_ANY_TYPE && ctx->computed_type != ctx->expected_type)
        throw std::runtime_error("Function '" + std::string(func_name) + "' returns unexpected type");
      return local;
    }

    static bool new_scope_pattern(const size_t &ret_type, const size_t &arg_count) {
      return ret_type != SCRIPT_SYSTEM_ANY_TYPE && !(type_id<void>() == ret_type || type_id<bool>() == ret_type || type_id<double>() == ret_type || type_id<std::string_view>() == ret_type) && arg_count == 0;
    }

    interface* system::make_effect_context(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont) {
      const bool has_condition = table_has_condition(rvalue);
      if (!has_condition && lvalue.empty()) return table_traverse(ctx, rvalue, cont);
      if (!has_condition && ctx->block_name == lvalue) return table_traverse(ctx, rvalue, cont);

      size_t ret_type = SIZE_MAX;
      size_t arg_count = SIZE_MAX;
      const bool complex_lvalue = is_complex_object(lvalue);
      const bool func_exists = !complex_lvalue ? function_exists(lvalue, &ret_type, &arg_count) : false;
      if (func_exists && arg_count == SCRIPT_SYSTEM_FUNCTION_ITERATOR) return create_overload(ctx, lvalue, rvalue, cont);
      if (func_exists && !new_scope_pattern(ret_type, arg_count)) {
        assert(ret_type == type_id<void>());
        return create_overload(ctx, lvalue, rvalue, cont);
      }

      size_t offset = SIZE_MAX;
      if (cont != nullptr) offset = cont->add_delayed<change_scope_effect>();
      ctx->current_size += sizeof(change_scope_effect);

      interface* scope = nullptr;
      {
        change_script_type cst(ctx, script_types::object);
        if (complex_lvalue) scope = make_complex_object(ctx, lvalue, rvalue, cont);
        else if (func_exists) scope = create_overload(ctx, lvalue, sol::object(), cont);
      }

      interface* cond = nullptr;
      if (const auto type = rvalue.get_type(); type == sol::type::table) {
        const auto table = rvalue.as<sol::table>();
        if (const auto proxy = table["condition"]; proxy.valid()) {
          change_script_type cst(ctx, script_types::condition);
          cond = make_raw_script_boolean(ctx, proxy, cont);
        }
      }

      auto childs = table_traverse(ctx, rvalue, cont);

      interface* ret = nullptr;
      if (cont != nullptr) {
        auto init = cont->get_init<change_scope_effect>(offset);
        ret = init.init(scope, cond, childs);
      }

      return ret;
    }

    interface* system::make_string_context(init_context* ctx, const sol::object &rvalue, container* cont) {
      // тут если кондишена нет, то нечего и делать то особо
      const bool has_condition = table_has_condition(rvalue);
      if (!has_condition) return table_traverse(ctx, rvalue, cont);

      size_t offset = SIZE_MAX;
      if (cont != nullptr) offset = cont->add_delayed<compute_string>();
      ctx->add_function<compute_string>();

      interface* cond = nullptr;
      if (const auto type = rvalue.get_type(); type == sol::type::table) {
        const auto table = rvalue.as<sol::table>();
        if (const auto proxy = table["condition"]; proxy.valid()) {
          change_script_type cst(ctx, script_types::condition);
          cond = make_raw_script_boolean(ctx, proxy, cont);
        }
      }

      auto childs = table_traverse(ctx, rvalue, cont);

      interface* ret = nullptr;
      if (cont != nullptr) {
        auto init = cont->get_init<compute_string>(offset);
        ret = init.init(cond, childs);
      }

      return ret;
    }

    interface* system::make_object_context(init_context* ctx, const sol::object &rvalue, container* cont) {
      // тут если кондишена нет, то нечего и делать то особо
      const bool has_condition = table_has_condition(rvalue);
      if (!has_condition) return table_traverse(ctx, rvalue, cont);

      size_t offset = SIZE_MAX;
      if (cont != nullptr) offset = cont->add_delayed<compute_object>();
      ctx->add_function<compute_object>();

      interface* cond = nullptr;
      if (const auto type = rvalue.get_type(); type == sol::type::table) {
        const auto table = rvalue.as<sol::table>();
        if (const auto proxy = table["condition"]; proxy.valid()) {
          change_script_type cst(ctx, script_types::condition);
          cond = make_raw_script_boolean(ctx, proxy, cont);
        }
      }

      auto childs = table_traverse(ctx, rvalue, cont);

      interface* ret = nullptr;
      if (cont != nullptr) {
        auto init = cont->get_init<compute_object>(offset);
        ret = init.init(cond, childs);
      }

      return ret;
    }

//     static bool compare_values_pattern(const size_t &ret_type, const size_t &arg_count) {
//       return (type_id<bool>() == ret_type || type_id<double>() == ret_type) && arg_count == 0;
//     }

    // такая же функция может быть еще и другими типами контекста
    interface* system::make_context_change(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont) {
      // сюда вообще то может придти еще SIZE_MAX по идее, например чтобы положить предыдущий объект в прев
      // что делать? я более менее понимаю что делать со сложным контекстом
      // тут 3 варианта: либо lvalue не задано, либо lvalue - сложная строка, либо lvalue - обычная существующая функция
      const bool has_condition = table_has_condition(rvalue);

      if (!has_condition && lvalue.empty()) return table_traverse(ctx, rvalue, cont);
      if (!has_condition && ctx->block_name == lvalue) return table_traverse(ctx, rvalue, cont);

      size_t ret_type = SIZE_MAX;
      size_t arg_count = SIZE_MAX;
      const bool complex_lvalue = is_complex_object(lvalue);
      const bool func_exists = !complex_lvalue ? function_exists(lvalue, &ret_type, &arg_count) : false;
      if (func_exists && arg_count == SCRIPT_SYSTEM_FUNCTION_ITERATOR) return create_overload(ctx, lvalue, rvalue, cont); // тут может быть функция random_*
      //if (func_exists && arg_count > 0) return create_overload(ctx, lvalue, rvalue, cont); // это обычная функция
      if (func_exists && !new_scope_pattern(ret_type, arg_count)) {
//         std::cout << "lvalue " << lvalue << "\n";
        return create_overload(ctx, lvalue, rvalue, cont); // это обычная функция
      }
      // где мы должны понять что нужно сравнение? может быть при создании функции
//       if (func_exists && compare_values_pattern(ret_type, arg_count)) {
//         //std::cout << "lvalue " << lvalue << "\n";
//         return make_compare(ctx, lvalue, rvalue, cont, ret_type);
//       }

      interface* cur = nullptr;

      size_t offset = SIZE_MAX;
      if (cont != nullptr) offset = cont->add_delayed<change_scope_condition>();
      ctx->current_size += sizeof(change_scope_condition);

      change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);

      // отсюда я должен получить новый тип
      interface* scope = nullptr;
      {
        change_script_type cst(ctx, script_types::object);
        if (func_exists) scope = create_overload(ctx, lvalue, sol::object(), cont);
        else if (complex_lvalue) scope = make_complex_object(ctx, lvalue, rvalue, cont);
      }

      //const size_t new_type = scope != nullptr ? ctx->computed_type : ctx->current_type;
      change_context_types cst(ctx, scope == nullptr ? ctx->current_type : ctx->computed_type, scope == nullptr ? ctx->prev_type : ctx->current_type);

      interface* cond = nullptr;
      if (const auto type = rvalue.get_type(); type == sol::type::table) {
        const auto table = rvalue.as<sol::table>();
        if (const auto proxy = table["condition"]; proxy.valid()) {
          change_script_type cst(ctx, script_types::condition);
          cond = make_raw_script_boolean(ctx, proxy, cont);
        }
      }

      auto child = ctx->current_block->func(this, ctx, rvalue, cont);

      if (cont != nullptr) {
        auto init = cont->get_init<change_scope_condition>(offset);
        cur = init.init(scope, cond, child);
      }

      return cur;
    }

    // static bool is_ASCII (const std::string_view &s) {
    //   return !std::any_of(s.begin(), s.end(), [] (char c) { return static_cast<unsigned char>(c) > 127; });
    // }

    static constexpr bool is_digit(char c) { return (c >= '0' && c <= '9'); } // || c == '+' || c == '-'

    static constexpr size_t compute_next_pos(const std::string_view &lvalue, const size_t &start) {
      for (size_t i = start; i < lvalue.size(); ++i) {
        if (i < lvalue.size()-1 && lvalue[i] == '.' && !is_digit(lvalue[i+1])) return i;
        if (i == lvalue.size()-1 && lvalue[i] == '.') return i;
      }

      return std::string_view::npos;
    }

    static constexpr size_t divide_complex_lvalue(const std::string_view &lvalue, const size_t &max_count, std::string_view* arr) {
      constexpr std::string_view dot = ".";
      size_t current = 0;
      size_t prev = 0;
      size_t counter = 0;
      constexpr size_t symbols_count = dot.length();
      while (current != std::string_view::npos && counter < max_count) {
        //current = lvalue.find(dot, prev);
        current = compute_next_pos(lvalue, prev);
        const size_t part_count = counter+1 == max_count ? std::string_view::npos : current-prev;
        const auto part = lvalue.substr(prev, part_count);
        arr[counter] = part;
        ++counter;
        prev = current + symbols_count;
      }

      counter = counter < max_count ? counter : SIZE_MAX;

      return counter;
    }

    static constexpr bool test() {
      constexpr std::string_view str = "context:title:+3.14.func1.func2:-3.14";
      std::array<std::string_view, 64> arr;
      const size_t count = divide_complex_lvalue(str, 64, arr.data());
      if (count != 3) return false;
      if (arr[0] != "context:title:+3.14") return false;
      if (arr[1] != "func1") return false;
      if (arr[2] != "func2:-3.14") return false;
      return true;
    }

    static_assert(test());

    static constexpr size_t divide_token(const std::string_view &str, const std::string_view &symbol, const size_t &max_count, std::string_view* array) {
      size_t current = 0;
      size_t prev = 0;
      size_t counter = 0;
      const size_t symbols_count = symbol.length();
      while (current != std::string_view::npos && counter < max_count) {
        current = str.find(symbol, prev);
        const size_t part_count = counter+1 == max_count ? std::string_view::npos : current-prev;
        const auto part = str.substr(prev, part_count);
        array[counter] = part;
        ++counter;
        prev = current + symbols_count;
      }

      counter = counter < max_count ? counter : SIZE_MAX;

      return counter;
    }

    static constexpr bool is_simple_number(const std::string_view &str) {
      for (const char c : str) { if (!isdigit(c)) return false; }
      return true;
    }

    // нужно разделить комплекс обжект и комплекс намбер, ожидаем сравнение только во втором
    // разделять к сожалению не особо удобно,
    interface* system::make_complex_object(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont) {
      //assert(is_complex_object(lvalue));

      sol::state_view s = rvalue.lua_state();

      interface* begin = nullptr;
      interface* current = nullptr;
      size_t counter = 0;

      // нужно еще вернуть ожидаемый тип
      size_t compare = compare_operators::more_eq;

      change_context_types cct(ctx, ctx->current_type, ctx->prev_type);
      change_script_type cst(ctx, ctx->script_type);
      ctx->script_type = SIZE_MAX;

      std::array<std::string_view, 64> tokens;
      // это нам еще числа похерит, какой еще вариант? сделать так чтобы числа могли быть только в конце, через двоеточие
      const size_t count = divide_complex_lvalue(lvalue, 64, tokens.data());
      if (count == SIZE_MAX) throw std::runtime_error("Bad complex lvalue string " + std::string(lvalue) + " in context " + std::string(ctx->function_name));
      for (size_t i = 0; i < count; ++i) {
        if (ctx->current_type == type_id<void>())
          throw std::runtime_error("Complex lvalue " + std::string(lvalue) + " evaluates to effect before end of token sequence, context " + std::string(ctx->function_name));

        //if (ctx->current_type == type_id<double>())
          //throw std::runtime_error("Complex lvalue " + std::string(lvalue) + " evaluates to number before end of token sequence, context " + std::string(ctx->function_name));

        const auto dot_token = tokens[i];
        std::array<std::string_view, 8> colon_tokens;
        const size_t colon_count = divide_token(dot_token, ":", 8, colon_tokens.data());
        if (colon_count == SIZE_MAX) throw std::runtime_error("Could not parse complex token part " + std::string(dot_token) + " in context " + std::string(ctx->function_name));

        // ожидаем что colon_tokens должен быть определен только в начале, хотя можно сделать через двоеточие указание переменной в конце
        //if (i != 0 && colon_count > 1) throw std::runtime_error("Bad complex object string " + std::string(lvalue));

        if (colon_count > 1) {
          for (size_t j = 0; j < colon_count; ++j) {
            const auto cur = colon_tokens[j];
            if (i == 0 && is_simple_number(cur)) continue;
            //if (i == 0 && RE2::FullMatch(cur, regex_number_matcher)) continue;

            if (const auto itr = compare_operators::map.find(cur); itr != compare_operators::map.end()) {
              compare = itr->second;
              continue;
            }

            // если это не оператор сравнения, то может быть функция + аргумент

            // если до сюда дошло, то ожидаем функцию + 1 числовой аргумент, один ли?
            // вообще наверное мы можем ожидать любое количество булевых, числовых или строковых аргументов
            // но думаю что сначала надо сделать хотя бы один
            // число обязано быть, чтобы сделать сравнение собственно с числом
            // нужно тогда этот обход точке сделать по другому, точнее составить массив иначе

            // мы можем ожидать функцию + 1 строковый ИЛИ числовой аргумент, может ли тут быть еще какая нибудь ошибка?
            // тут мы еще можем предугадать что лежит в контексте и в локале
            // вообще желательно наверное подсказать какой у нас первый и единственный аргумент, чтобы сразу дать понять что мы не обслуживаем
            // функции с аргументом-объектом

            // луа стейт лучше никак не менять, наверное тут лучше всего использовать std::any, другое дело как
            // std::any передать в функцию, они сейчас принимают только sol::object !!!
            // для того чтобы сделать так, придется переделать парсер, будем сначала парсить луа превращая его в набор
            // std::any + std::vector<std::string, std::any>, и только после этого составлять скрипт

            const auto func_id = colon_tokens[j];
            sol::object arg;
            if (j < colon_count-1) {
              const auto value = colon_tokens[j+1];
              if (value.empty()) throw std::runtime_error("Invalid complex lvalue token '" + std::string(dot_token) + "', context " + std::string(ctx->function_name));

              double num = 0.0;
              if (value == "true" || value == "false") {
                arg = sol::make_object(s, value == "true");
              //} else if (RE2::FullMatch(value, regex_number_matcher)) {
              } else if (const auto res = std::from_chars(value.begin(), value.end(), num); res.ec == std::errc()) {
                // ожидаем что это функция которая принимает на вход числовой аргумент
                // например "year_income:2.75" - функция "year_income" аргумент "2.75"
                // const auto res = std::from_chars(value.begin(), value.end(), num);
                // if (res.ec == std::errc::invalid_argument || res.ec == std::errc::result_out_of_range) {
                //   throw std::runtime_error("Could not parse number in token '" + std::string(cur) + ":" + std::string(value) + "', context " + std::string(ctx->function_name));
                // }

                arg = sol::make_object(s, num);
              } else {
                //throw std::runtime_error("Expected number value in second part of token '" + std::string(cur) + ":" + std::string(value) + "', context " + std::string(ctx->function_name));
                // ожидаем что это функция которая принимает на вход строковый аргумент
                // например "title:first_title" - функция "title" аргумент "first_title"
                if (is_complex_object(value))
                  throw std::runtime_error("Token '" + std::string(cur) + ":" + std::string(value) + "' expected to be valid pair "
                    "'funcion name:basic string', 'funcion name:number' or 'funcion name:boolean', context " + std::string(ctx->function_name));

                arg = sol::make_object(s, value);
              }
            }

            change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);
            auto local = create_overload(ctx, func_id, arg, cont);
            ctx->current_type = ctx->computed_type;
            if (begin == nullptr) begin = local;
            if (current != nullptr) current->next = local;
            current = local;
            ++counter;
            j += 1;
          }
        } else {
          assert(colon_count == 1);

          // токен title и другие могут быть и в качестве функций
          change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);
          const auto func_id = colon_tokens[0];
          auto local = create_overload(ctx, func_id, sol::object(), cont);
          ctx->current_type = ctx->computed_type;
          if (begin == nullptr) begin = local;
          if (current != nullptr) current->next = local;
          current = local;
          ++counter;
        }

        ctx->prev_type = ctx->current_type;
      }

      // тут ctx->current_type восстановится на то что было, нужно ли?
      // нужно его как то сохранить
      ctx->computed_type = ctx->current_type;
      ctx->compare_operator = compare;

      //const size_t type = get_script_type(ctx->computed_type);
      assert(get_script_type(ctx->computed_type) == cst.script_type);

      //assert(begin != nullptr);
      if (counter == 0) throw std::runtime_error("Could not parse complex lvalue '" + std::string(lvalue) + "'");
      if (counter == 1) return begin;
      //if (begin == current) return begin;

      interface* ret = nullptr;
      if (cont != nullptr) { ret = cont->add<complex_object>(begin); }
      ctx->add_function<complex_object>();
      return ret;
    }

//     interface* system::make_table_rvalue(init_context* ctx, const sol::object &obj, container* cont) {
//
//     }

    bool is_object_type(const size_t &type) {
      return type != type_id<void>() && type != type_id<bool>() && type != type_id<double>() && type != type_id<std::string_view>();
    }

    interface* system::make_table_lvalue(init_context* ctx, const std::string_view &lvalue, const sol::object &rvalue, container* cont) {
      // тут мы должны понять что за lvalue к нам пришло, это может быть сложный объект или просто функция
      // сложный объект должен просто возвращать какой то объект
      //if (is_complex_object(lvalue)) return make_complex_object(name, ctx, lvalue, rvalue, cont);

      if (ctx->current_type == SIZE_MAX) {
        // overload
      }

      return make_context_change(ctx, lvalue, rvalue, cont);
    }

    interface* system::condition_table_traverse(init_context* ctx, const sol::object &data, container* cont) {
      assert(compare_script_types(ctx->script_type, script_types::condition));
      interface* begin = nullptr;
      interface* cur = nullptr;
      sol::state_view s = data.lua_state();
      const auto table = data.as<sol::table>();

      for (const auto &pair : table) {
        interface* next = nullptr;
        const auto first_type = pair.first.get_type();
        switch (first_type) {
          case sol::type::boolean:
          case sol::type::number: {
            if (pair.second.get_type() == sol::type::table) next = make_context_change(ctx, "", pair.second, cont);
            else next = make_raw_script_boolean(ctx, pair.second, cont);
            break;
          }

          case sol::type::string: {
            // ожидаем название функции (в том числе функции из 'void' (общие функции)) или новый контекст
            const auto str = pair.first.as<std::string_view>();
            if (const auto ignore_key = ignore_keys::map.find(str); ignore_key != ignore_keys::map.end()) continue;
            next = make_table_lvalue(ctx, str, pair.second, cont);
            break;
          }

          default: throw std::runtime_error("Bad table lvalue of type " + std::string(detail::get_sol_type_name(first_type)) +
                     ", supported only boolean, number and string");
        }

        if (cur != nullptr) cur->next = next;
        if (begin == nullptr) begin = next;
        while (next != nullptr) { cur = next; next = next->next; }
      }

      return begin;
    }

    interface* system::numeric_table_traverse(init_context* ctx, const sol::object &data, container* cont) {
      assert(compare_script_types(ctx->script_type, script_types::numeric));
      interface* begin = nullptr;
      interface* cur = nullptr;
      sol::state_view s = data.lua_state();
      const auto table = data.as<sol::table>();

      for (const auto &pair : table) {
        interface* next = nullptr;
        const auto first_type = pair.first.get_type();
        switch (first_type) {
          case sol::type::boolean:
          case sol::type::number: {
            if (pair.second.get_type() == sol::type::table) next = make_context_change(ctx, "", pair.second, cont);
            else next = make_raw_script_number(ctx, pair.second, cont);
            break;
          }

          case sol::type::string: {
            // ожидаем название функции (в том числе функции из 'void' (общие функции)) или новый контекст
            // может придти функция без аргументов, а значит нужно сделать сравнение
            const auto str = pair.first.as<std::string_view>();
            if (const auto ignore_key = ignore_keys::map.find(str); ignore_key != ignore_keys::map.end()) continue;
            next = make_table_lvalue(ctx, str, pair.second, cont);
            break;
          }

          default: throw std::runtime_error("Bad table lvalue of type " + std::string(detail::get_sol_type_name(first_type)) +
                     ", supported only boolean, number and string, context " + std::string(ctx->function_name));
        }

        if (cur != nullptr) cur->next = next;
        if (begin == nullptr) begin = next;
        while (next != nullptr) { cur = next; next = next->next; }
      }

      return begin;
    }

    interface* system::string_table_traverse(init_context* ctx, const sol::object &data, container* cont) {
      // здесь может придти таблица с перечислением строк + таблицы с кондишенем
      assert(compare_script_types(ctx->script_type, script_types::string));
      interface* begin = nullptr;
      interface* cur = nullptr;
      sol::state_view s = data.lua_state();
      const auto table = data.as<sol::table>();

      for (const auto &pair : table) {
        interface* next = nullptr;
        const auto first_type = pair.first.get_type();
        switch (first_type) {
          //case sol::type::boolean: // ???
          case sol::type::number: {
            const auto cur_type = pair.second.get_type();
            if (cur_type == sol::type::string) next = make_raw_script_string(ctx, pair.second, cont);
            else if (cur_type == sol::type::table) next = make_string_context(ctx, pair.second, cont);
            else throw std::runtime_error("Bad table rvalue, supported only string or table, context " + std::string(ctx->function_name));
            break;
          }

          default: throw std::runtime_error("Bad table lvalue of type " + std::string(detail::get_sol_type_name(first_type)) +
                     ", supported only number, context " + std::string(ctx->function_name));
        }

        if (cur != nullptr) cur->next = next;
        if (begin == nullptr) begin = next;
        while (next != nullptr) { cur = next; next = next->next; }
      }

      return begin;
    }

    interface* system::object_table_traverse(init_context* ctx, const sol::object &data, container* cont) {
      assert(compare_script_types(ctx->script_type, script_types::object));
      interface* begin = nullptr;
      interface* cur = nullptr;
      sol::state_view s = data.lua_state();
      const auto table = data.as<sol::table>();

      const sol::object nil = sol::make_object(data.lua_state(), sol::nil);

      // а может тут быть lvalue строкой для того чтобы сменить контекс и искать объект в измененном контексте?
      // хороший вопрос
      for (const auto &pair : table) {
        interface* next = nullptr;
        const auto first_type = pair.first.get_type();
        switch (first_type) {
          //case sol::type::boolean: // ???
          case sol::type::number: {
          // тут может прийти таблица или сложный объект
            const auto cur_type = pair.second.get_type();
            if (cur_type == sol::type::string) {
              change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);
              next = make_complex_object(ctx, pair.second.as<std::string_view>(), nil, cont);
              if (ctx->expected_type != SCRIPT_SYSTEM_ANY_TYPE && ctx->computed_type != SCRIPT_SYSTEM_ANY_TYPE && ctx->computed_type != ctx->expected_type)
                throw std::runtime_error("Complex lvalue " + std::string(pair.second.as<std::string_view>()) +
                  " evaluates into object with unexpected type, context " + std::string(ctx->function_name));
            } else if (cur_type == sol::type::table) next = make_object_context(ctx, pair.second, cont);
            else throw std::runtime_error("Bad table rvalue, supported only string or table, context " + std::string(ctx->function_name));
            break;
          }

          default: throw std::runtime_error("Bad table lvalue of type " + std::string(detail::get_sol_type_name(first_type)) +
                     ", supported only number, context " + std::string(ctx->function_name));
        }

        if (cur != nullptr) cur->next = next;
        if (begin == nullptr) begin = next;
        while (next != nullptr) { cur = next; next = next->next; }
      }

      return begin;
    }

    interface* system::effect_table_traverse(init_context* ctx, const sol::object &data, container* cont) {
      assert(compare_script_types(ctx->script_type, script_types::effect));
      interface* begin = nullptr;
      interface* cur = nullptr;
      sol::state_view s = data.lua_state();
      const auto table = data.as<sol::table>();

      const sol::object nil = sol::make_object(data.lua_state(), sol::nil);

      for (const auto &pair : table) {
        interface* next = nullptr;
        const auto first_type = pair.first.get_type();
        switch (first_type) {
          // что сюда может придти? сюда приходят либо просто имена функций без аргументов либо имена функций и аргументы к ним + кондишен
          case sol::type::boolean:
          case sol::type::number: {
            // ожидаем сложную функцию без аргументов
            const auto cur_type = pair.second.get_type();
            if (cur_type == sol::type::string) {
              change_computed_type cct(ctx, SCRIPT_SYSTEM_ANY_TYPE);
              next = make_complex_object(ctx, pair.second.as<std::string_view>(), nil, cont);
              // тут по идее должна быть строгая проверка
              if (ctx->computed_type != type_id<void>())  //ctx->computed_type != SCRIPT_SYSTEM_ANY_TYPE &&
                throw std::runtime_error("Complex lvalue " + std::string(pair.second.as<std::string_view>()) +
                  " evaluates into object with unexpected type, context " + std::string(ctx->function_name));
            } else if (cur_type == sol::type::table) next = make_effect_context(ctx, std::string_view(), pair.second, cont);
            else throw std::runtime_error("Bad table rvalue, supported only string or table, context " + std::string(ctx->function_name));
            break;
          }

          case sol::type::string: {
            const auto str = pair.first.as<std::string_view>();
            if (const auto ignore_key = ignore_keys::map.find(str); ignore_key != ignore_keys::map.end()) continue;
            next = make_effect_context(ctx, str, pair.second, cont);
            break;
          }

          default: throw std::runtime_error("Unexpected table lvalue of type " + std::string(detail::get_sol_type_name(first_type)) +
                     ", supported only boolean, number and string, context " + std::string(ctx->function_name));
        }

        if (cur != nullptr) cur->next = next;
        if (begin == nullptr) begin = next;
        while (next != nullptr) { cur = next; next = next->next; }
      }

      return begin;
    }

    interface* system::table_traverse(init_context* ctx, const sol::object &obj, container* cont) {
      switch (ctx->script_type) {
        case script_types::condition: return condition_table_traverse(ctx, obj, cont);
        case script_types::numeric:   return numeric_table_traverse(ctx, obj, cont);
        case script_types::string:    return string_table_traverse(ctx, obj, cont);
        case script_types::effect:    return effect_table_traverse(ctx, obj, cont);
        case script_types::object:    return object_table_traverse(ctx, obj, cont);
        default: throw std::runtime_error("Add new script type");
      }

      return nullptr;
    }

    interface* system::make_boolean_container(const bool val, container* cont) {
      if (cont == nullptr) return nullptr;
      return cont->add<boolean_container>(val);
    }

    interface* system::make_number_container(const double val, container* cont) {
      if (cont == nullptr) return nullptr;
      return cont->add<number_container>(val);
    }

    interface* system::make_string_container(const std::string_view val, container* cont) {
      if (cont == nullptr) return nullptr;
      return cont->add<string_container>(val);
    }

    interface* system::make_object_container(const object val, container* cont) {
      if (cont == nullptr) return nullptr;
      return cont->add<object_container>(val);
    }

    interface* make_invalid_producer(container* cont) {
      if (cont == nullptr) return nullptr;
      return cont->add<invalid>();
    }

    interface* make_ignore_producer(container* cont) {
      if (cont == nullptr) return nullptr;
      return cont->add<ignore>();
    }

    script_data* system::create_script_raw(const size_t root_type, const sol::object &obj, const std::string_view name, const make_script_func_t func) {
      if (!obj.valid()) return nullptr;

      // совершенно необязательно использовать SIZE_MAX, когда я просто могу взять type_id<object>()
      // может быть нужно спрятать его в какой нибудь макрос
      //const size_t id = type_id<T>() == type_id<object>() ? SIZE_MAX : type_id<T>();
      init_context ctx;
      ctx.root = ctx.current_type = root_type;
      ctx.script_name = name;
      ctx.script_type = script_types::condition;

      // мне конечно по прежнему не нравится этот двойной обход, но наверное тут ничего не поделаешь
      // можно ли на 100% предугадать использование того или иного аргумента до непосредственно попытки создания объекта?
      (*this.*func)(&ctx, obj, nullptr);
      const size_t additional_data_size = align_to(sizeof(script_data), SCRIPT_SYSTEM_DEFAULT_ALIGMENT);
      auto cont = containers_pool.create(additional_data_size + ctx.current_size, SCRIPT_SYSTEM_DEFAULT_ALIGMENT);
      auto data = cont->add<script_data>();
      data->size = ctx.current_size;
      data->count = ctx.current_count;
      data->max_locals = ctx.current_local_var_id;
      populate_data_locals(data, &ctx);

      ctx.clear();
      auto i = (*this.*func)(&ctx, obj, cont);
      data->begin = i;
      containers.emplace_back(data, cont);
      assert(data->size == ctx.current_size);
      assert(data->count == ctx.current_count);
      assert(data->max_locals == ctx.current_local_var_id);

      return data;
    }

#define EVERY_FUNC_INIT                              \
  ctx->current_size += sizeof(function_type);        \
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
  if (init.valid()) final_int = init.init(std::string(name), cond, childs); \

    void system::register_every_list() {
      constexpr size_t script_type = get_script_type<object>();
      static_assert(script_type != script_types::object && script_type != script_types::string);

      constexpr std::string_view func_name = "every_in_list";

      init_func_data ifd{
        script_type, SCRIPT_SYSTEM_ANY_TYPE, SCRIPT_SYSTEM_EVERY_RETURN_TYPE, SCRIPT_SYSTEM_FUNCTION_ITERATOR,
        [] (system* sys, init_context* ctx, const sol::object &obj, container* cont) {
          constexpr std::string_view func_name = "every_in_list";

          const sol::table t = obj.as<sol::table>();
          const auto name_proxy = t["name"]; //"list"
          if (!name_proxy.valid() || name_proxy.get_type() != sol::type::string) throw std::runtime_error("'every_in_list' expects field 'name' with list name");
          const auto name = name_proxy.get<std::string_view>();

          change_block_function cbf(ctx, sys->get_init_function<void>(func_name));
          change_block_name cbn(ctx, func_name);
          change_current_function_name ccfn(ctx, func_name);

          interface* final_int = nullptr;
          if (ctx->script_type == script_types::effect) {
            using function_type = every_in_list_effect;

            EVERY_FUNC_INIT

          } else if (ctx->script_type == script_types::condition) {
            using function_type = every_in_list_logic;

            EVERY_FUNC_INIT

          } else if (ctx->script_type == script_types::numeric) {
            using function_type = every_in_list_numeric;

            EVERY_FUNC_INIT

          } else throw std::runtime_error("Cannot use 'every_in_list' in string or object script");

          return final_int;
        }, [] () {
          return "every_in_list"" I: " + std::string(type_name<void>()) +
                                " O: bool, double or nothing ";
        }
      };

      register_function(type_id<void>(), type_name<void>(), func_name, std::move(ifd));
    }

#undef EVERY_FUNC_INIT

    void system::register_function(const size_t &id, const std::string_view &type_name, const std::string_view &func_name, init_func_data&& data) {
      auto itr = func_map.find(id);
      if (itr == func_map.end()) throw std::runtime_error("Type " + std::string(type_name) + " is not registered");

      //if (func_name == "sign") throw std::runtime_error("afasfa");

      if (func_name.empty()) throw std::runtime_error("Empty function name string");
      if (is_complex_object(func_name)) throw std::runtime_error("Dots '.' and colons ':' is not allowed in function name");
      if (const auto itr = ignore_keys::map.find(func_name); itr != ignore_keys::map.end())
        throw std::runtime_error("Function name '" + std::string(func_name) + "' is not allowed");
      if (const auto itr = compare_operators::map.find(func_name); itr != compare_operators::map.end())
        throw std::runtime_error("Function name '" + std::string(func_name) + "' is not allowed");
      if (is_digit(func_name[0])) throw std::runtime_error("Function name '" + std::string(func_name) + "' must not start with digit");

      auto& map = itr->second;
      if (map.find(func_name) != map.end()) throw std::runtime_error("Function '" + std::string(func_name) + "' is already registered");

      map[func_name] = std::move(data);
    }

    static double one_minus(const double val) { return 1.0-val; }
    static double sin(const double val) { return std::sin(val); }
    static double cos(const double val) { return std::cos(val); }
    static double abs(const double val) { return std::abs(val); }
    static double ceil(const double val) { return std::ceil(val); }
    static double floor(const double val) { return std::floor(val); }
    static double round(const double val) { return std::round(val); }
    static double sqrt(const double val) { return std::sqrt(val); }
    static double sqr(const double val) { return val * val; }
    static double frac(const double val) {
      double unused = 0.0f;
      return std::modf(val, &unused);
    }

    static double inv(const double val) { return -val; }
    static double clamp(const double val, const double min, const double max) { return std::clamp(val, min, max); }
    static double mix(const double x, const double y, const double a) { return x * (1.0 - a) + y * a; } // glm::mix(x, y, a);
    static double smoothstep(const double edge0, const double edge1, const double x) {
      const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
      return t * t * (3.0 - 2.0 * t);
      //return glm::smoothstep(edge0, edge1, x);
    }

    static double step(const double edge, const double x) {
      //return glm::step(edge, x);
      return x >= edge;
    }
    static double sign(const double x) { return std::abs(x) < EPSILON ? 0.0 : (x < 0.0 ? -1.0 : 1.0 ); }

    // делать ли тут проверки?
    static double pow(const double x, const double y) {
      if (x < 0.0) throw std::runtime_error("Function pow with x < 0.0 is undefined");
      if (std::abs(x) < EPSILON && y <= 0.0) throw std::runtime_error("Function pow with x == 0.0 and y <= 0.0 is undefined");
      return std::pow(x, y);
    }

    static double log2(const double x) { return std::log2(x); }
    static double log(const double x) { return std::log(x); }
    static double fma(const double a, const double b, const double c) { return a * b + c; } // glm::fma(a, b, c);
    static double exp(const double x) { return std::exp(x); }
    static double acos(const double x) { return std::acos(x); }
    static double asin(const double x) { return std::asin(x); }
    static bool isnan(const double x) { return std::isnan(x); }
    // static double exp2(const double x) { return glm::exp2(x); }
    // static double cosh(const double x) { return glm::cosh(x); }
    // static double sinh(const double x) { return glm::sinh(x); }
    // static double acosh(const double x) { return glm::acosh(x); }
    // static double asinh(const double x) { return glm::asinh(x); }

    // нужно еще добавить алиасы и что самое важное реакции на эффекты
    // можно ли добавить реакцию на любую функцию? это непрактично,
    // вычисления и условия задумывались как константные функции,
    // такие функции не должны никоем образом взаимодействовать с локальным стейтом или с внешним
    // как добавить алиас? самое простое это создать ту же функцию но с другим названием
    // это и название в функции сменит, нужно ли именно тут делать реакцию?
    // или ее лучше сторонними средствами добавить? проще иметь возможность тут указать что запускать
    // с другой стороны запуск этого лучше отложить по времени, вот что точно:
    // нужно запустить какую то функцию во время выполнения эффекта
    // как сделать эту функцию? вообще я так понимаю можно сделать какую то генерик функцию
    // а потом ее переопределить где нибудь в коде, как выглядит функция? наверное максимально просто:
    // template <typename... Args>
    // void on_effect(const std::string_view name, Args args...);
    // где первый аргумент должен быть текущий хендл, а остальные это входные данные
    // переопределять такую функцию сложно: любое изменение аргументов функции
    // потребует изменение и этой функции + непонятно как сделать совсем дженерик функцию,
    // придется переопределять для всех аргументов примерно тоже самое
    // лучший вариант - это сделать отдельную функцию для каждого эффекта, как она выглядит?
    // void on_effect(const std::string_view name, const object current, const std::array<object, 16> &args); + нужно наверное еще снабдить юзер_датой
    // можно ли как нибудь избежать четкого количества аргументов? наверное можно использовать std::initializer_list
    // как функцию задать? наверное как указатель на функцию в темплейтах, в такой функции мне скорее всего потребуется
    // задать контекст для ивентов и каких то других эффектов, как это сделать автоматически? это довольно сложно,
    // с++ не дает узнать имена входных переменных, но хорошо у большинства функций одно входное значение
    // нужно ли возвращаемое значение передать? скорее нет

    // когда запускать эффекты и кондишены реакции на функцию? там же где вызвали или отсрочить это дело?
    // для скорости наверное имеет смысл отсрочить, но при этом могут быть неприятные коллизии
    // для некоторых состояний придется придумать специальные флажки, чтобы другие энтити понимали
    // что происходит, например смерть нужно отсрочить чтобы посчитать наследство, во время отсрочки может быть
    // какое то обращение к покойнику

    // мне еще нужно понять как можно сделать функции для glm::vec4 так чтобы те пересекались с обычными нумериками,
    // вопервых отдельный тип скрипта + видимо отдельное хранилище для функций типа: оно существует,
    // по идее в оверлоаде можно проверить если это тип вектор,

//     if constexpr (!std::is_same_v<output_type, void>) {
//       if constexpr (curr == N) ret = object(std::invoke(f, std::forward<Args>(args)...));
//       else {
//         using curr_type = final_arg_type<F, curr>;
//         auto obj = current->process(ctx);
//         auto arg = obj.template get<curr_type>();
//         return call_func<F, f, name, react, N, curr+1>(ctx, current->next, std::forward<Args>(args)..., arg);
//       }
//     } else {
//       if constexpr (curr == N) std::invoke(f, std::forward<Args>(args)...);
//       else {
//         using curr_type = final_arg_type<F, curr>;
//         auto obj = current->process(ctx);
//         auto arg = obj.template get<curr_type>();
//         call_func<F, f, name, react, N, curr+1>(ctx, current->next, std::forward<Args>(args)..., arg);
//       }
//     }
//     if constexpr (curr == N && react != nullptr) { if (ctx->draw_state()) { react(name, ctx->current, { object(args)... }, ctx->user_data); } }

//     if constexpr (!std::is_same_v<output_type, void>) return call_func<F, f, name, react, func_arg_count, 0>(ctx, args);
//     else call_func<F, f, name, react, func_arg_count, 0>(ctx, args);
//     return object();

//     if constexpr (!std::is_same_v<output_type, void>) {
//       if constexpr (curr == N) ret = object(std::invoke(f, handle, std::forward<Args>(args)...));
//       else {
//         using curr_type = std::remove_reference_t<function_argument_type<F, curr>>;
//         auto obj = current->process(ctx);
//         auto arg = obj.template get<curr_type>();
//         return call_func<Th, F, f, N, curr+1>(ctx, handle, current->next, std::forward<Args>(args)..., arg);
//       }
//     } else {
//       if constexpr (curr == N) std::invoke(f, handle, std::forward<Args>(args)...);
//       else {
//         using curr_type = std::remove_reference_t<function_argument_type<F, curr>>;
//         auto obj = current->process(ctx);
//         auto arg = obj.template get<curr_type>();
//         call_func<Th, F, f, N, curr+1>(ctx, handle, current->next, std::forward<Args>(args)..., arg);
//       }
//     }
//     if constexpr (curr == N && react != nullptr) { if (ctx->draw_state()) { react(name, ctx->current, { object(args)... }, ctx->user_data); } }

//     if constexpr (!std::is_same_v<output_type, void>) return call_func<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(ctx, cur, args);
//     else call_func<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(ctx, cur, args);
//     return object();

    void system::init() {
      register_usertype<void>();
      register_usertype<bool>();
      register_usertype<double>();
      register_usertype<std::string_view>();
      register_usertype<object>();

      // это проверит объект только на тип, но не на валидность данных в нем
      register_function<void, type_checker<void, "is_valid"_create>, bool, "is_valid"_create>();
      register_function<void, type_checker<bool, "is_condition"_create>, bool, "is_condition"_create>();
      register_function<void, type_checker<double, "is_numeric"_create>, bool, "is_numeric"_create>();
      register_function<void, type_checker<std::string_view, "is_string"_create>, bool, "is_string"_create>();
      register_function<void, type_checker<object, "any_object"_create>, bool, "any_object"_create>();

#define LOCAL_REG_BASIC(name) REG_BASIC(name, #name)();
#define LOCAL_REG_BASIC_ARGS(name) REG_BASIC(name, #name)();
#define LOCAL_REG_ITR(name, ret_type) REG_ITR(void, name, ret_type, #name)();

#define NUMERIC_COMMAND_BLOCK_FUNC(name) REG_ITR(void, name, double, #name)();
      NUMERIC_COMMANDS_LIST2
#undef NUMERIC_COMMAND_BLOCK_FUNC

      constexpr size_t func_arg_count = get_function_argument_count<decltype(&clamp)>();
      static_assert(func_arg_count == 3);

      LOCAL_REG_BASIC(one_minus)
      LOCAL_REG_BASIC(sin)
      LOCAL_REG_BASIC(cos)
      LOCAL_REG_BASIC(abs)
      LOCAL_REG_BASIC(ceil)
      LOCAL_REG_BASIC(floor)
      LOCAL_REG_BASIC(round)
      LOCAL_REG_BASIC(sqrt)
      LOCAL_REG_BASIC(sqr)
      LOCAL_REG_BASIC(frac)
      LOCAL_REG_BASIC(inv)
      LOCAL_REG_BASIC(log2)
      LOCAL_REG_BASIC(log)
      LOCAL_REG_BASIC(exp)
      LOCAL_REG_BASIC(acos)
      LOCAL_REG_BASIC(asin)
      LOCAL_REG_BASIC_ARGS(clamp)
      LOCAL_REG_BASIC_ARGS(mix)
      LOCAL_REG_BASIC_ARGS(smoothstep)
      LOCAL_REG_BASIC_ARGS(step)
      LOCAL_REG_BASIC_ARGS(sign)
      LOCAL_REG_BASIC_ARGS(pow)
      LOCAL_REG_BASIC_ARGS(fma)
      LOCAL_REG_BASIC(isnan)
      // LOCAL_REG_BASIC(exp2)
      // LOCAL_REG_BASIC(cosh)
      // LOCAL_REG_BASIC(sinh)
      // LOCAL_REG_BASIC(acosh)
      // LOCAL_REG_BASIC(asinh)

      //REG_BASIC_NAMED_ARGS(clamp, "clamp", "value", "min", "max")();

#define LOGIC_BLOCK_COMMAND_FUNC(name) REG_ITR(void, name, bool, #name)();
      LOGIC_BLOCK_COMMANDS_LIST
#undef LOGIC_BLOCK_COMMAND_FUNC

      //LOCAL_REG_ITR(selector, object)
      LOCAL_REG_ITR(equality, bool)
      LOCAL_REG_ITR(type_equality, bool)

      // я указал это дело в system::find_common_function
      REG_BASIC_TYPE(void, root, object, "root")();
      REG_BASIC_TYPE(void, prev, object, "prev")();
      REG_BASIC_TYPE(void, current, object, "current")();

      REG_BASIC_TYPE(void, index, double, "index")();
      REG_BASIC_TYPE(void, prev_index, double, "prev_index")();

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<save_local> init) {
          // тут мы ожидаем строку с названием, ее мы "вычислим" то есть зарегистрируем в контексте
          // и ожидаем любой непустой объект, к сожалению отсюда я сказать не могу что нам нужно
          // может быть объект, а может быть число, строку ожидать не можем
          // вот что - объект можем сохранить текущий, а для числа будем ожидать в таблице ключ "value"

          if (obj.get_type() == sol::type::table) {
            const auto t = obj.as<sol::table>();
            const auto str_proxy = t["name"];
            const auto number_proxy = t["value"];
            if (!str_proxy.valid() || !number_proxy.valid()) throw std::runtime_error("Function 'save_local' expects fields 'name' with string and 'value' with scripted number");
            if (str_proxy.get_type() != sol::type::string) throw std::runtime_error("Function 'save_local' expects field 'name' to be string");
            const auto str = str_proxy.get<std::string_view>();
            const size_t index = v.save_local(std::string(str), type_id<double>());
            //auto num = v.make_scripted_numeric(number_proxy);
            auto num = v.make_scripted_object<object>(number_proxy); // любой объект, число получаем функцией value
            return init.init(std::string(str), index, num);
          }

          if (obj.get_type() != sol::type::string) throw std::runtime_error("Function 'save_local' expects string or table as input data");

          const auto str = obj.as<std::string_view>();
          const size_t index = v.save_local(std::string(str), v.get_context()->current_type);
          return init.init(std::string(str), index, nullptr);
        };
        REG_USER(void, save_local, object, "save_local")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<has_local> init) {
          if (obj.get_type() != sol::type::string) throw std::runtime_error("Function 'has_local' expects string as input data");
          const auto str = obj.as<std::string_view>();
          const auto [index, type] = v.get_local(str);
          return init.init(index);
        };
        REG_USER(void, has_local, bool, "has_local")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<remove_local> init) {
          if (obj.get_type() != sol::type::string) throw std::runtime_error("Function 'remove_local' expects string as input data");
          const auto str = obj.as<std::string_view>();
          // наверное имеет смысл именно удалить запись о переменной
          // хотя эти вещи могут быть удалены/созданы в каких то условных переходах
          // то есть всего 64 переменных под разными названиями
          const size_t index = v.remove_local(str);
          if (index == SIZE_MAX) throw std::runtime_error("Could not find local variable " + std::string(str) + ", context 'remove_local'");
          return init.init(str, index);
        };
        REG_USER(void, remove_local, object, "remove_local")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<save> init) {
          if (obj.get_type() == sol::type::table) {
            const auto t = obj.as<sol::table>();
            const auto str_proxy = t["name"];
            const auto number_proxy = t["value"];
            if (!str_proxy.valid() || !number_proxy.valid()) throw std::runtime_error("Function 'save' expects fields 'name' with string and 'value' with scripted number");
            if (str_proxy.get_type() != sol::type::string) throw std::runtime_error("Function 'save' expects field 'name' to be string");
            const auto str = str_proxy.get<std::string_view>();
            auto num = v.make_scripted_object<object>(number_proxy); // любой объект, число получаем функцией value
            return init.init(str, num);
          }

          if (obj.get_type() != sol::type::string) throw std::runtime_error("Function 'save' expects string or table as input data");

          const auto str = obj.as<std::string_view>();
          return init.init(str, nullptr);
        };
        REG_USER(void, save, object, "save")(f);
      }

      {
        const auto f = [] (view, const sol::object &obj, container::delayed_initialization<has_context> init) {
          if (obj.get_type() != sol::type::string) throw std::runtime_error("Function 'has_local' expects string as input data");
          const auto str = obj.as<std::string_view>();
          return init.init(str);
        };
        REG_USER(void, has_context, bool, "has_context")(f);
      }

      {
        const auto f = [] (view, const sol::object &obj, container::delayed_initialization<remove> init) {
          if (obj.get_type() != sol::type::string) throw std::runtime_error("Function 'remove' expects string as input data");
          const auto str = obj.as<std::string_view>();
          return init.init(str);
        };
        REG_USER(void, remove, object, "remove")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<sequence> init) {
          if (obj.get_type() != sol::type::table) throw std::runtime_error("Function 'sequence' expects table as input");

          interface* count_val = nullptr;
          interface* begin = nullptr;
          interface* current = nullptr;
          const auto t = obj.as<sol::table>();
          for (const auto &pair : t) {
            if (pair.first.get_type() == sol::type::string) {
              const auto str = pair.first.as<std::string_view>();
              if (const auto itr = ignore_keys::map.find(str); itr != ignore_keys::map.end()) continue;
              if (str == "count") {
                count_val = v.make_scripted_numeric(pair.second);
                continue;
              }

              throw std::runtime_error("String keys, except 'count', in function 'sequence' is not allowed");
            }

            if (pair.first.get_type() != sol::type::number) throw std::runtime_error("Function 'sequence' expects array with tables or 'count' obj");

            // тут нужно создать блочные функции
            auto cur = v.any_scripted_object(pair.second);
            if (begin == nullptr) begin = cur;
            if (current != nullptr) current->next = cur;
            current = cur;
          }

          return init.init(count_val, begin);
        };
        REG_ITR(void, sequence, object, "sequence")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<selector> init) {
          if (obj.get_type() != sol::type::table) throw std::runtime_error("Function 'selector' expects table as input");

          interface* begin = nullptr;
          interface* current = nullptr;
          const auto t = obj.as<sol::table>();
          for (const auto &pair : t) {
            if (pair.first.get_type() == sol::type::string) {
              const auto str = pair.first.as<std::string_view>();
              // указание на то что пропущено скорее будет сбивать с толку
              if (const auto itr = ignore_keys::map.find(str); itr != ignore_keys::map.end()) continue;
            }

            if (pair.first.get_type() != sol::type::number) continue;

            // тут нужно создать блочные функции
            auto cur = v.any_scripted_object(pair.second);
            if (begin == nullptr) begin = cur;
            if (current != nullptr) current->next = cur;
            current = cur;
          }

          return init.init(begin);
        };
        REG_ITR(void, selector, object, "selector")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<chance> init) {
          auto val = v.make_scripted_numeric(obj);
          const size_t state = v.get_random_state();
          return init.init(state, val);
        };
        REG_ITR(void, chance, bool, "chance")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<random_value> init) -> interface* {
          interface* val = nullptr;
          if (obj.valid()) val = v.make_scripted_numeric(obj);
          const size_t state = v.get_random_state();
          return init.init(state, val);
        };
        REG_ITR(void, random_value, double, "random_value")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<weighted_random> init) -> interface* {
          interface* begin_childs = nullptr;
          interface* cur_child = nullptr;
          interface* begin_weights = nullptr;
          interface* cur_weight = nullptr;
          const auto table = obj.as<sol::table>();
          for (const auto &pair : table) {
            if (pair.second.get_type() != sol::type::table) continue;
            if (pair.first.get_type() == sol::type::string) {
              const auto str = pair.first.as<std::string_view>();
              if (const auto itr = ignore_keys::map.find(str); itr != ignore_keys::map.end()) continue;
            }

            const auto t = pair.second.as<sol::table>();
            const auto proxy = t["weight"];
            {
              if (!proxy.valid()) throw std::runtime_error("'weighted_random' expects array of lua tables with weight number script field");
              auto ret = v.make_scripted_numeric(proxy);
              if (begin_weights == nullptr) begin_weights = ret;
              if (cur_weight != nullptr) cur_weight->next = ret;
              cur_weight = ret;
            }

            if (const auto proxy = t["condition"]; proxy.valid()) throw std::runtime_error("'condition' field located in same block with 'weight' in function 'weighted_random' is considered as error");

            // тут наверное справедливы *_table_init, а в селекторе или в сиквенсе нет
            // нет, тут тоже требуется контейнеры по умолчанию
            {
              auto cur = v.any_scripted_object(pair.second);
              if (begin_childs == nullptr) begin_childs = cur;
              if (cur_child != nullptr) cur_child->next = cur;
              cur_child = cur;
            }
          }

          const size_t state = v.get_random_state();
          return init.init(state, begin_childs, begin_weights);
        };
        REG_ITR(void, weighted_random, object, "weighted_random")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<equals_to> init) -> interface* {
          //auto val = v.any_scripted_object(obj);
          auto val = v.make_scripted_object<object>(obj);
          return init.init(val);
        };
        REG_USER(void, equals_to, bool, "equals_to")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<not_equals_to> init) -> interface* {
          //auto val = v.any_scripted_object(obj);
          auto val = v.make_scripted_object<object>(obj);
          return init.init(val);
        };
        REG_USER(void, not_equals_to, bool, "not_equals_to")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<compare> init) -> interface* {
          if (obj.get_type() != sol::type::table) throw std::runtime_error("'compare' function expects table");

          size_t op = compare_operators::more_eq;
          const auto t = obj.as<sol::table>();
          if (const auto proxy = t["op"]; proxy.valid()) {
            if (proxy.get_type() != sol::type::string) throw std::runtime_error("Table field 'op' must be string");
            const auto str = proxy.get<std::string_view>();
            const auto itr = compare_operators::map.find(str);
            if (itr == compare_operators::map.end()) throw std::runtime_error("Could not find compare operator " + std::string(str));
            op = itr->second;
          }

          // нужно дать понять что ожидаются числа
          auto childs = v.traverse_children_numeric(obj);
          return init.init(op, childs);
        };
        REG_USER(void, compare, bool, "compare")(f);
      }

      {
        // add_to_list, is_in_list, (random,every,has) x_in_list -
        // наверное add_to_list нужно сделать по аналогии с save_local (добавить поле value)
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<add_to_list> init) -> interface* {
          // что ожидаем? можем ли мы добавить в лист любой объект? можем ли мы добавить в лист число/булеан?
          // наверное числа добавлять в лист не нужно, объект - любой, нужн добавить проверку типов
          // либо в лист добавить только один тип, как обойти?

          if (obj.get_type() != sol::type::string) throw std::runtime_error("'add_to_list' expects list name");

          // нужно ли проверить что лист создан? ну не здесь а в других функкциях
          const auto str = obj.as<std::string_view>();
          v.add_list(std::string(str));

          return init.init(std::string(str));
        };
        REG_USER(void, add_to_list, object, "add_to_list")(f);
      }

      {
        const auto f = [] (view, const sol::object &obj, container::delayed_initialization<is_in_list> init) -> interface* {
          if (obj.get_type() != sol::type::string) throw std::runtime_error("'is_in_list' expects list name");
          const auto str = obj.as<std::string_view>();
          // проверка?
          return init.init(std::string(str));
        };
        REG_USER(void, is_in_list, bool, "is_in_list")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<has_in_list> init) -> interface* {
          if (obj.get_type() != sol::type::table) throw std::runtime_error("'has_in_list' expects table");

          const auto t = obj.as<sol::table>();
          const auto name_proxy = t["name"]; //"list"
          if (!name_proxy.valid() || name_proxy.get_type() != sol::type::string) throw std::runtime_error("'has_in_list' expects field 'name' with list name");
          const auto name = name_proxy.get<std::string_view>();

          interface* percentage = nullptr;
          interface* max_count = nullptr;
          if (const auto proxy = t["percentage"]; proxy.valid()) {
            percentage = v.make_scripted_numeric(proxy);
          } else if (const auto proxy = t["max_count"]; proxy.valid()) {
            max_count = v.make_scripted_numeric(proxy);
          }

          auto childs = v.traverse_children(obj);

          return init.init(std::string(name), max_count, percentage, childs);
        };
        REG_ITR(void, has_in_list, double, "has_in_list")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<random_in_list> init) -> interface* {
          if (obj.get_type() != sol::type::table) throw std::runtime_error("'random_in_list' expects table");

          auto t = obj.as<sol::table>();
          const auto name_proxy = t["name"]; //"list"
          if (!name_proxy.valid() || name_proxy.get_type() != sol::type::string) throw std::runtime_error("'random_in_list' expects field 'name' with list name");
          const auto name = name_proxy.get<std::string_view>();

          interface* condition = nullptr;
          interface* weight = nullptr;
          if (const auto proxy = t["condition"]; proxy.valid()) {
            condition = v.make_scripted_numeric(proxy);
          }

          if (const auto proxy = t["weight"]; proxy.valid()) {
            weight = v.make_scripted_numeric(proxy);
          }

          //auto childs = v.traverse_children(obj);
          const sol::object cond_obj = t["condition"];
          t["condition"] = sol::nil;
          auto childs = v.any_scripted_object(obj);
          t["condition"] = cond_obj;

          const size_t state = v.get_random_state();
          return init.init(std::string(name), state, condition, weight, childs);
        };
        REG_ITR(void, random_in_list, object, "random_in_list")(f);
      }

      // тут потребуется новая функция: мне нужно создать разные типы
      register_every_list();

      {
        const auto f = [] (view, const sol::object &obj, container::delayed_initialization<get_context> init) -> interface* {
          if (obj.get_type() != sol::type::string) throw std::runtime_error("Function 'context' expects string as input");
          const auto str = obj.as<std::string_view>();
          // тут мы можем погадать что у нас в контексте, в контексте по идее всегда лежит как бы любой тип
          return init.init(str);
        };
        REG_USER(void, get_context, object, "context")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<get_local> init) -> interface* {
          if (obj.get_type() != sol::type::string) throw std::runtime_error("Function 'local' expects string as input");
          const auto str = obj.as<std::string_view>();
          const auto [index, type] = v.get_local(str);
          // локал вернет скорее всего определенный тип
          v.set_return_type(type);
          return init.init(std::string(str), index);
        };
        REG_USER(void, get_local, object, "local")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<value> init) -> interface* {
          //if (obj.get_type() != sol::type::number) throw std::runtime_error("Function 'value' expects number as input");
          //const auto num = obj.as<double>();
          auto compute = v.make_scripted_numeric(obj);
          return init.init(compute);
        };
        REG_USER(void, value, double, "value")(f);
      }

      // нам еще потребуется хранить глобальные переменные + переменные в объекте?

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<assert_condition> init) -> interface* {
          if (obj.get_type() != sol::type::table) throw std::runtime_error("Function 'assert_condition' expects table as input");

          const auto t = obj.as<sol::table>();
          const auto cond_proxy = t["condition"];
          if (!cond_proxy.valid()) throw std::runtime_error("Function 'assert_condition' must have a 'condition' field");
          auto cond = v.make_scripted_conditional(cond_proxy);

          const auto text_proxy = t["text"];
          interface* str = nullptr;
          if (text_proxy.valid()) str = v.make_scripted_string(text_proxy);

          return init.init(cond, str);
        };
        REG_USER(void, assert_condition, bool, "assert_condition")(f);
      }

      // было бы неплохо добавить функции для view: transform, filter, reduce, take, drop
      // как должен выглядеть сам view? для того чтобы это создать нам действительно нужен ЛЮБОЙ
      // объект как возвращаемое значение, вне зависимости от типа скрипта

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<transform> init) -> interface* {
          // тут мы ожидаем что? на самом деле скорее всего все что угодно, но тут может быть и таблица
          // из таблицы по умолчанию ожидаем число? наверное
          interface* changes = nullptr;
          if (obj.get_type() == sol::type::string) {
            // это скорее всего сложное лвалуе: ожидаем любой объект
            changes = v.make_scripted_object<object>(obj);
          } else {
            changes = v.make_scripted_numeric(obj);
          }

          return init.init(changes);
        };
        REG_USER(void, transform, object, "transform")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<filter> init) -> interface* {
          // тут мы ожидаем таблицу с вычислением булеана (а мож и не таблицу)
          auto cond = v.make_scripted_conditional(obj);
          return init.init(cond);
        };
        REG_USER(void, filter, object, "filter")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<reduce> init) -> interface* {
          // тут что? вот тут было бы неплохо сделать скрипт-объект, то есть тут бы мы например выбрали бы самый крутой город по какой-нибудь метрике
          // да, и число тут было бы тоже неплохо ожидать, как тут это число можно вычислить? действительно нужно сделать функцию value
          // которая тупо вычислит число
          interface* red = nullptr;
          const size_t script_type = v.get_context()->script_type;
          if (script_type == script_types::condition) {
            red = v.make_scripted_conditional(obj);
          } else if (script_type == script_types::numeric) {
            red = v.make_scripted_numeric(obj);
          } else if (script_type == script_types::object) {
            const size_t expected = v.get_context()->expected_type;
            red = v.make_scripted_object(expected, obj);
          }

          return init.init(red);
        };
        REG_USER(void, reduce, object, "reduce")(f);
      }

      {
        const auto f = [] (view, const sol::object &obj, container::delayed_initialization<take> init) -> interface* {
          if (obj.get_type() != sol::type::number) throw std::runtime_error("Function 'take' expects number as input");
          return init.init(obj.as<double>());
        };
        REG_USER(void, take, object, "take")(f);
      }

      {
        const auto f = [] (view, const sol::object &obj, container::delayed_initialization<drop> init) -> interface* {
          if (obj.get_type() != sol::type::number) throw std::runtime_error("Function 'drop' expects number as input");
          return init.init(obj.as<double>());
        };
        REG_USER(void, drop, object, "drop")(f);
      }

      {
        const auto f = [] (view v, const sol::object &obj, container::delayed_initialization<execute> init) -> interface* {
          if (obj.get_type() == sol::type::string) {
            const auto str = obj.as<std::string_view>();
            const auto [safe_name, script, type] = v.find_script(str);
            if (script == nullptr) throw std::runtime_error("Could not find script '" + std::string(str) + "'");
            v.set_return_type(type);
            return init.init(safe_name, script, std::vector<const interface*>{});
          }

          if (obj.get_type() != sol::type::table) throw std::runtime_error("Function 'execute' expects string or table as input");

          const auto t = obj.as<sol::table>();
          const auto name_proxy = t["name"];
          if (!name_proxy.valid() || name_proxy.get_type() != sol::type::string) throw std::runtime_error("'execute' expects field 'name' with script name");
          const auto str = name_proxy.get<std::string_view>();
          const auto [safe_name, script, type] = v.find_script(str);
          if (script == nullptr) throw std::runtime_error("Could not find script '" + std::string(str) + "'");
          v.set_return_type(type);

          std::vector<const interface*> new_locals(script->locals.size(), nullptr);
          for (const auto &p : t) {
            if (p.first.get_type() != sol::type::string) continue;

            const auto str = p.first.as<std::string_view>();
            if (const auto itr = ignore_keys::map.find(str); itr != ignore_keys::map.end()) continue;

            size_t index = SIZE_MAX;
            for (size_t i = 0; i < script->locals.size(); ++i) {
              if (script->locals[i] != str) continue;
              index = i;
              break;
            }

            if (index == SIZE_MAX) throw std::runtime_error("Could not find local variable '" + std::string(str) + "' amongst local variables for script '" + std::string(safe_name) + "'");

            auto var = v.make_scripted_object<object>(p.second);
            new_locals[index] = var;
          }

          return init.init(safe_name, script, std::move(new_locals));
        };
        REG_USER(void, execute, object, "execute")(f);
      }
    }

    namespace detail {
      const phmap::flat_hash_set<std::string_view> view_allowed_funcs = {
        "transform", "filter", "reduce", "take", "drop"
      };

      std::string_view get_sol_type_name(const sol::type &t) {
        std::string_view str;
        switch (t) {
          case sol::type::none:     str = "none"; break;
          case sol::type::lua_nil:  str = "nil"; break;
//           case sol::type::nil: str = "none"; break;
          case sol::type::string:   str = "string"; break;
          case sol::type::number:   str = "number"; break;
          case sol::type::thread:   str = "thread"; break;
          case sol::type::boolean:  str = "boolean"; break;
          case sol::type::function: str = "function"; break;
          case sol::type::userdata: str = "userdata"; break;
          case sol::type::lightuserdata: str = "lightuserdata"; break;
          case sol::type::table:    str = "table"; break;
          case sol::type::poly:     str = "poly"; break;
        }

        return str;
      }
    } // namespace detail
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
