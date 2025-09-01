#include "devils_script/prng.h"

#include "devils_script/type_traits.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif
namespace prng {

// миксить 4 числа ОДНИМ вызовом xoshiro256starstar неправильно!!!
// довольно просто можно сделать на основе хешфункции

static inline uint64_t rotl(const uint64_t x, int k) {
  return (x << k) | (x >> (64 - k));
}

double prng_normalize(const uint64_t value) noexcept {
  union { uint64_t i; double d; } u;
  u.i = (UINT64_C(0x3FF) << 52) | (value >> 12);
  return u.d - 1.0;
}

uint64_t mix(const uint64_t v1) noexcept {
  return splitmix64::value(splitmix64::next(splitmix64::init(v1)));
}

uint64_t mix_hash(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept {
  const uint64_t arr[4] = { v1, v2, v3, v4 };
  const size_t size = sizeof(arr) / sizeof(arr[0]);
  const auto input = std::string_view(reinterpret_cast<const char*>(arr), sizeof(uint64_t) * size);
  return utils::murmur_hash64A(input, splitmix64::value(splitmix64::next(splitmix64::init(v1))));
}

uint64_t mix_splitmix(const uint64_t v1, const uint64_t v2) noexcept {
  uint64_t x = v1 ^ (v2 + 0x9e3779b97f4a7c15ULL);
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL; 
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL; 
  x = x ^ (x >> 31); 
  return x;
}

uint64_t mix(const uint64_t v1, const uint64_t v2) noexcept {
  return mix_splitmix(v1, v2);
}

uint64_t mix_splitmix(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept {
  return mix_splitmix(mix_splitmix(v1, v2), mix_splitmix(v3, v4));
}

uint64_t mix_xoshiro1(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept {
  xoshiro256starstar::state s{ { mix(v1), mix(v2), mix(v3), mix(v4) } };
  return xoshiro256starstar::value(xoshiro256starstar::next(s));
}

constexpr size_t mix_count = 10;
uint64_t mix_xoshiro2(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept {
  xoshiro256starstar::state s{ { v1, v2, v3, v4 } };
  for (size_t i = 0; i < mix_count; ++i) {
    s = xoshiro256starstar::next(s);
  }
  return xoshiro256starstar::value(s);
}

uint64_t mix_mulxor(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept {
  uint64_t x = (v1 ^ (v2 << 1)) * 0x9e3779b97f4a7c15ULL;
  x ^= (v3 ^ (v4 << 3)) * 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  return x ^ (x >> 29);
}

uint64_t mix(const uint64_t v1, const uint64_t v2, const uint64_t v3) noexcept {
  return mix_splitmix(v1, v2, v3, mix(v1));
}

uint64_t mix(const uint64_t v1, const uint64_t v2, const uint64_t v3, const uint64_t v4) noexcept {
  return mix_hash(v1, v2, v3, v4);
}

splitmix64::state splitmix64::init(const uint64_t seed) noexcept {
  return next(splitmix64::state{ { seed } });
}

splitmix64::state splitmix64::next(state s) noexcept {
  return splitmix64::state{ { s.s[0] + 0x9e3779b97f4a7c15ull } };
}

uint64_t splitmix64::value(const state& s) noexcept {
  uint64_t z = s.s[0];
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}

template <typename T>
typename T::state typed_init(const uint64_t seed) {
  typename T::state new_state;
  const size_t state_size = T::state_size;
  splitmix64::state splitmix_states[state_size];
  splitmix_states[0] = splitmix64::next(splitmix64::state{ { seed } });
  for (size_t i = 1; i < state_size; ++i) splitmix_states[i] = splitmix64::next(splitmix_states[i - 1]);
  for (size_t i = 0; i < state_size; ++i) new_state.s[i] = splitmix64::value(splitmix_states[i]);
  return new_state;
}

xoshiro256starstar::state xoshiro256starstar::init(const uint64_t seed) noexcept {
  return typed_init<xoshiro256starstar>(seed);
}

xoshiro256starstar::state xoshiro256starstar::next(state s) noexcept {
  const uint64_t t = s.s[1] << 17;
  s.s[2] ^= s.s[0];
  s.s[3] ^= s.s[1];
  s.s[1] ^= s.s[2];
  s.s[0] ^= s.s[3];
  s.s[2] ^= t;
  s.s[3] = rotl(s.s[3], 45);
  return s;
}

uint64_t xoshiro256starstar::value(const state& s) noexcept {
  return rotl(s.s[1] * 5, 7) * 9;
}


}
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}