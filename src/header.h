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

      T process(context* ctx) const {
        allocate_additional_locals aal(ctx, max_locals());

        if constexpr (std::is_same_v<T, void>) {
          data->begin->process(ctx);
          return;
        } else {
          const auto ret = data->begin->process(ctx);
          return ret.get<T>();
        }
      }

      // compute script state
      local_state* compute(context* ctx, local_state_allocator* allocator) const {
        allocate_additional_locals aal(ctx, max_locals());
        return data->begin->compute(ctx, allocator);
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

    using number = base<double>;
    using string = base<std::string_view>;
    using consition = base<bool>;
    using effect = base<void>;
    template <typename T>
    using user_data = base<T>;
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
