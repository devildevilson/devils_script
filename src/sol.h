#ifndef DEVILS_ENGINE_UTILS_SOL_H
#define DEVILS_ENGINE_UTILS_SOL_H

#ifndef _NDEBUG
  #define SOL_SAFE_USERTYPE 1
  #define SOL_SAFE_REFERENCES 1
  #define SOL_SAFE_FUNCTION_CALLS 1
  #define SOL_SAFE_NUMERICS 0
  #define SOL_SAFE_GETTER 1
  #define SOL_SAFE_FUNCTION_CALLS 1
  //#define SOL_ALL_SAFETIES_ON 1
  #include <sol/sol.hpp>
#else // release
  #include <sol/sol.hpp>
#endif

#define TO_LUA_INDEX(index) ((index)+1)
#define FROM_LUA_INDEX(index) ((index)-1)

#define CHECK_ERROR(ret) if (!ret.valid()) {       \
  sol::error err = ret;                            \
  std::cout << err.what();                         \
  luaL_error(s, "Catched lua error");              \
}

#define CHECK_ERROR_THROW(ret) if (!ret.valid()) {  \
  sol::error err = ret;                             \
  std::cout << err.what();                          \
  throw std::runtime_error("There are lua errors"); \
}

#endif
