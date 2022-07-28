#ifndef DEVILS_SCRIPT_TEMPLATES_H
#define DEVILS_SCRIPT_TEMPLATES_H

#include <type_traits>
#include "type_traits.h"
#include "interface.h"
#include "context.h"
#include "forward_list.h"

// может быть реакт нужно делать раньше чем непосредственный вызов функции? почему?
// обычно эффект приводит к изменениям, а нам нужен объект еще ДО изменений...
// или нет? хороший вопрос когда делать callback

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    inline void on_effect(const std::string_view, const object, const std::initializer_list<object> &, void*) {}
    using on_effect_func_t = decltype(&on_effect);

    enum iterator_continue_status : uint8_t {
      function_continue,
      function_early_exit,
      function_ignore
    };

    template <typename Th, const char* name>
    class type_checker final : public interface {
    public:
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    template <typename T, size_t U>
    class span_container final : public interface {
    public:
      span_container(const std::span<T, U> view) noexcept;
      struct object process(context*) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::span<T, U> view;
    };

    template <typename T>
    class general_container final : public interface {
    public:
      general_container(const T obj) noexcept;
      struct object process(context*) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      T obj;
    };

    template <typename F, F f, const char* name, on_effect_func_t react = nullptr>
    class basic_function_no_arg final : public interface {
    public:
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    template <typename F, F f, const char* name, on_effect_func_t react = nullptr>
    class basic_function_scripted_arg final : public interface {
    public:
      basic_function_scripted_arg(const interface* arg) noexcept;
      ~basic_function_scripted_arg() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* arg;
    };

    template <typename F, F f, const char* name, on_effect_func_t react = nullptr>
    class basic_function_scripted_args final : public interface {
    public:
      basic_function_scripted_args(const interface* args) noexcept;
      ~basic_function_scripted_args() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* args;
    };

    template <typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    class basic_function_scripted_named_args final : public interface {
    public:
      basic_function_scripted_named_args(const interface* args) noexcept;
      ~basic_function_scripted_named_args() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* args;
    };

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react = nullptr>
    class scripted_function_no_arg final : public interface {
    public:
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react = nullptr>
    class scripted_function_one_arg final : public interface {
    public:
      scripted_function_one_arg(const interface* val) noexcept;
      ~scripted_function_one_arg() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* val;
    };

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t... args>
    class scripted_function_const_args final : public interface {
    public:
      static_assert(std::is_invocable_v<F, Th, decltype(args)...>);
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    };

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    class scripted_function_args final : public interface {
    public:
      static_assert(std::is_invocable_v<F, Th, const Args &...>);
      scripted_function_args(Args&&... args) noexcept;
      explicit scripted_function_args(const std::tuple<Args...> &args) noexcept;
      ~scripted_function_args() noexcept;
      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      std::tuple<Args...> args;
    };

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react = nullptr>
    class scripted_function_scripted_args final : public interface {
    public:
      scripted_function_scripted_args(const interface* args) noexcept;
      ~scripted_function_scripted_args() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* args;
    };

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    class scripted_function_scripted_named_args final : public interface {
    public:
      scripted_function_scripted_named_args(const interface* args) noexcept;
      ~scripted_function_scripted_named_args() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* args;
    };

    template <typename T, typename F, F f, const char* name>
    class scripted_iterator final : public interface {
    public:
      scripted_iterator(const interface* childs) noexcept;
      ~scripted_iterator() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* childs;
    };

    template <typename Th, typename F, F f, const char* name>
    class scripted_iterator_every_numeric final : public interface {
    public:
      scripted_iterator_every_numeric(const interface* condition, const interface* childs) noexcept;
      ~scripted_iterator_every_numeric() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* condition;
      const interface* childs;
    };

    template <typename Th, typename F, F f, const char* name>
    class scripted_iterator_every_effect final : public interface {
    public:
      scripted_iterator_every_effect(const interface* condition, const interface* childs) noexcept;
      ~scripted_iterator_every_effect() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* condition;
      const interface* childs;
    };

    template <typename Th, typename F, F f, const char* name>
    class scripted_iterator_every_logic final : public interface {
    public:
      scripted_iterator_every_logic(const interface* condition, const interface* childs) noexcept;
      ~scripted_iterator_every_logic() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* condition;
      const interface* childs;
    };

    template <typename Th, typename F, F f, const char* name>
    class scripted_iterator_has final : public interface {
    public:
      scripted_iterator_has(const interface* childs) noexcept;
      scripted_iterator_has(const interface* max_count, const interface* percentage, const interface* childs) noexcept;
      ~scripted_iterator_has() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* max_count;
      const interface* percentage;
      const interface* childs;
    };

    template <typename Th, typename F, F f, const char* name>
    class scripted_iterator_random final : public interface {
    public:
      scripted_iterator_random(const size_t &state, const interface* condition, const interface* weight, const interface* childs) noexcept;
      ~scripted_iterator_random() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      size_t state;
      const interface* condition;
      const interface* weight;
      const interface* childs;
    };

    template <typename Th, typename F, F f, const char* name>
    class scripted_iterator_view final : public interface {
    public:
      scripted_iterator_view(const interface* default_value, const interface* childs) noexcept;
      ~scripted_iterator_view() noexcept;

      struct object process(context* ctx) const override;
      local_state* compute(context* ctx, local_state_allocator* allocator) const override;
      //void draw(context* ctx) const override;
      size_t get_type_id() const;
      std::string_view get_name() const;
    private:
      const interface* default_value;
      const interface* childs;
    };

    /* ================================================================================================================================================ */
    // implementation
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    /* ================================================================================================================================================ */

    static_assert(std::is_pointer_v<const int*>);
    static_assert(!std::is_const_v<const int*>);
    static_assert(!std::is_const_v<int const*>);
    static_assert(!std::is_const_v<int*>);
    static_assert(std::is_const_v<const int>);
    static_assert(std::is_const_v<int* const>);

    template <typename T>
    static void add_to_list_func(void* vector_ptr, const object &obj) {
      auto ptr = reinterpret_cast<std::vector<T>*>(vector_ptr);
      ptr->push_back(obj.get<T>());
    }

    template <typename T>
    static object make_obj(T&& arg) {
      if constexpr (std::is_same_v<std::nullopt_t, T>) return object();
      else if constexpr (std::is_enum_v<T>) return object(static_cast<int64_t>(arg));
      else return object(arg);
    }

    template <typename Th, const char* name>
    struct object type_checker<Th, name>::process(context* ctx) const {
      //if constexpr (std::is_pointer_v<Th> && !std::is_const_v<std::remove_pointer_t<Th>>) return object(ctx->current.is<Th>() || ctx->current.is<const Th>());
      if constexpr (std::is_same_v<Th, void>) return object(ctx->current.valid());
      else if constexpr (std::is_same_v<Th, object>) return object(ctx->current.valid() && (!ctx->current.is<bool>() || !ctx->current.is<double>() || !ctx->current.is<std::string_view>()));
      else return object(ctx->current.is<Th>());
    }

    template <typename Th, const char* name>
    local_state* type_checker<Th, name>::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name(), process(ctx));
      ptr->func = [] (const local_state* s) -> object {
        if constexpr (std::is_same_v<Th, void>) return object(s->current.valid());
        else if constexpr (std::is_same_v<Th, object>) return object(s->current.valid() && (!s->current.is<bool>() || !s->current.is<double>() || !s->current.is<std::string_view>()));
        else return object(s->current.is<Th>());
      };
      return ptr;
    }

//     template <typename Th, const char* name>
//     void type_checker<Th, name>::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       dd.value = process(ctx);
//       ctx->draw(&dd);
//     }

    template <typename Th, const char* name>
    size_t type_checker<Th, name>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, const char* name>
    std::string_view type_checker<Th, name>::get_name() const { return name; }

    template <typename T, size_t U>
    span_container<T, U>::span_container(const std::span<T, U> view) noexcept : view(view) {}
    template <typename T, size_t U>
    struct object span_container<T, U>::process(context*) const { return object(view); }
    template <typename T, size_t U>
    local_state* span_container<T, U>::compute(context* ctx, local_state_allocator* allocator) const { return allocator->create(ctx, get_name(), view); }
    template <typename T, size_t U>
    size_t span_container<T, U>::get_type_id() const { return type_id<object>(); }
    template <typename T, size_t U>
    std::string_view span_container<T, U>::get_name() const { return "array_container"; }

    // возможно нужно хранить все таки в формате int64_t
    template <typename T>
    general_container<T>::general_container(const T obj) noexcept : obj(obj) {}
    template <typename T>
    struct object general_container<T>::process(context*) const { return object(obj); }
    template <typename T>
    local_state* general_container<T>::compute(context* ctx, local_state_allocator* allocator) const { return allocator->create(ctx, get_name(), obj); }
    //void draw(context* ctx) const override;
    template <typename T>
    size_t general_container<T>::get_type_id() const { return type_id<object>(); }
    template <typename T>
    std::string_view general_container<T>::get_name() const { return "general_container"; }

    template <typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t curr, typename... Args>
    static object call_func(context* ctx, const interface* current, Args&&... args);

    template <typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename T, typename... Args>
    static object make_list_for_last_arg(context* ctx, const interface* current, forward_list<T>* list_begin, forward_list<T>* list_current, Args&&... args) {
      using cur_type = final_arg_type<F, cur>;
      if constexpr (std::is_pointer_v<cur_type>) {
        if (current == nullptr) return call_func<F, f, name, react, N, cur+1>(ctx, current, std::forward<Args>(args)..., list_begin);
      } else {
        if (current == nullptr) return call_func<F, f, name, react, N, cur+1>(ctx, current, std::forward<Args>(args)..., *list_begin);
      }
      const auto &obj = current->process(ctx);
      if constexpr (is_optional_v<cur_type>) {
        if (!obj.valid()) return call_func<F, f, name, react, N, cur+1>(ctx, current, std::forward<Args>(args)..., std::nullopt);
      }
      forward_list<T> node(obj.template get<T>());
      if (list_current != nullptr) list_current->next = &node;
      return make_list_for_last_arg<F, f, name, react, N, cur, T>(ctx, current->next, list_begin == nullptr ? &node : list_begin, &node, std::forward<Args>(args)...);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t curr, typename... Args>
    static object call_func(context* ctx, Th handle, const interface* current, Args&&... args);

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename T, typename... Args>
    static object make_list_for_last_arg(context* ctx, Th handle, const interface* current, forward_list<T>* list_begin, forward_list<T>* list_current, Args&&... args) {
      using cur_type = final_arg_type<F, cur>;
      if constexpr (std::is_pointer_v<cur_type>) {
        if (current == nullptr) return call_func<Th, F, f, name, react, N, cur+1>(ctx, handle, current, std::forward<Args>(args)..., list_begin);
      } else {
        if (current == nullptr) return call_func<Th, F, f, name, react, N, cur+1>(ctx, handle, current, std::forward<Args>(args)..., *list_begin);
      }
      const auto &obj = current->process(ctx);
      if constexpr (is_optional_v<cur_type>) {
        if (!obj.valid()) return call_func<Th, F, f, name, react, N, cur+1>(ctx, handle, current, std::forward<Args>(args)..., std::nullopt);
      }
      forward_list<T> node(obj.template get<T>());
      if (list_current != nullptr) list_current->next = &node;
      return make_list_for_last_arg<Th, F, f, name, react, N, cur, T>(ctx, handle, current->next, list_begin == nullptr ? &node : list_begin, &node, std::forward<Args>(args)...);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename... Args>
    static object invoke_scripted_function_scripted_args(const local_state* s, const local_state* child, Args&&... args);

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename T, typename... Args>
    static object make_list(const local_state* s, const local_state* current, forward_list<T>* list_begin, forward_list<T>* list_current, Args&&... args) {
      using cur_type = final_arg_type<F, cur>;
      if constexpr (std::is_pointer_v<cur_type>) {
        if (current == nullptr) return invoke_scripted_function_scripted_args<Th, F, f, name, react, N, cur+1>(s, current, std::forward<Args>(args)..., list_begin);
      } else {
        if (current == nullptr) return invoke_scripted_function_scripted_args<Th, F, f, name, react, N, cur+1>(s, current, std::forward<Args>(args)..., *list_begin);
      }
      const auto &obj = current->value;
      if (obj.unresolved()) return unresolved_value;

      if constexpr (is_optional_v<cur_type>) {
        if (!obj.valid()) return invoke_scripted_function_scripted_args<Th, F, f, name, react, N, cur+1>(s, current, std::forward<Args>(args)..., std::nullopt);
      }

      forward_list<T> node(obj.template get<T>());
      if (list_current != nullptr) list_current->next = &node;
      return make_list<Th, F, f, name, react, N, cur, T>(s, current->next, list_begin == nullptr ? &node : list_begin, &node, std::forward<Args>(args)...);
    }

    template <typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename... Args>
    static object invoke_basic_function_scripted_args(const local_state* s, const local_state* child, Args&&... args);

    template <typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename T, typename... Args>
    static object make_list_basic(const local_state* s, const local_state* current, forward_list<T>* list_begin, forward_list<T>* list_current, Args&&... args) {
      using cur_type = final_arg_type<F, cur>;
      if constexpr (std::is_pointer_v<cur_type>) {
        if (current == nullptr) return invoke_basic_function_scripted_args<F, f, name, react, N, cur+1>(s, current, std::forward<Args>(args)..., list_begin);
      } else {
        if (current == nullptr) return invoke_basic_function_scripted_args<F, f, name, react, N, cur+1>(s, current, std::forward<Args>(args)..., *list_begin);
      }
      const auto &obj = current->value;
      if (obj.unresolved()) return unresolved_value;

      if constexpr (is_optional_v<cur_type>) {
        if (!obj.valid()) return invoke_basic_function_scripted_args<F, f, name, react, N, cur+1>(s, current, std::forward<Args>(args)..., std::nullopt);
      }

      forward_list<T> node(obj.template get<T>());
      if (list_current != nullptr) list_current->next = &node;
      return make_list_basic<F, f, name, react, N, cur, T>(s, current->next, list_begin == nullptr ? &node : list_begin, &node, std::forward<Args>(args)...);
    }

    // возможно тут еще имеет смысл составить функцию, при передаче local_state в которую вызовется оригинальная функция с вычисленными аргументами
    // сделал
    // нужно ли запускать реакт? скорее да чем нет
    template <typename F, F f, const char* name, on_effect_func_t react>
    static object invoke_basic_function_no_arg(const local_state* s) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      return invoke_basic_function_scripted_args<F, f, name, react, func_arg_count, 0>(s, s->children);
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    static object invoke_basic_function_scripted_arg(const local_state* s) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      if (s->children == nullptr)
        throw std::runtime_error("State of function '" + std::string(s->function_name) +
                                 "' has not arguments, must be 1");
      //(id: '" + std::string(s->id) + "', method: '" + std::string(s->method_name) + "')"

      return invoke_basic_function_scripted_args<F, f, name, react, func_arg_count, 0>(s, s->children);
    }

    template <typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename... Args>
    static object invoke_basic_function_scripted_args(const local_state* s, const local_state* child, Args&&... args) {
      // вряд ли эта ошибка вообще возникнет
      if (child == nullptr)
        throw std::runtime_error("State of function '" + std::string(s->function_name) +
                                 "' has invalid argument " + std::to_string(cur));
      //" (id: '" + std::string(s->id) + "', method: '" + std::string(s->method_name) + "')"

      using output_type = final_output_type<F>;
      if constexpr (cur == N) {
        object ret;
        if constexpr (std::is_same_v<output_type, void>) std::invoke(f, std::forward<Args>(args)...);
        else {
          auto ret_val = std::invoke(f, std::forward<Args>(args)...);
          ret = is_unresolved(ret_val) ? unresolved_value : object(ret_val);
        }
        if constexpr (react != nullptr) { react(name, s->current, { make_obj(std::forward<Args>())... }, s->user_data); }
        return ret;
      } else {
        using cur_type = final_arg_type<F, cur>;
        if (child->value.unresolved()) return unresolved_value;

        if constexpr (is_forward_list_v<cur_type> || is_forward_list_v<optional_type<cur_type>>) {
          using final_arg = typename std::conditional_t<is_optional_v<cur_type>, forward_list_type<optional_type<cur_type>>, forward_list_type<cur_type>>;
          static_assert(!is_forward_list_v<final_arg>);
          static_assert(cur + 1 == N);
          // если как то пометить конец листа, то можно и в середину засунуть, но как пометить непонятно
          return make_list_basic<F, f, name, react, N, cur, final_arg>(s, child, nullptr, nullptr, std::forward<Args>(args)...);
        } else
        {
          using final_type = typename std::conditional_t<is_optional_v<cur_type>, optional_type<cur_type>, cur_type>;
          const auto &obj = child->value;
          if constexpr (is_optional_v<final_type>) {
            if (!obj.valid()) return invoke_basic_function_scripted_args<F, f, name, react, N, cur+1>(s, child->next, std::forward<Args>(args)..., std::nullopt);
          }

          if constexpr (std::is_enum_v<final_type>) {
            auto arg = obj.get<int64_t>();
            return invoke_basic_function_scripted_args<F, f, name, react, N, cur+1>(s, child->next, std::forward<Args>(args)..., static_cast<final_type>(arg));
          } else {
            auto arg = obj.template get<final_type>();
            return invoke_basic_function_scripted_args<F, f, name, react, N, cur+1>(s, child->next, std::forward<Args>(args)..., arg);
          }
        }
      }
      return object();
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    static object invoke_basic_function_scripted_args(const local_state* s) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();

      if (func_arg_count > 0 && s->children == nullptr)
        throw std::runtime_error("State of function '" + std::string(s->function_name) +
                                 "' has not arguments, must be " + std::to_string(func_arg_count));
      //" (id: '" + std::string(s->id) + "', method: '" + std::string(s->method_name) + "')"

      return invoke_basic_function_scripted_args<F, f, name, react, func_arg_count, 0>(s, s->children);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    static object invoke_scripted_function_no_arg(const local_state* s) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      if (s->current.unresolved()) return unresolved_value;
      return invoke_scripted_function_scripted_args<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(s, s->children);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    static object invoke_scripted_function_scripted_arg(const local_state* s) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      if (s->children == nullptr)
        throw std::runtime_error("State of function '" + std::string(s->function_name) +
                                 "' has not arguments, must be 1");
      //(id: '" + std::string(s->id) + "', method: '" + std::string(s->method_name) + "')"

      if (s->current.unresolved()) return unresolved_value;
      return invoke_scripted_function_scripted_args<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(s, s->children);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t... args>
    static object invoke_scripted_function_const_args(const local_state* s) {
      using output_type = final_output_type<F>;
      if (s->current.unresolved()) return unresolved_value;

      object ret;
      if constexpr (std::is_same_v<output_type, void>) std::invoke(f, s->current.get<Th>(), args...);
      else {
        auto ret_val = std::invoke(f, s->current.get<Th>(), args...);
        ret = is_unresolved(ret_val) ? unresolved_value : object(ret_val);
      }
      if constexpr (react != nullptr) { react(name, s->current, { object(double(args))... }, s->user_data); }
      return ret;
    }

    template <typename F, typename Th, typename Tuple, size_t... I>
    static auto apply_tuple_impl(F f, Th cur, const Tuple &t, std::index_sequence<I...>) -> final_output_type<F> {
      return std::invoke(f, cur, std::get<I>(t)...);
    }

    template <size_t N, typename F, typename Th, typename Tuple, typename Indices = std::make_index_sequence<N>>
    static auto apply_tuple(F f, Th cur, const Tuple &t) -> final_output_type<F> {
      return apply_tuple_impl(f, cur, t, Indices{});
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    static object invoke_scripted_function_args(const local_state* s, const std::tuple<Args...> &args) {
      using output_type = final_output_type<F>;
      if (s->current.unresolved()) return unresolved_value;

      object ret;
      if constexpr (std::is_same_v<output_type, void>) apply_tuple<std::tuple_size_v<std::tuple<Args...>>, F, Th, decltype(args)>(f, s->current.get<Th>(), args);
      else {
        auto ret_val = apply_tuple<std::tuple_size_v<std::tuple<Args...>>, F, Th, decltype(args)>(f, s->current.get<Th>(), args);
        ret = is_unresolved(ret_val) ? unresolved_value : object(ret_val);
      }
      if constexpr (react != nullptr) { react(name, s->current, {}, s->user_data); }
      return ret;
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename... Args>
    static object invoke_scripted_function_scripted_args(const local_state* s, const local_state* child, Args&&... args) {
      if (child == nullptr)
        throw std::runtime_error("State of function '" + std::string(s->function_name) +
                                 "' has invalid argument " + std::to_string(cur));
      //" (id: '" + std::string(s->id) + "', method: '" + std::string(s->method_name) + "')"

      using output_type = final_output_type<F>;
      if constexpr (cur == N) {
        object ret;
        if constexpr (std::is_same_v<output_type, void>) std::invoke(f, s->current.get<Th>(), std::forward<Args>(args)...);
        else {
          auto ret_val = std::invoke(f, s->current.get<Th>(), std::forward<Args>(args)...);
          ret = is_unresolved(ret_val) ? unresolved_value : object(ret_val);
        }
        if constexpr (react != nullptr) { react(name, s->current, { make_obj(std::forward<Args>())... }, s->user_data); }
        return ret;
      } else {
        using cur_type = final_arg_type<F, cur>;
        if (child->value.unresolved()) return unresolved_value;

        if constexpr (is_forward_list_v<cur_type> || is_forward_list_v<optional_type<cur_type>>) {
          using final_arg = typename std::conditional_t<is_optional_v<cur_type>, forward_list_type<optional_type<cur_type>>, forward_list_type<cur_type>>;
          static_assert(!is_forward_list_v<final_arg>);
          static_assert(cur + 1 == N);
          // если как то пометить конец листа, то можно и в середину засунуть, но как пометить непонятно
          return make_list<Th, F, f, name, react, N, cur, final_arg>(s, child, nullptr, nullptr, std::forward<Args>(args)...);
        } else
        {
          using final_type = typename std::conditional_t<is_optional_v<cur_type>, optional_type<cur_type>, cur_type>;
          const auto &obj = child->value;
          if constexpr (is_optional_v<final_type>) {
            if (!obj.valid()) return invoke_scripted_function_scripted_args<Th, F, f, name, react, N, cur+1>(s, child->next, std::forward<Args>(args)..., std::nullopt);
          }

          if constexpr (std::is_enum_v<final_type>) {
            auto arg = obj.get<int64_t>();
            return invoke_scripted_function_scripted_args<Th, F, f, name, react, N, cur+1>(s, child->next, std::forward<Args>(args)..., static_cast<final_type>(arg));
          } else {
            auto arg = obj.template get<final_type>();
            return invoke_scripted_function_scripted_args<Th, F, f, name, react, N, cur+1>(s, child->next, std::forward<Args>(args)..., arg);
          }
        }
      }
      return object();
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    inline object invoke_scripted_function_scripted_args(const local_state* s) {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      if ((func_arg_count - is_member_v<Th, F> > 0) && s->children == nullptr)
        throw std::runtime_error("State of function '" + std::string(s->function_name) +
                                 "' has not arguments, must be " + std::to_string(func_arg_count));
      //" (id: '" + std::string(s->id) + "', method: '" + std::string(s->method_name) + "')"

      if (s->current.unresolved()) return unresolved_value;
      return invoke_scripted_function_scripted_args<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(s, s->children);
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    struct object basic_function_no_arg<F, f, name, react>::process(context* ctx) const {
//       using output_type = final_output_type<F>;
//       object ret;
//       if constexpr (std::is_same_v<output_type, void>) std::invoke(f);
//       else ret = object(std::invoke(f));
//       if constexpr (react != nullptr) { if (!ctx->draw_state()) { react(get_name(), ctx->current, {}, ctx->user_data); } }
//       return ret;
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      return call_func<F, f, name, react, func_arg_count, 0>(ctx, nullptr);
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    local_state* basic_function_no_arg<F, f, name, react>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;
      auto ptr = allocator->create(ctx, get_name());
      ptr->func = &invoke_basic_function_no_arg<F, f, name, react>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_basic_function_no_arg<F, f, name, nullptr>(ptr);
      return ptr;
    }

//     template <typename F, F f, const char* name, on_effect_func_t react>
//     void basic_function_no_arg<F, f, name, react>::draw(context* ctx) const {
//       using output_type = final_output_type<F>;
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       if constexpr (!std::is_same_v<output_type, void>) dd.value = process(ctx);
//       ctx->draw(&dd);
//     }

    template <typename F, F f, const char* name, on_effect_func_t react>
    size_t basic_function_no_arg<F, f, name, react>::get_type_id() const { return type_id<object>(); }
    template <typename F, F f, const char* name, on_effect_func_t react>
    std::string_view basic_function_no_arg<F, f, name, react>::get_name() const { return name; }

    template <typename F, F f, const char* name, on_effect_func_t react>
    basic_function_scripted_arg<F, f, name, react>::basic_function_scripted_arg(const interface* arg) noexcept : arg(arg) {}
    template <typename F, F f, const char* name, on_effect_func_t react>
    basic_function_scripted_arg<F, f, name, react>::~basic_function_scripted_arg() noexcept { arg->~interface(); }

    template <typename F, F f, const char* name, on_effect_func_t react>
    struct object basic_function_scripted_arg<F, f, name, react>::process(context* ctx) const {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      return call_func<F, f, name, react, func_arg_count, 0>(ctx, arg);
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    local_state* basic_function_scripted_arg<F, f, name, react>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;
      auto ptr = allocator->create(ctx, get_name());

      local_state* arg_state = nullptr;
      {
        change_function_name cfn(ctx, name);
        change_nesting cn(ctx, ctx->nest_level+1);
        arg_state = arg->compute(ctx, allocator);
      }
      ptr->children = arg_state;
      ptr->func = &invoke_basic_function_scripted_arg<F, f, name, react>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_basic_function_scripted_arg<F, f, name, nullptr>(ptr);
      return ptr;
    }

//     template <typename F, F f, const char* name, on_effect_func_t react>
//     void basic_function_scripted_arg<F, f, name, react>::draw(context* ctx) const {
//       using input_type = final_arg_type<F, 0>;
//       using output_type = final_output_type<F>;
//
//       {
//         local_state dd(ctx);
//         dd.function_name = get_name();
//         dd.original = arg->process(ctx);
//         if constexpr (!std::is_same_v<output_type, void>) dd.value = object(std::invoke(f, dd.original.get<input_type>()));
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, name);
//       change_nesting cn(ctx, ctx->nest_level+1);
//       arg->draw(ctx);
//     }

    template <typename F, F f, const char* name, on_effect_func_t react>
    size_t basic_function_scripted_arg<F, f, name, react>::get_type_id() const { return type_id<object>(); }
    template <typename F, F f, const char* name, on_effect_func_t react>
    std::string_view basic_function_scripted_arg<F, f, name, react>::get_name() const { return name; }

    template <typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename... Args>
    static object call_func(context* ctx, const interface* current, Args&&... args) {
      using output_type = final_output_type<F>;
      if constexpr (cur == N) {
        object ret;
        if constexpr (std::is_same_v<output_type, void>) std::invoke(f, std::forward<Args>(args)...);
        else ret = object(std::invoke(f, std::forward<Args>(args)...));
        if constexpr (react != nullptr) { if (!ctx->draw_state()) { react(name, ctx->current, { make_obj(std::forward<Args>(args))... }, ctx->user_data); } }
        return ret;
      } else {
        using cur_type = final_arg_type<F, cur>;
        // как то так выглядит код для входного array_view, в чем проблема? проблема в change_vector_and_func, передаем указатели на void что неудачно
        // + довольно переусложнен код вызова следующей функции, а, и нам еще придется где то хранить std::vector<final_type> в функции compute
        // что же с тобой делать? вообще в функцию не передавать array_view, точнее передавать только array_view с object, а его брать либо из уже
        // вычисленных контейнеров либо из листов, видимо это единственный более менее разумный вариант
        // еще пара вариантов таких: а) мы запоминаем здесь функцию с несколькими std::optional'ами в конце и работаем с ними как с массивом
        // б) мы создаем небольшой лист прямо тут, примерно такой же как и interface*, и пока не кончатся интерфейсы продолжаем создавать локальные ноды для листов
        // в) мы пытаемся что то придумать с векторами
        // самый последний) объект может хранить векторы и другие контейнеры (наверное можно сделать наследование для объекта с серьезными типами, которые будут занимать отведенную память)
        // короч я посмотрел и в цк3 скорее всего функций принимающих на вход составной аргумент (массив) не так много (вот я один start_war нашел)
        // в start_war титулы указываются просто перечислением target_title = W1 target_title = W2 ...
        // похоже что легко можно создать forward_list для таких вот входных данных и расположить его последним аргументом
//         if constexpr (utils::is_array_view_v<optional_type<cur_type>> || utils::is_array_view_v<cur_type>) {
//           using final_type = typename std::conditional_t<is_optional_v<cur_type>, utils::array_view_type<optional_type<cur_type>>, utils::array_view_type<cur_type>>::type;
//           static_assert(!is_optional_v<final_type>, "Do not use array_view with optionals");
//           std::vector<final_type> container;
//           // что тут? мы должны положить в контекст массив и функцию
//           change_vector_and_func cvaf(ctx, &container, &add_to_list_func<final_type>);
//           const auto &obj = current->process(ctx);
//           if constexpr (is_optional_v<cur_type>) {
//             if (obj.valid()) {
//               auto arg = obj.template get<final_type>();
//               ret = call_func<F, f, name, react, N, curr+1>(ctx, current->next, std::forward<Args>(args)..., arg);
//             } else {
//               ret = call_func<F, f, name, react, N, curr+1>(ctx, current->next, std::forward<Args>(args)..., std::nullopt);
//             }
//           } else {
//             auto arg = obj.template get<final_type>();
//             ret = call_func<F, f, name, react, N, curr+1>(ctx, current->next, std::forward<Args>(args)..., arg);
//           }
//         } else
        // может ли лист быть оптионалом? наверное может
        if constexpr (is_forward_list_v<cur_type> || is_forward_list_v<optional_type<cur_type>>) {
          using final_arg = typename std::conditional_t<is_optional_v<cur_type>, forward_list_type<optional_type<cur_type>>, forward_list_type<cur_type>>;
          static_assert(!is_forward_list_v<final_arg>);
          static_assert(cur + 1 == N);
          // это последний аргумент, нужно вызвать функцию которая создаст ноды листа рекурсивно
          // так можно составить и array_view если создать тут статический массив на 128 например
          // тогда будет меняться семантика аргумента от положения в функции, чего лучше избегать
          // лучше просто сделать хвостовую рекурсию
          return make_list_for_last_arg<F, f, name, react, N, cur, final_arg>(ctx, current, nullptr, nullptr, std::forward<Args>(args)...);
        } else
        {
          using final_type = typename std::conditional_t<is_optional_v<cur_type>, optional_type<cur_type>, cur_type>;
          const auto &obj = current->process(ctx);
          if constexpr (is_optional_v<final_type>) {
            if (!obj.valid()) return call_func<F, f, name, react, N, cur+1>(ctx, current->next, std::forward<Args>(args)..., std::nullopt);
          }

          if constexpr (std::is_enum_v<final_type>) {
            auto arg = obj.get<int64_t>();
            return call_func<F, f, name, react, N, cur+1>(ctx, current->next, std::forward<Args>(args)..., static_cast<final_type>(arg));
          } else {
            auto arg = obj.template get<final_type>();
            return call_func<F, f, name, react, N, cur+1>(ctx, current->next, std::forward<Args>(args)..., arg);
          }
        }
      }
      return object();
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    basic_function_scripted_args<F, f, name, react>::basic_function_scripted_args(const interface* args) noexcept : args(args) {}
    template <typename F, F f, const char* name, on_effect_func_t react>
    basic_function_scripted_args<F, f, name, react>::~basic_function_scripted_args() noexcept {
      for (auto cur = args; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    struct object basic_function_scripted_args<F, f, name, react>::process(context* ctx) const {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      return call_func<F, f, name, react, func_arg_count, 0>(ctx, args);
    }

    template <typename F, F f, const char* name, on_effect_func_t react>
    local_state* basic_function_scripted_args<F, f, name, react>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;

      auto ptr = allocator->create(ctx, get_name());

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      {
        auto cur_arg = args;
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        for (size_t i = 0; i < get_function_argument_count<F>(); ++i) {
          auto child = cur_arg->compute(ctx, allocator);
          if (first_child == nullptr) first_child = child;
          if (children != nullptr) children->next = child;
          children = child;
          cur_arg = cur_arg->next;
        }
      }

      ptr->children = first_child;
      ptr->func = &invoke_basic_function_scripted_args<F, f, name, react>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_basic_function_scripted_args<F, f, name, nullptr>(ptr);
      return ptr;
    }

//     template <typename F, F f, const char* name, on_effect_func_t react>
//     void basic_function_scripted_args<F, f, name, react>::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = name;
//         using output_type = final_output_type<F>;
//         if constexpr (!std::is_same_v<output_type, void>) dd.value = process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, name);
//       change_nesting cn(ctx, ctx->nest_level+1);
//       for (auto cur = args; cur != nullptr; cur = cur->next) { cur->draw(ctx); }
//     }

    template <typename F, F f, const char* name, on_effect_func_t react>
    size_t basic_function_scripted_args<F, f, name, react>::get_type_id() const { return type_id<object>(); }
    template <typename F, F f, const char* name, on_effect_func_t react>
    std::string_view basic_function_scripted_args<F, f, name, react>::get_name() const { return name; }

    template <typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    basic_function_scripted_named_args<F, f, name, react, first_name, args_names...>::basic_function_scripted_named_args(const interface* args) noexcept : args(args) {}
    template <typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    basic_function_scripted_named_args<F, f, name, react, first_name, args_names...>::~basic_function_scripted_named_args() noexcept {
      for (auto cur = args; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    struct object basic_function_scripted_named_args<F, f, name, react, first_name, args_names...>::process(context* ctx) const {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      return call_func<F, f, name, react, func_arg_count, 0>(ctx, args);
    }

    template <const char* first_name>
    void setup_args_names(local_state* cur) {
      cur->argument_name = first_name;
    }

    template <const char* first_name, const char* next_name, const char*... args_names>
    void setup_args_names(local_state* cur) {
      cur->argument_name = first_name;
      setup_args_names<next_name, args_names...>(cur->next);
    }

    template <typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    local_state* basic_function_scripted_named_args<F, f, name, react, first_name, args_names...>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;

      auto ptr = allocator->create(ctx, get_name());

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      {
        auto cur_arg = args;
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        for (size_t i = 0; i < get_function_argument_count<F>(); ++i) {
          auto child = cur_arg->compute(ctx, allocator);
          if (first_child == nullptr) first_child = child;
          if (children != nullptr) children->next = child;
          children = child;
          cur_arg = cur_arg->next;
        }

        setup_args_names<first_name, args_names...>(first_child);
      }

      ptr->children = first_child;
      ptr->func = &invoke_basic_function_scripted_args<F, f, name, react>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_basic_function_scripted_args<F, f, name, nullptr>(ptr);
      return ptr;
    }

    //template <typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    //void draw(context* ctx) const override;
    template <typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    size_t basic_function_scripted_named_args<F, f, name, react, first_name, args_names...>::get_type_id() const { return type_id<object>(); }
    template <typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    std::string_view basic_function_scripted_named_args<F, f, name, react, first_name, args_names...>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    struct object scripted_function_no_arg<Th, F, f, name, react>::process(context* ctx) const {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      auto cur = ctx->current.get<Th>();
      return call_func<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(ctx, cur, nullptr);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    local_state* scripted_function_no_arg<Th, F, f, name, react>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;
      auto ptr = allocator->create(ctx, get_name());
      ptr->func = &invoke_scripted_function_no_arg<Th, F, f, name, react>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_scripted_function_no_arg<Th, F, f, name, nullptr>(ptr);
      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
//     void scripted_function_no_arg<Th, F, f, name, react>::draw(context* ctx) const {
//       using output_type = final_output_type<F>;
//       local_state dd(ctx);
//       dd.function_name = get_name();
//       if constexpr (!std::is_same_v<output_type, void>) dd.value = process(ctx);
//       ctx->draw(&dd);
//     }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    size_t scripted_function_no_arg<Th, F, f, name, react>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    std::string_view scripted_function_no_arg<Th, F, f, name, react>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    scripted_function_one_arg<Th, F, f, name, react>::scripted_function_one_arg(const interface* val) noexcept : val(val) {}
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    scripted_function_one_arg<Th, F, f, name, react>::~scripted_function_one_arg() noexcept { val->~interface(); }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    struct object scripted_function_one_arg<Th, F, f, name, react>::process(context* ctx) const {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      auto cur = ctx->current.get<Th>();
      return call_func<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(ctx, cur, val);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    local_state* scripted_function_one_arg<Th, F, f, name, react>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;
      auto ptr = allocator->create(ctx, get_name());

      local_state* arg_state = nullptr;
      {
        change_function_name cfn(ctx, name);
        change_nesting cn(ctx, ctx->nest_level+1);
        arg_state = val->compute(ctx, allocator);
      }

      ptr->children = arg_state;
      ptr->func = &invoke_scripted_function_scripted_arg<Th, F, f, name, react>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_scripted_function_scripted_arg<Th, F, f, name, nullptr>(ptr);
      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
//     void scripted_function_one_arg<Th, F, f, name, react>::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = name;
//         using output_type = final_output_type<F>;
//         if constexpr (!std::is_same_v<output_type, void>) dd.value = process(ctx);
//         dd.original = val->process(ctx);
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, name);
//       change_nesting cn(ctx, ctx->nest_level+1);
//       val->draw(ctx);
//     }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    size_t scripted_function_one_arg<Th, F, f, name, react>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    std::string_view scripted_function_one_arg<Th, F, f, name, react>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t... args>
    struct object scripted_function_const_args<Th, F, f, name, react, args...>::process(context* ctx) const {
      using output_type = final_output_type<F>;
      auto cur = ctx->current.get<Th>();
      object ret;
      if constexpr (!std::is_same_v<output_type, void>) ret = object(std::invoke(f, cur, args...));
      else std::invoke(f, cur, args...);
      // нужно ли добавить константные аргументы? почему бы и нет
      if constexpr (react != nullptr) { if (!ctx->draw_state()) { react(get_name(), ctx->current, { object(double(args))... }, ctx->user_data); } }
      return ret;
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t... args>
    local_state* scripted_function_const_args<Th, F, f, name, react, args...>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;
      auto ptr = allocator->create(ctx, get_name());
      ptr->func = &invoke_scripted_function_const_args<Th, F, f, name, react, args...>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_scripted_function_const_args<Th, F, f, name, nullptr, args...>(ptr);
      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t... args>
//     void scripted_function_const_args<Th, F, f, name, react, args...>::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = name;
//       using output_type = final_output_type<F>;
//       if constexpr (!std::is_same_v<output_type, void>) dd.value = process(ctx);
//       ctx->draw(&dd);
//     }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t... args>
    size_t scripted_function_const_args<Th, F, f, name, react, args...>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, int64_t... args>
    std::string_view scripted_function_const_args<Th, F, f, name, react, args...>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    scripted_function_args<Th, F, f, name, react, Args...>::scripted_function_args(Args&&... args) noexcept : args(std::forward<Args>(args)...) {}
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    scripted_function_args<Th, F, f, name, react, Args...>::scripted_function_args(const std::tuple<Args...> &args) noexcept : args(args) {}
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    scripted_function_args<Th, F, f, name, react, Args...>::~scripted_function_args() noexcept {} // ничего?
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    struct object scripted_function_args<Th, F, f, name, react, Args...>::process(context* ctx) const {
      auto cur = ctx->current.get<Th>();
      using output_type = final_output_type<F>;
      object ret;
      if constexpr (std::is_same_v<output_type, void>) apply_tuple<std::tuple_size_v<decltype(args)>>(f, cur, args);
      else ret = object(apply_tuple<std::tuple_size_v<decltype(args)>>(f, cur, args));
      // тут добавить аргументы не выйдет
      if constexpr (react != nullptr) { if (!ctx->draw_state()) { react(get_name(), ctx->current, {}, ctx->user_data); } }
      return ret;
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    local_state* scripted_function_args<Th, F, f, name, react, Args...>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;
      auto ptr = allocator->create(ctx, get_name());
      ptr->func = std::bind(&invoke_scripted_function_args<Th, F, f, name, react, Args...>, std::placeholders::_1, args);
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_scripted_function_args<Th, F, f, name, react, Args...>(ptr, args);
      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
//     void scripted_function_args<Th, F, f, name, react, Args...>::draw(context* ctx) const {
//       local_state dd(ctx);
//       dd.function_name = name;
//       using output_type = final_output_type<F>;
//       if constexpr (!std::is_same_v<output_type, void>) dd.value = process(ctx);
//       ctx->draw(&dd);
//     }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    size_t scripted_function_args<Th, F, f, name, react, Args...>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, typename... Args>
    std::string_view scripted_function_args<Th, F, f, name, react, Args...>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    scripted_function_scripted_args<Th, F, f, name, react>::scripted_function_scripted_args(const interface* args) noexcept : args(args) {}
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    scripted_function_scripted_args<Th, F, f, name, react>::~scripted_function_scripted_args() noexcept {
      for (auto cur = args; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, size_t N, size_t cur, typename... Args>
    static object call_func(context* ctx, Th handle, const interface* current, Args&&... args) {
      using output_type = final_output_type<F>;
      if constexpr (cur == N) {
        object ret;
        if constexpr (std::is_same_v<output_type, void>) std::invoke(f, handle, std::forward<Args>(args)...);
        else ret = object(std::invoke(f, handle, std::forward<Args>(args)...));
        // или перед непосредственным вызовом функций, есть свои плюсы и минусы, в цк2 обычно реакции вызываются ПОСЛЕ функций
        // есть только on_death, который вызывается непосредственно перед, наверное там просто сделан отложенный эффект
        // (то есть функция смерти ничего не делает, только вызывает реакцию, а в ней уже умирает персонаж)
        if constexpr (react != nullptr) { if (!ctx->draw_state()) { react(name, ctx->current, { make_obj(std::forward<Args>(args))... }, ctx->user_data); } }
        return ret;
      } else {
        using cur_type = final_arg_type<F, cur>;
        if constexpr (is_forward_list_v<cur_type> || is_forward_list_v<optional_type<cur_type>>) {
          using final_arg = typename std::conditional_t<is_optional_v<cur_type>, forward_list_type<optional_type<cur_type>>, forward_list_type<cur_type>>;
          static_assert(!is_forward_list_v<final_arg>);
          static_assert(cur + 1 == N);
          return make_list_for_last_arg<Th, F, f, name, react, N, cur, final_arg>(ctx, handle, current, nullptr, nullptr, std::forward<Args>(args)...);
        } else
        {
          using final_type = typename std::conditional_t<is_optional_v<cur_type>, optional_type<cur_type>, cur_type>;
          const auto &obj = current->process(ctx);
          if constexpr (is_optional_v<final_type>) {
            if (!obj.valid()) return call_func<Th, F, f, name, react, N, cur+1>(ctx, handle, current->next, std::forward<Args>(args)..., std::nullopt);
          }

          if constexpr (std::is_enum_v<final_type>) {
            auto arg = obj.get<int64_t>();
            return call_func<Th, F, f, name, react, N, cur+1>(ctx, handle, current->next, std::forward<Args>(args)..., static_cast<final_type>(arg));
          } else {
            auto arg = obj.template get<final_type>();
            return call_func<Th, F, f, name, react, N, cur+1>(ctx, handle, current->next, std::forward<Args>(args)..., arg);
          }
        }
      }
      return object();
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    struct object scripted_function_scripted_args<Th, F, f, name, react>::process(context* ctx) const {
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      auto cur = ctx->current.get<Th>();
      return call_func<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(ctx, cur, args);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    local_state* scripted_function_scripted_args<Th, F, f, name, react>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;
//       using input_type = final_arg_type<F, 0>;
      auto ptr = allocator->create(ctx, get_name());

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      {
        auto cur_arg = args;
        change_function_name cfn(ctx, name);
        change_nesting cn(ctx, ctx->nest_level+1);
        for (size_t i = 0; i < get_function_argument_count<F>(); ++i) {
          auto child = cur_arg->compute(ctx, allocator);
          if (first_child == nullptr) first_child = child;
          if (children != nullptr) children->next = child;
          children = child;
          cur_arg = cur_arg->next;
        }
      }

      ptr->children = first_child;
      ptr->func = &invoke_scripted_function_scripted_args<Th, F, f, name, react>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_scripted_function_scripted_args<Th, F, f, name, nullptr>(ptr);
      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
//     void scripted_function_scripted_args<Th, F, f, name, react>::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = name;
//         using output_type = final_output_type<F>;
//         if constexpr (!std::is_same_v<output_type, void>) dd.value = process(ctx);
//         // нужно аргументы разложить
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_function_name cfn(ctx, name);
//       change_nesting cn(ctx, ctx->nest_level+1);
//       for (auto cur = args; cur != nullptr; cur = cur->next) { cur->draw(ctx); }
//     }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    size_t scripted_function_scripted_args<Th, F, f, name, react>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react>
    std::string_view scripted_function_scripted_args<Th, F, f, name, react>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    scripted_function_scripted_named_args<Th, F, f, name, react, first_name, args_names...>::scripted_function_scripted_named_args(const interface* args) noexcept : args(args) {}
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    scripted_function_scripted_named_args<Th, F, f, name, react, first_name, args_names...>::~scripted_function_scripted_named_args() noexcept {
      for (auto cur = args; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    struct object scripted_function_scripted_named_args<Th, F, f, name, react, first_name, args_names...>::process(context* ctx) const {
      auto cur = ctx->current.get<Th>();
      constexpr size_t func_arg_count = get_function_argument_count<F>();
      return call_func<Th, F, f, name, react, func_arg_count, !is_member_v<Th, F>>(ctx, cur, args);
    }

    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    local_state* scripted_function_scripted_named_args<Th, F, f, name, react, first_name, args_names...>::compute(context* ctx, local_state_allocator* allocator) const {
      using output_type = final_output_type<F>;
//       using input_type = final_arg_type<F, 0>;
      auto ptr = allocator->create(ctx, get_name());

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      {
        auto cur_arg = args;
        change_function_name cfn(ctx, name);
        change_nesting cn(ctx, ctx->nest_level+1);
        for (size_t i = 0; i < get_function_argument_count<F>(); ++i) {
          auto child = cur_arg->compute(ctx, allocator);
          if (first_child == nullptr) first_child = child;
          if (children != nullptr) children->next = child;
          children = child;
          cur_arg = cur_arg->next;
        }

        setup_args_names<first_name, args_names...>(first_child);
      }

      ptr->children = first_child;
      ptr->func = &invoke_scripted_function_scripted_args<Th, F, f, name, react>;
      if constexpr (!std::is_same_v<output_type, void>) ptr->value = invoke_scripted_function_scripted_args<Th, F, f, name, nullptr>(ptr);
      return ptr;
    }

    //template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    //void draw(context* ctx) const override;
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    size_t scripted_function_scripted_named_args<Th, F, f, name, react, first_name, args_names...>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name, on_effect_func_t react, const char* first_name, const char*... args_names>
    std::string_view scripted_function_scripted_named_args<Th, F, f, name, react, first_name, args_names...>::get_name() const { return name; }

    template <typename T, typename F, F f, const char* name>
    scripted_iterator<T, F, f, name>::scripted_iterator(const interface* childs) noexcept : childs(childs) {}
    template <typename T, typename F, F f, const char* name>
    scripted_iterator<T, F, f, name>::~scripted_iterator() noexcept {
      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    // static std::tuple<object, const interface*> first_not_ignore(context* ctx, const interface* childs) {
    //   object obj = ignore_value;
    //   auto cur = childs;
    //   for (; cur != nullptr && obj.ignore(); cur = cur->next) {
    //     obj = cur->process(ctx);
    //   }
    //   return std::make_tuple(obj, cur);
    // }

    template <typename T, typename F, F f, const char* name>
    struct object scripted_iterator<T, F, f, name>::process(context* ctx) const {
      //static_assert(std::is_invocable_v<F, object, object>);
      //static_assert(std::is_same_v< std::invoke_result_t<F, object, object>, std::tuple<object, bool> >);
      //using ret_t = std::conditional_t< std::is_same_v< final_arg_type<F, 0>, T>, final_arg_type<F, 1>, final_arg_type<F, 0>>;
      using ret_t = std::tuple_element_t<0, final_output_type<F>>;
      static_assert(std::is_invocable_v<F, T, ret_t, object, size_t> || std::is_invocable_v<F, ret_t, object, size_t>);
      static_assert(std::is_same_v< std::conditional_t<
        std::is_invocable_v <F, T, ret_t, object, size_t>,
        std::invoke_result_t<F, T, ret_t, object, size_t>,
        std::invoke_result_t<F,    ret_t, object, size_t>
      >, std::tuple<ret_t, iterator_continue_status> >);

      // first_not_ignore не нужен, как относится к тем или иным вещам нужно решить со стороны функции
      // может потребоваться счетчик, чтобы инициализировать первое значение
//       auto [current_obj, start] = first_not_ignore(ctx, childs);
//       if (current_obj.ignore()) return ignore_value;
//       auto current = current_obj.get<ret_t>();
      ret_t current{};
      iterator_continue_status continue_func = function_continue;
      size_t counter = 0;
      if constexpr (std::is_invocable_v<F, T, ret_t, object, size_t>) {
        auto ctx_current = ctx->current.get<T>();
        for (auto cur = childs; cur != nullptr && continue_func == function_continue; cur = cur->next, ++counter) {
          const auto &next = cur->process(ctx);
          const auto [ret, cont] = std::invoke(f, ctx_current, current, next, counter);
          current = ret;
          continue_func = cont;
        }
      } else {
        for (auto cur = childs; cur != nullptr && continue_func == function_continue; cur = cur->next, ++counter) {
          const auto &next = cur->process(ctx);
          const auto [ret, cont] = std::invoke(f, current, next, counter);
          current = ret;
          continue_func = cont;
        }
      }

      return continue_func != function_ignore ? object(current) : ignore_value;
    }

    // static std::tuple<object, const local_state*> first_not_ignore(const local_state* childs) {
    //   object obj = ignore_value;
    //   auto cur = childs;
    //   for (; cur != nullptr && obj.ignore() && !obj.unresolved(); cur = cur->next) { obj = cur->value; }
    //   return std::make_tuple(obj, cur);
    // }

    template <typename T, typename F, F f, const char* name>
    local_state* scripted_iterator<T, F, f, name>::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      for (auto cur = childs; cur != nullptr; cur = cur->next) {
        auto ch = cur->compute(ctx, allocator);
        if (first_child == nullptr) first_child = ch;
        if (children != nullptr) children->next = ch;
        children = ch;
      }

      ptr->children = first_child;
      ptr->func = [] (const local_state* s) -> object {
        //using ret_t = std::conditional_t< std::is_same_v< final_arg_type<F, 0>, T>, final_arg_type<F, 1>, final_arg_type<F, 0>>;
        using ret_t = std::tuple_element_t<0, final_output_type<F>>;
//         auto [current_obj, start] = first_not_ignore(s);
//         if (current_obj.ignore()) return ignore_value;
//         if (current_obj.unresolved()) return unresolved_value;
//         auto current = current_obj.get<ret_t>();
        ret_t current{};
        iterator_continue_status continue_func = function_continue;
        size_t counter = 0;
        if constexpr (std::is_invocable_v<F, T, ret_t, object, size_t>) {
          if (s->current.unresolved()) return unresolved_value;
          auto ctx_current = s->current.get<T>();
          for (auto cur = s->children; cur != nullptr && continue_func == function_continue; cur = cur->next, ++counter) {
            const auto &next = cur->value;
            if (next.unresolved()) return unresolved_value;
            const auto [ret, cont] = std::invoke(f, ctx_current, current, next, counter);
            current = ret;
            continue_func = cont;
          }
        } else {
          for (auto cur = s->children; cur != nullptr && continue_func == function_continue; cur = cur->next, ++counter) {
            const auto &next = cur->value;
            if (next.unresolved()) return unresolved_value;
            const auto [ret, cont] = std::invoke(f, current, next, counter);
            current = ret;
            continue_func = cont;
          }
        }

        return continue_func != function_ignore ? object(current) : ignore_value;
      };
      ptr->value = ptr->func(ptr);
      return ptr;
    }

    //void draw(context* ctx) const override;
    template <typename T, typename F, F f, const char* name>
    size_t scripted_iterator<T, F, f, name>::get_type_id() const { return type_id<object>(); }
    template <typename T, typename F, F f, const char* name>
    std::string_view scripted_iterator<T, F, f, name>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_every_numeric<Th, F, f, name>::scripted_iterator_every_numeric(const interface* condition, const interface* childs) noexcept : condition(condition), childs(childs) {}
    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_every_numeric<Th, F, f, name>::~scripted_iterator_every_numeric() noexcept {
      if (condition != nullptr) condition->~interface();
      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename Th, typename F, F f, const char* name>
    static double invoke_every_numeric(context* ctx, Th cur, const interface* condition, const interface* childs) {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      using predicate_arg_type = function_argument_type<predicate_type, 0>;

      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);
      size_t counter = 0;
      double val = 0.0;
      const auto pred_func = [&] (predicate_arg_type arg) -> bool {
        ctx->current = object(arg);
        ctx->index = counter;
        ++counter;

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) return true;
        }

        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          const auto &obj = cur->process(ctx);
          val += obj.ignore() ? 0.0 : obj.get<double>();
        }

        return true;
      };
      std::invoke(f, cur, pred_func);

      return val;
    }

    template <typename F, F f, const char* name>
    static double invoke_every_numeric(context* ctx, const interface* condition, const interface* childs) {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      using predicate_arg_type = function_argument_type<predicate_type, 0>;

      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);
      size_t counter = 0;
      double val = 0.0;
      const auto pred_func = [&] (predicate_arg_type arg) -> bool {
        ctx->current = object(arg);
        ctx->index = counter;
        ++counter;

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) return true;
        }

        for (auto cur = childs; cur != nullptr; cur = cur->next) {
          const auto &obj = cur->process(ctx);
          val += obj.ignore() ? 0.0 : obj.get<double>();
        }

        return true;
      };
      std::invoke(f, pred_func);

      return val;
    }

    template <typename Th, typename F, F f, const char* name>
    struct object scripted_iterator_every_numeric<Th, F, f, name>::process(context* ctx) const {
      // должна быть какая то функция которую мы запустим, а ответ от функции вернем
      // нет, нам скорее нужен итератор, чтобы не писать тонну ненужной фигни для
      // функций типа: сортированный обход, рандом, имеется_ли и проч
      //change_scope cs(ctx, object(), ctx->current);

      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;

      if constexpr (std::is_invocable_v<F, predicate_type>) {
        return invoke_every_numeric<F, f, name>(ctx, condition, childs);
      } else {
        Th cur = ctx->current.get<Th>();
        return invoke_every_numeric<Th, F, f, name>(ctx, cur, condition, childs);
      }
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

    // проблема в том что при малейшем изменении контекста может измениться current, а значит будут взяты другие значения в функции итераторе
    template <typename Th, typename F, F f, const char* name>
    local_state* scripted_iterator_every_numeric<Th, F, f, name>::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      // что делать если ctx->current.unresolved()? нужно в любом случае хотя бы один раз пройтись по всем дочерним функциям
      // в общем то отличие только в том что необязательно проходить тогда по функции
      if (ctx->current.unresolved() && !std::is_same_v<void*, Th>) {
        change_scope cs(ctx, unresolved_value, ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        local_every_numeric_func(ctx, allocator, name, condition, childs, unresolved_value, &first_child, &children);
      } else {
        using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
        using predicate_arg_type = function_argument_type<predicate_type, 0>;

        size_t counter = 0;
        const auto pred_func = [&] (predicate_arg_type arg) -> bool {
          ctx->current = object(arg);
          ctx->index = counter;
          ++counter;
          local_every_numeric_func(ctx, allocator, name, condition, childs, ctx->current, &first_child, &children);
          return true;
        };

        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);

        if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
        else {
          Th cur = ctx->current.get<Th>();
          std::invoke(f, cur, pred_func);
        }
      }

      ptr->children = first_child;
      // теперь тут функция будет выглядеть тип мы просто пересчитали данные из local_state::value
      ptr->value = compute_every_numeric_value(ptr);
      ptr->func = &compute_every_numeric_value;

      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name>
//     void scripted_iterator_every_numeric<Th, F, f, name>::draw(context* ctx) const {
//       const auto val = process(ctx);
//
//       {
//         local_state dd(ctx);
//         dd.function_name = name;
//         dd.value = val;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       Th cur;
//       if constexpr (std::is_same_v<void*, Th>) cur = nullptr;
//       else cur = ctx->current.get<Th>();
//       object first;
//       f(cur, [&] (const object &obj) -> bool {
//         if (condition != nullptr) {
//           const auto obj = condition->process(ctx);
//           if (obj.ignore() || !obj.get<bool>()) return true;
//         }
//
//         first = obj;
//         return false;
//       });
//       if (!first.valid()) return;
//
//       change_scope cs(ctx, first, ctx->current);
//       change_function_name cfn(ctx, name);
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

    template <typename Th, typename F, F f, const char* name>
    size_t scripted_iterator_every_numeric<Th, F, f, name>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name>
    std::string_view scripted_iterator_every_numeric<Th, F, f, name>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_every_effect<Th, F, f, name>::scripted_iterator_every_effect(const interface* condition, const interface* childs) noexcept : condition(condition), childs(childs) {}
    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_every_effect<Th, F, f, name>::~scripted_iterator_every_effect() noexcept {
      if (condition != nullptr) condition->~interface();
      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename Th, typename F, F f, const char* name>
    struct object scripted_iterator_every_effect<Th, F, f, name>::process(context* ctx) const {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      using predicate_arg_type = function_argument_type<predicate_type, 0>;

      size_t counter = 0;
      const auto pred_func = [&] (predicate_arg_type arg) -> bool {
        ctx->current = object(arg);
        ctx->index = counter;
        ++counter;

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) return true;
        }

        for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->process(ctx); }
        return true;
      };

      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);

      if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
      else {
        Th cur = ctx->current.get<Th>();
        std::invoke(f, cur, pred_func);
      }

      return object();
    }

    template <typename Th, typename F, F f, const char* name>
    local_state* scripted_iterator_every_effect<Th, F, f, name>::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      if (ctx->current.unresolved() && !std::is_same_v<void*, Th>) {
        change_scope cs(ctx, unresolved_value, ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        local_every_effect_func(ctx, allocator, name, condition, childs, unresolved_value, &first_child, &children);
      } else {
        using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
        using predicate_arg_type = function_argument_type<predicate_type, 0>;

        size_t counter = 0;
        const auto pred_func = [&] (predicate_arg_type arg) -> bool {
          ctx->current = object(arg);
          ctx->index = counter;
          ++counter;
          local_every_effect_func(ctx, allocator, name, condition, childs, ctx->current, &first_child, &children);
          return true;
        };

        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);

        if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
        else {
          Th cur = ctx->current.get<Th>();
          std::invoke(f, cur, pred_func);
        }
      }

      ptr->children = first_child;
      //ptr->value = compute_every_effect_value(ptr);
      //ptr->func = &compute_every_effect_value;

      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name>
//     void scripted_iterator_every_effect<Th, F, f, name>::draw(context* ctx) const {
//       {
//         local_state dd(ctx);
//         dd.function_name = name;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       Th cur;
//       if constexpr (std::is_same_v<void*, Th>) cur = nullptr;
//       else cur = ctx->current.get<Th>();
//       object first;
//       f(cur, [&] (const object &obj) -> bool {
//         if (condition != nullptr) {
//           const auto obj = condition->process(ctx);
//           if (obj.ignore() || !obj.get<bool>()) return true;
//         }
//
//         first = obj;
//         return false;
//       });
//       if (!first.valid()) return;
//
//       change_scope cs(ctx, first, ctx->current);
//       change_function_name cfn(ctx, name);
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

    template <typename Th, typename F, F f, const char* name>
    size_t scripted_iterator_every_effect<Th, F, f, name>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name>
    std::string_view scripted_iterator_every_effect<Th, F, f, name>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_every_logic<Th, F, f, name>::scripted_iterator_every_logic(const interface* condition, const interface* childs) noexcept : condition(condition), childs(childs) {}
    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_every_logic<Th, F, f, name>::~scripted_iterator_every_logic() noexcept {
      if (condition != nullptr) condition->~interface();
      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename Th, typename F, F f, const char* name>
    struct object scripted_iterator_every_logic<Th, F, f, name>::process(context* ctx) const {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      using predicate_arg_type = function_argument_type<predicate_type, 0>;

      size_t counter = 0;
      bool val = true;
      const auto pred_func = [&] (predicate_arg_type arg) -> bool {
        if (!val) return false;

        ctx->index = counter;
        ++counter;

        ctx->current = object(arg);

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) return true;
        }

        bool final_r = true;
        for (auto cur = childs; cur != nullptr && final_r; cur = cur->next) {
          const auto &obj = cur->process(ctx);
          final_r = obj.ignore() ? true : obj.get<bool>();
        }

        val = val && final_r;
        return true;
      };

      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);

      if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
      else {
        Th cur = ctx->current.get<Th>();
        std::invoke(f, cur, pred_func);
      }

      return object(val);
    }

    template <typename Th, typename F, F f, const char* name>
    local_state* scripted_iterator_every_logic<Th, F, f, name>::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      if (ctx->current.unresolved() && !std::is_same_v<void*, Th>) {
        change_scope cs(ctx, unresolved_value, ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        local_every_logic_func(ctx, allocator, name, condition, childs, unresolved_value, &first_child, &children);
      } else {
        using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
        using predicate_arg_type = function_argument_type<predicate_type, 0>;

        size_t counter = 0;
        const auto pred_func = [&] (predicate_arg_type arg) -> bool {
          ctx->current = object(arg);
          ctx->index = counter;
          ++counter;
          local_every_logic_func(ctx, allocator, name, condition, childs, ctx->current, &first_child, &children);
          return true;
        };

        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);

        if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
        else {
          Th cur = ctx->current.get<Th>();
          std::invoke(f, cur, pred_func);
        }
      }

      ptr->children = first_child;
      ptr->value = compute_every_logic_value(ptr);
      ptr->func = &compute_every_logic_value;

      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name>
//     void scripted_iterator_every_logic<Th, F, f, name>::draw(context* ctx) const {
//       const auto val = process(ctx);
//
//       {
//         local_state dd(ctx);
//         dd.function_name = name;
//         dd.value = val;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       Th cur;
//       if constexpr (std::is_same_v<void*, Th>) cur = nullptr;
//       else cur = ctx->current.get<Th>();
//       object first;
//       f(cur, [&] (const object &obj) -> bool {
//         if (condition != nullptr) {
//           const auto obj = condition->process(ctx);
//           if (obj.ignore() || !obj.get<bool>()) return true;
//         }
//
//         first = obj;
//         return false;
//       });
//       if (!first.valid()) return;
//
//       change_scope cs(ctx, first, ctx->current);
//       change_function_name cfn(ctx, name);
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

    template <typename Th, typename F, F f, const char* name>
    size_t scripted_iterator_every_logic<Th, F, f, name>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name>
    std::string_view scripted_iterator_every_logic<Th, F, f, name>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_has<Th, F, f, name>::scripted_iterator_has(const interface* childs) noexcept : max_count(nullptr), percentage(nullptr), childs(childs) {}
    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_has<Th, F, f, name>::scripted_iterator_has(const interface* max_count, const interface* percentage, const interface* childs) noexcept :
      max_count(max_count), percentage(percentage), childs(childs) {}
    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_has<Th, F, f, name>::~scripted_iterator_has() noexcept {
      if (max_count != nullptr) max_count->~interface();
      if (percentage != nullptr) percentage->~interface();
      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename Th, typename F, F f, const char* name>
    struct object scripted_iterator_has<Th, F, f, name>::process(context* ctx) const {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      using predicate_arg_type = function_argument_type<predicate_type, 0>;

      size_t final_max_count = SIZE_MAX;
      size_t counter = 0;
      size_t val = 0;
      const auto pred_func = [&] (predicate_arg_type arg) -> bool {
        if (val >= final_max_count) return false;
        ctx->index = counter;
        ++counter;
        ctx->current = object(arg);
        // задать в контекст текущий val и counter в качестве локальных переменных

        bool final_r = true;
        for (auto cur = childs; cur != nullptr && final_r; cur = cur->next) {
          const auto &obj = cur->process(ctx);
          final_r = obj.ignore() ? true : obj.get<bool>();
        }

        // val - должен посчитать количество совпадений + должен быть ограничен сверху final_max_count
        val += size_t(final_r);
        return true;
      };

      if (percentage != nullptr) {
        const auto val = percentage->process(ctx);
        const double final_percent = val.get<double>();
        if (final_percent < 0.0) throw std::runtime_error(std::string(name) + " percentage cannot be less than zero");

        size_t counter = 0;
        const auto counter_func = [&counter] (predicate_arg_type) -> bool { ++counter; return true; };
        if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, counter_func);
        else {
          Th cur = ctx->current.get<Th>();
          std::invoke(f, cur, counter_func);
        }

        final_max_count = counter * final_percent;
      } else if (max_count != nullptr) {
        const auto val = max_count->process(ctx);
        const double v = val.get<double>();
        if (v < 0.0) throw std::runtime_error(std::string(name) + " count cannot be less than zero");
        final_max_count = v;
      }

      if (final_max_count == 0) return object(0.0);

      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);

      if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
      else {
        Th cur = ctx->current.get<Th>();
        std::invoke(f, cur, pred_func);
      }

      return object(double(val));
    }

    static void local_has_func(
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
      bool l_val = true;
      auto ptr = allocator->create(ctx, name);
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
      ptr->children = l_first_child;
      ptr->func = &compute_every_logic_child_value;

      if (*first_child == nullptr) *first_child = ptr;
      if (*children != nullptr) (*children)->next = ptr;
      *children = ptr;
    }

    template <typename Th, typename F, F f>
    static struct object compute_has_value(const local_state* s) {


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
          using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
          using predicate_arg_type = function_argument_type<predicate_type, 0>;

          const double final_percent = arg->value.get<double>();
          if (final_percent < 0.0) throw std::runtime_error("'percentage' cannot be less than zero");

          size_t counter = 0;
          const auto counter_func = [&] (predicate_arg_type) -> bool { ++counter; return true; };
          if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, counter_func);
          else {
            Th cur = s->current.get<Th>();
            std::invoke(f, cur, counter_func);
          }

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
    }

    template <typename Th, typename F, F f, const char* name>
    local_state* scripted_iterator_has<Th, F, f, name>::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      local_state* first_child = nullptr;
      local_state* children = nullptr;
      if (ctx->current.unresolved() && !std::is_same_v<void*, Th>) {
        change_scope cs(ctx, unresolved_value, ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        local_has_func(ctx, allocator, name, childs, unresolved_value, &first_child, &children);
      } else {
        using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
        using predicate_arg_type = function_argument_type<predicate_type, 0>;

        size_t counter = 0;
        const auto pred_func = [&] (predicate_arg_type obj) -> bool {
          ctx->current = obj;
          ctx->index = counter;
          ++counter;
          local_has_func(ctx, allocator, name, childs, obj, &first_child, &children);
          return true;
        };

        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);

        if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
        else {
          Th cur = ctx->current.get<Th>();
          std::invoke(f, cur, pred_func);
        }
      }

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
      ptr->value = compute_has_value<Th, F, f>(ptr);
      ptr->func = &compute_has_value<Th, F, f>;

      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name>
//     void scripted_iterator_has<Th, F, f, name>::draw(context* ctx) const {
//       const auto value = process(ctx);
//
//       {
//         object count;
//         object percent;
//         if (max_count != nullptr) count = max_count->process(ctx);
//         if (percentage != nullptr) percent = percentage->process(ctx);
//
//         local_state arg(ctx);
//         if (percentage != nullptr) {
//           arg.argument_name = "percentage";
//           arg.value = percent;
//         } else if (max_count != nullptr) {
//           arg.argument_name = "count";
//           arg.value = count;
//         }
//
//         local_state dd(ctx, get_name());
//         dd.value = value;
//         if (percentage != nullptr || max_count != nullptr) dd.children = &arg;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       Th cur;
//       if constexpr (std::is_same_v<void*, Th>) cur = nullptr;
//       else cur = ctx->current.get<Th>();
//
//       object first;
//       f(cur, [&] (const object &obj) -> bool {
//         first = obj;
//         return false;
//       });
//       if (!first.valid()) return;
//
//       change_scope cs(ctx, first, ctx->current);
//       change_function_name cfn(ctx, name);
//       change_nesting cn(ctx, ctx->nest_level+1);
//       for (auto cur = childs; cur != nullptr; cur = cur->next) {
//         cur->draw(ctx);
//       }
//     }

    template <typename Th, typename F, F f, const char* name>
    size_t scripted_iterator_has<Th, F, f, name>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name>
    std::string_view scripted_iterator_has<Th, F, f, name>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_random<Th, F, f, name>::scripted_iterator_random(const size_t &state, const interface* condition, const interface* weight, const interface* childs) noexcept :
      state(state), condition(condition), weight(weight), childs(childs) {}
    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_random<Th, F, f, name>::~scripted_iterator_random() noexcept {
      if (condition != nullptr) condition->~interface();
      if (weight != nullptr) weight->~interface();
      //for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
      childs->~interface();
    }

    template <typename F, F f, typename Th>
    static struct object get_rand_obj(context* ctx, const interface* condition, const interface* weight, const size_t state, const std::string_view &func_name) {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      using predicate_arg_type = function_argument_type<predicate_type, 0>;

      size_t counter = 0;
      double accum_weight = 0.0;
      std::vector<std::pair<struct object, double>> objects;
      const auto pred_func = [&] (predicate_arg_type arg) -> bool {
        ctx->current = object(arg);
        ctx->index = counter;
        ++counter;

        // здесь поди тоже можно задать индексы

        if (condition != nullptr) {
          const auto obj = condition->process(ctx);
          if (obj.ignore() || !obj.get<bool>()) return true;
        }

        object weight_val(1.0);
        if (weight != nullptr) {
          weight_val = weight->process(ctx);
        }

        const double local = weight_val.get<double>();
        if (local < 0.0) throw std::runtime_error(std::string(func_name) + " weights must not be less than zero");
        objects.emplace_back(ctx->current, local);
        accum_weight += local;
        return true;
      };

      change_scope cs(ctx, object(), ctx->current);
      change_indices ci(ctx, 0, ctx->index);

      objects.reserve(50);
      if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
      else {
        Th cur = ctx->current.get<Th>();
        std::invoke(f, cur, pred_func);
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

    template <typename Th, typename F, F f, const char* name>
    struct object scripted_iterator_random<Th, F, f, name>::process(context* ctx) const {
      const auto obj = get_rand_obj<F, f, Th>(ctx, condition, weight, state, name);
      // рандом можно использовать только в эффекте, или нет? что я получаю если не в эффекте?
      // если передавать сюда собранный скрипт объекта, то понятно становится что возвращать
      change_scope cs(ctx, object(), ctx->current);
      ctx->current = obj;
      return obj.valid() ? childs->process(ctx) : ignore_value;
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

    // вообще тут можно попробовать найти случайный объект, но тогда придется process все таки вызывать
    template <typename Th, typename F, F f, const char* name>
    local_state* scripted_iterator_random<Th, F, f, name>::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());
      auto final_state = allocator->create(ctx, get_name());

      local_state* first_child = nullptr;
      local_state* children = nullptr;

      {
        change_function_name cfn(ctx, get_name());
        change_nesting cn(ctx, ctx->nest_level+1);
        if (ctx->current.unresolved() && !std::is_same_v<void*, Th>) {
          change_scope cs(ctx, unresolved_value, ctx->current);
          change_indices ci(ctx, 0, ctx->index);
          local_random_func(ctx, allocator, name, condition, weight, unresolved_value, &first_child, &children);
        } else {
          using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
          using predicate_arg_type = function_argument_type<predicate_type, 0>;

          size_t counter = 0;
          const auto pred_func = [&] (predicate_arg_type arg) -> bool {
            ctx->current = object(arg);
            ctx->index = counter;
            ++counter;
            local_random_func(ctx, allocator, name, condition, weight, ctx->current, &first_child, &children);
            return true;
          };

          change_scope cs(ctx, object(), ctx->current);
          change_indices ci(ctx, 0, ctx->index);

          if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
          else {
            Th cur = ctx->current.get<Th>();
            std::invoke(f, cur, pred_func);
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

    //using test_type = int*;
    //static_assert(std::is_fundamental_v<test_type> && std::is_pointer_v<test_type>);

//     template <typename Th, typename F, F f, const char* name>
//     void scripted_iterator_random<Th, F, f, name>::draw(context* ctx) const {
//       Th cur;
//       if constexpr (std::is_same_v<void*, Th>) cur = nullptr;
//       else cur = ctx->current.get<Th>();
//       auto obj = get_rand_obj<F, f>(ctx, cur, condition, weight, state, name);
//       // а нужно ли это рисовать вообще если рандом не получился? не думаю, но было бы неплохо для дебага
//       if (!obj.valid()) {
//         object first;
//         f(cur, [&] (const object &obj) -> bool {
//           first = obj;
//           return false;
//         });
//         obj = first;
//       }
//
//       //if (!obj.valid()) return;
//
//       {
//         local_state dd(ctx);
//         dd.function_name = name;
//         dd.value = obj;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       change_scope cs(ctx, obj, ctx->current);
//       //change_nesting cn(ctx, ctx->nest_level+1);
//       change_function_name cfn(ctx, name);
// //       for (auto cur = childs; cur != nullptr; cur = cur->next) {
// //         cur->draw(ctx);
// //       }
//       childs->draw(ctx);
//     }

    template <typename Th, typename F, F f, const char* name>
    size_t scripted_iterator_random<Th, F, f, name>::get_type_id() const { return type_id<Th>(); }
    template <typename Th, typename F, F f, const char* name>
    std::string_view scripted_iterator_random<Th, F, f, name>::get_name() const { return name; }

    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_view<Th, F, f, name>::scripted_iterator_view(const interface* default_value, const interface* childs) noexcept : default_value(default_value), childs(childs) {}
    template <typename Th, typename F, F f, const char* name>
    scripted_iterator_view<Th, F, f, name>::~scripted_iterator_view() noexcept {
      if (default_value != nullptr) default_value->~interface();
      for (auto cur = childs; cur != nullptr; cur = cur->next) { cur->~interface(); }
    }

    template <typename Th, typename F, F f, const char* name>
    struct object scripted_iterator_view<Th, F, f, name>::process(context* ctx) const {
      using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
      using predicate_arg_type = function_argument_type<predicate_type, 0>;

      size_t counter = 0;
      object cur_ret = ignore_value;
      const auto pred_func = [&] (predicate_arg_type arg) -> bool {
        ctx->current = object(arg);
        ctx->index = counter;
        for (auto child = childs; child != nullptr && !ctx->current.ignore(); child = child->next) {
          const auto &ret = child->process(ctx);
          ctx->current = ret;
        }

        counter += size_t(!ctx->current.ignore());
        cur_ret = !ctx->current.ignore() ? ctx->current : cur_ret;
        return true;
      };

      const auto def_val = default_value != nullptr ? default_value->process(ctx) : object();

      change_indices ci(ctx, 0, ctx->index);
      change_reduce_value crv(ctx, def_val);
      change_scope cs(ctx, object(), ctx->current);

      if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
      else {
        Th cur = ctx->current.get<Th>();
        std::invoke(f, cur, pred_func);
      }

      return cur_ret;
    }

    // причина?
//     static struct object compute_view_child_value(const local_state* s) {
//       bool l_unresolved = false;
//       object l_val;
//
//       for (auto child = s->children; child != nullptr; child = child->next) {
//         if (child->value.unresolved()) l_unresolved = true;
//         else l_val = child->value;
//         // смена контекста?
//       }
//
//       return l_unresolved ? unresolved_value : l_val;
//     }

    // берем последнее значение
    static struct object compute_view_value(const local_state* s) {
      object l_val;
      for (auto child = s->children; child != nullptr; child = child->next) {
        l_val = child->value;
      }
      return l_val;
    }

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

    template <typename Th, typename F, F f, const char* name>
    local_state* scripted_iterator_view<Th, F, f, name>::compute(context* ctx, local_state_allocator* allocator) const {
      auto ptr = allocator->create(ctx, get_name());

      change_function_name cfn(ctx, get_name());
      change_nesting cn(ctx, ctx->nest_level+1);

      auto d_v = default_value != nullptr ? default_value->compute(ctx, allocator) : nullptr;
      change_reduce_value crv(ctx, d_v != nullptr ? d_v->value : object());
      local_state* first_child = nullptr;
      local_state* children = nullptr;
      if (ctx->current.unresolved() && !std::is_same_v<void*, Th>) {
        change_scope cs(ctx, unresolved_value, ctx->current);
        change_indices ci(ctx, 0, ctx->index);
        local_view_func(ctx, allocator, name, childs, unresolved_value, &first_child, &children);
      } else {
        using predicate_type = typename std::conditional<get_function_argument_count<F>() == 1, final_arg_type<F, 0>, final_arg_type<F, 1>>::type;
        using predicate_arg_type = function_argument_type<predicate_type, 0>;

        size_t counter = 0;
        const auto pred_func = [&] (predicate_arg_type arg) -> bool {
          ctx->current = object(arg);
          ctx->index = counter;
          ++counter;
          local_view_func(ctx, allocator, name, childs, ctx->current, &first_child, &children);
          return true;
        };

        change_scope cs(ctx, object(), ctx->current);
        change_indices ci(ctx, 0, ctx->index);

        if constexpr (std::is_invocable_v<F, predicate_type>) std::invoke(f, pred_func);
        else {
          Th cur = ctx->current.get<Th>();
          std::invoke(f, cur, pred_func);
        }
      }

      if (d_v != nullptr) {
        d_v->argument_name = "default_value";
        d_v->next = first_child;
      }

      ptr->children = d_v == nullptr ? first_child : d_v;
      ptr->value = compute_view_value(ptr);
      ptr->func = &compute_view_value;
      return ptr;
    }

//     template <typename Th, typename F, F f, const char* name>
//     void scripted_iterator_view<Th, F, f, name>::draw(context* ctx) const {
//       {
//         const auto obj = process(ctx);
//         local_state dd(ctx);
//         dd.function_name = name;
//         dd.value = obj.ignore() ? object() : obj;
//         if (!ctx->draw(&dd)) return;
//       }
//
//       //change_scope cs(ctx, obj, ctx->current);
//       change_nesting cn(ctx, ctx->nest_level+1);
//       change_function_name cfn(ctx, name);
//       for (auto child = childs; child != nullptr; child = child->next) { child->draw(ctx); }
//     }

    template <typename Th, typename F, F f, const char* name>
    size_t scripted_iterator_view<Th, F, f, name>::get_type_id() const { return type_id<object>(); }
    template <typename Th, typename F, F f, const char* name>
    std::string_view scripted_iterator_view<Th, F, f, name>::get_name() const { return name; }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
