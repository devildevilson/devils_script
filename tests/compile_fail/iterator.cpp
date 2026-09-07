#include "devils_script/system.h"
#ifdef NEGATIVE
constexpr auto kind = devils_script::system::get_user_function_type<void(*)(), std::string_view, true>();
#else
constexpr auto kind = devils_script::system::get_user_function_type<void(*)(), double, true>();
static_assert(kind == devils_script::system::user_function_type::iterator_arithmetic);
#endif
