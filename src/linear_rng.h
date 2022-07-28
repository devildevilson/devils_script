#ifndef DEVILS_SCRIPT_LINEAR_RNG_H
#define DEVILS_SCRIPT_LINEAR_RNG_H

#include <cstdint>
#include <cstddef>

// http://prng.di.unimi.it/

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    namespace utils {
      double rng_normalize(const uint64_t &value);

      namespace splitmix64 {
        const size_t state_size = 1;
        struct state { uint64_t s[state_size]; };
        state init(const uint64_t &seed);
        state advance(state s);
        uint64_t get_value(const state &s);
      }

      namespace xoshiro256starstar {
        const size_t state_size = 4;
        struct state { uint64_t s[state_size]; };
        state init(const uint64_t &seed);
        state advance(state s);
        uint64_t get_value(const state &s);
      }
    }
  }
#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

#endif
