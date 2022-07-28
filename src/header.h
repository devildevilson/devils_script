#ifndef DEVILS_ENGINE_SCRIPT_HEADER_H
#define DEVILS_ENGINE_SCRIPT_HEADER_H

#include <string_view>
#include "interface.h"
#include "common.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    class interface;
    struct context;

    template <typename T>
    class base {
    public:
      base() noexcept : data(nullptr) {}
      base(const script_data* data) noexcept : data(data) {}
      base(const base &copy) noexcept = default;
      base(base &&move) noexcept = default;
      base & operator=(const base &copy) noexcept = default;
      base & operator=(base &&move) noexcept = default;

      T compute(context* ctx) const {
        allocate_additional_locals aal(ctx, max_locals());

        if constexpr (std::is_same_v<T, void>) {
          data->begin->process(ctx);
          return;
        } else {
          const auto ret = data->begin->process(ctx);
          return ret.get<T>();
        }
      }
      // draw теперь не существует, вместо этого нужно вычислить полный стейт скрипта
      // для этого нужно создать аллокатор, вызвать compute и рекурсивно пройтись по всем детям стейта
      void draw(context* ctx) const {
        //begin->draw(ctx);
      }

      bool valid() const noexcept { return data != nullptr; }
      size_t size() const noexcept { return valid() ? data->size : 0; }
      size_t count() const noexcept { return valid() ? data->count : 0; }
      size_t max_locals() const noexcept { return valid() ? data->max_locals : 0; }
      const std::vector<std::string> & locals_data() const noexcept { return data->locals; }
      const script_data* native() const noexcept { return data; }
    private:
      const script_data* data;
    };

    class number : public base<double> {
    public:
      number() noexcept = default;
      number(const script_data* begin) noexcept : base(begin) {}
      number(const number &copy) noexcept = default;
      number(number &&move) noexcept = default;
      number & operator=(const number &copy) noexcept = default;
      number & operator=(number &&move) noexcept = default;
    };

    class string : public base<std::string_view> {
    public:
      string() noexcept = default;
      string(const script_data* begin) noexcept : base(begin) {}
      string(const string &copy) noexcept = default;
      string(string &&move) noexcept = default;
      string & operator=(const string &copy) noexcept = default;
      string & operator=(string &&move) noexcept = default;

    };
    class condition : public base<bool> {
    public:
      condition() noexcept = default;
      condition(const script_data* begin) noexcept : base(begin) {}
      condition(const condition &copy) noexcept = default;
      condition(condition &&move) noexcept = default;
      condition & operator=(const condition &copy) noexcept = default;
      condition & operator=(condition &&move) noexcept = default;
    };

    class effect : public base<void> {
    public:
      effect() noexcept = default;
      effect(const script_data* begin) noexcept : base(begin) {}
      effect(const effect &copy) noexcept = default;
      effect(effect &&move) noexcept = default;
      effect & operator=(const effect &copy) noexcept = default;
      effect & operator=(effect &&move) noexcept = default;
    };

    template <typename T>
    class user_data : public base<T> {
    public:
      user_data() noexcept = default;
      user_data(const script_data* begin) noexcept : base<T>(begin) {}
      user_data(const user_data &copy) noexcept = default;
      user_data(user_data &&move) noexcept = default;
      user_data & operator=(const user_data &copy) noexcept = default;
      user_data & operator=(user_data &&move) noexcept = default;
    };
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
