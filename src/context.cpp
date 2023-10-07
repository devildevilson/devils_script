#include "context.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    using SCRIPT_SYSTEM_DEFAULT_GENERATOR_NAMESPACE::advance;
    using SCRIPT_SYSTEM_DEFAULT_GENERATOR_NAMESPACE::get_value;
    using state = SCRIPT_SYSTEM_DEFAULT_GENERATOR_NAMESPACE::state;

    static object default_local_state_func(const local_state* s) { return s->value; }

    local_state::local_state() noexcept :
      seed(0), type(SIZE_MAX), operator_type(SIZE_MAX), nest_level(0), index(0), prev_index(0), children(nullptr), next(nullptr), user_data(nullptr),
      func(&default_local_state_func)
    {}
    local_state::local_state(context* ctx) noexcept :
      prev_function_name(ctx->prev_function_name), //id(ctx->id), method_name(ctx->method_name),
      id_hash(ctx->id_hash), method_hash(ctx->method_hash), seed(ctx->seed), type(ctx->type), operator_type(ctx->operator_type),
      nest_level(ctx->nest_level), index(ctx->index), prev_index(ctx->prev_index), local_rand_state(0),
      current(ctx->current), prev(ctx->prev),
      children(nullptr), next(nullptr), user_data(ctx->user_data),
      func(&default_local_state_func)
    {}

    local_state::local_state(context* ctx, const std::string_view &function_name) noexcept :
      function_name(function_name), prev_function_name(ctx->prev_function_name),  //id(ctx->id), method_name(ctx->method_name),
      id_hash(ctx->id_hash), method_hash(ctx->method_hash), seed(ctx->seed), type(ctx->type), operator_type(ctx->operator_type), nest_level(ctx->nest_level),
      index(ctx->index), prev_index(ctx->prev_index), local_rand_state(0),
      current(ctx->current), prev(ctx->prev),
      children(nullptr), next(nullptr), user_data(ctx->user_data),
      func(&default_local_state_func)
    {}

    local_state::local_state(context* ctx, const std::string_view &function_name, const object &value) noexcept :
      function_name(function_name), prev_function_name(ctx->prev_function_name),  //id(ctx->id), method_name(ctx->method_name),
      id_hash(ctx->id_hash), method_hash(ctx->method_hash), seed(ctx->seed), type(ctx->type), operator_type(ctx->operator_type), nest_level(ctx->nest_level),
      index(ctx->index), prev_index(ctx->prev_index), local_rand_state(0),
      current(ctx->current), prev(ctx->prev), value(value),
      children(nullptr), next(nullptr), user_data(ctx->user_data),
      func(&default_local_state_func)
    {}

    uint64_t local_state::get_random_value(const size_t &static_state) const noexcept {
      const state s = { id_hash, method_hash, seed, static_state };
      return get_value(advance(s));
    }

    void local_state::for_each(const std::function<void(local_state*)> &f) {
      for (auto child = children; child != nullptr; child = child->next) {
        f(child);
      }
    }

//     void draw_data::set_arg(const uint32_t &index, const std::string_view &name, const object &obj) {
//       arguments[index].first = name;
//       arguments[index].second = obj;
//     }

//     double random_state::normalize(const uint64_t val) {
//       return utils::rng_normalize(val);
//     }
//
//     random_state::random_state() {}
//     random_state::random_state(const size_t &val1, const size_t &val2, const size_t &val3, const size_t &val4) :
//       cur{ val1, val2, val3, val4 }
//     {}
//
//     random_state::random_state(const size_t &state_root, const size_t &state_container, const size_t &current_turn) :
//       cur{ state_root, state_container, current_turn, utils::splitmix64::get_value(utils::splitmix64::rng({current_turn})) }
//     {}
//
//     uint64_t random_state::next() {
//       using DEFAULT_GENERATOR_NAMESPACE::rng;
//       using DEFAULT_GENERATOR_NAMESPACE::get_value;
//       cur = rng(cur);
//       return get_value(cur);
//     }

    void local_state_allocator::traverse_and_free(local_state* data) {
      for (auto child = data->children; child != nullptr;) {
        auto tmp = child->next;
        traverse_and_free(child);
        child = tmp;
      }
      free(data);
    }

    local_state* default_local_state_allocator::create(context* ctx) { return local_state_pool.create(ctx); }
    local_state* default_local_state_allocator::create(context* ctx, const std::string_view &fn) { return local_state_pool.create(ctx, fn); }
    local_state* default_local_state_allocator::create(context* ctx, const std::string_view &fn, const object &v) { return local_state_pool.create(ctx, fn, v); }
    void default_local_state_allocator::free(local_state* ptr) { local_state_pool.destroy(ptr); }

    double context::normalize_value(const uint64_t value) {
      return utils::rng_normalize(value);
    }

    // здесь довольно много вычислений, но они суперпростые
    static uint64_t mix_value(const uint64_t &val) {
      return utils::splitmix64::get_value(utils::splitmix64::advance({val}));
    }

    //const uint64_t hash_seed = 128847150991130ull;
    //const uint64_t hash_seed = 18446744073709551293ull; // 18446744073709551253ull; // prime?
    //const uint64_t hash_seed = uint64_t(-1)-363; // prime?
    const uint64_t hash_seed = (size_t(0xc6a4a793) << 32) | size_t(0x5bd1e995); // мож такой использовать?
    context::context() noexcept :
      type(SIZE_MAX), operator_type(SIZE_MAX), nest_level(0),
      id_hash(0), method_hash(0), seed(0),
      index(0), prev_index(0), locals_offset(0)
//      vector_ptr(nullptr), add_func_ptr(nullptr), user_data(nullptr)
    {}
    context::context(const std::string_view &id, const std::string_view &method_name, const size_t &seed) noexcept :
      id(id), method_name(method_name),
      type(SIZE_MAX), operator_type(SIZE_MAX), nest_level(0),
      id_hash(murmur_hash64A(id, hash_seed)), // используем свою реализацию, вообще имеет смысл придумать какой то сид
      method_hash(murmur_hash64A(method_name, hash_seed)),
      seed(mix_value(seed)),
      index(0), prev_index(0), locals_offset(0)
//      vector_ptr(nullptr), add_func_ptr(nullptr), user_data(nullptr)
    {}

    void context::set_data(const std::string_view &id, const std::string_view &method_name, const size_t &seed) noexcept {
      this->id = id;
      this->method_name = method_name;
      this->seed = mix_value(seed);
      id_hash = murmur_hash64A(id, hash_seed);
      method_hash = murmur_hash64A(method_name, hash_seed);
    }

    void context::set_data(const std::string_view &id, const std::string_view &method_name) noexcept {
      this->id = id;
      this->method_name = method_name;
      id_hash = murmur_hash64A(id, hash_seed);
      method_hash = murmur_hash64A(method_name, hash_seed);
    }

//    bool context::draw_state() const noexcept { return bool(draw_function); }
//    bool context::draw(const local_state* data) const {
//      if (!draw_state()) return true;
//      return draw_function(data);
//    }

//    // по идее рекурсивный метод достаточно быстрый
//    void context::traverse_and_draw(const local_state* data) const {
//      if (!draw_state()) throw std::runtime_error("Context does not have draw function");
//      if (!draw(data)) return;
//      for (auto child = data->children; child != nullptr; child = child->next) {
//        traverse_and_draw(child);
//      }
//    }

    uint64_t context::get_random_value(const size_t &static_state) const noexcept {
      // хеши наверное тоже нужно замиксить, вряд ли, там получаются неплохие значения
      const state s = { id_hash, method_hash, seed, static_state };
      return get_value(advance(s));
    }

    object context::get_local(const size_t index) const {
      const size_t final_index = locals_offset + index;
      return locals[final_index];
    }

    void context::save_local(const size_t index, const object obj) {
      const size_t final_index = locals_offset + index;
      if (final_index >= locals.size()) throw std::runtime_error("Local index too big (" + std::to_string(final_index) + " >= " + std::to_string(locals.size()) + ")");
      locals[final_index] = obj;
    }

    void context::remove_local(const size_t index) noexcept {
      const size_t final_index = locals_offset + index;
      if (final_index >= locals.size()) return;
      locals[final_index] = object();
    }

    void context::clear() noexcept {
      map.clear();
      object_lists.clear();
      unique_objects.clear();
      //memset(locals.data(), 0, sizeof(locals[0]) * locals.size());
      //for (auto &l : locals) { l = object(); }
      locals.clear();
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
