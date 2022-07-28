#include "linear_rng.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#  define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    namespace utils {
      double rng_normalize(const uint64_t &value) {
        union { uint64_t i; double d; } u;
        u.i = (UINT64_C(0x3FF) << 52) | (value >> 12);
        return u.d - 1.0;
      }

      static inline uint64_t rotl(const uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
      }

      namespace splitmix64 {
        state init(const uint64_t &seed) {
          return advance({seed});
        }

        state advance(state s) {
          return {s.s[0] + 0x9e3779b97f4a7c15};
        }

        uint64_t get_value(const state &s) {
          uint64_t z = s.s[0];
          z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
          z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
          return z ^ (z >> 31);
        }
      }

      namespace xoshiro256starstar {
        state init(const uint64_t &seed) {
          state new_state;
          splitmix64::state splitmix_states[state_size];
          splitmix_states[0] = splitmix64::advance({seed});
          for (uint32_t i = 1; i < state_size; ++i) splitmix_states[i] = splitmix64::advance(splitmix_states[i-1]);
          for (uint32_t i = 0; i < state_size; ++i) new_state.s[i] = splitmix64::get_value(splitmix_states[i]);
          return new_state;
        }

        state advance(state s) {
          const uint64_t t = s.s[1] << 17;
          s.s[2] ^= s.s[0];
          s.s[3] ^= s.s[1];
          s.s[1] ^= s.s[2];
          s.s[0] ^= s.s[3];
          s.s[2] ^= t;
          s.s[3] = rotl(s.s[3], 45);
          return s;
        }

        uint64_t get_value(const state &s) {
          return rotl(s.s[1] * 5, 7) * 9;
        }
      }
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
