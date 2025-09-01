#pragma once

#include <cstdint>
#include <cstddef>

// http://prng.di.unimi.it/

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif
namespace prng {

double prng_normalize(const uint64_t value) noexcept;
uint64_t mix(const uint64_t v1) noexcept;
uint64_t mix(const uint64_t v1, const uint64_t v2) noexcept;
uint64_t mix(const uint64_t v1, const uint64_t v2, const uint64_t v3) noexcept;
uint64_t mix(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;
uint64_t mix_splitmix(const uint64_t v1, const uint64_t v2) noexcept;
uint64_t mix_hash(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;
uint64_t mix_splitmix(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;
uint64_t mix_xoshiro1(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;
uint64_t mix_xoshiro2(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;
uint64_t mix_mulxor(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;


struct splitmix64 {
  static constexpr size_t state_size = 1;
  struct state {
    using outer = splitmix64;
    uint64_t s[state_size];
  };
  static state init(const uint64_t seed) noexcept;
  static state next(state s) noexcept;
  static uint64_t value(const state& s) noexcept;
};

struct xoshiro256starstar {
  static constexpr size_t state_size = 4;
  struct state {
    using outer = xoshiro256starstar;
    uint64_t s[state_size];
  };
  static state init(const uint64_t seed) noexcept;
  static state next(state s) noexcept;
  static uint64_t value(const state& s) noexcept;
};

}
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}