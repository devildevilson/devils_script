#pragma once

#include <cstdint>
#include <cstddef>

// Small deterministic PRNG helpers for script randomness.
//
// xoshiro256** is used for per-context random streams. The mix helpers derive stable
// one-shot values from script seeds and numeric inputs; multi-value mixing intentionally
// uses SplitMix/Murmur-style hashing instead of consuming several xoshiro outputs from
// a freshly initialized state.
//
// Algorithms are based on the public-domain generators from http://prng.di.unimi.it/.

namespace devils_script {
namespace prng {

double prng_normalize(const uint64_t value) noexcept;
uint64_t mix(const uint64_t v1) noexcept;
uint64_t mix(const uint64_t v1, const uint64_t v2) noexcept;
uint64_t mix(const uint64_t v1, const uint64_t v2, const uint64_t v3) noexcept;
uint64_t mix(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;
uint64_t mix_splitmix(const uint64_t v1, const uint64_t v2) noexcept;
uint64_t mix_hash(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;
uint64_t mix_splitmix(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept;


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
}
