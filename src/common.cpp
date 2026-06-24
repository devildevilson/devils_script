#include "devils_script/common.h"

namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
namespace DEVILS_SCRIPT_INNER_NAMESPACE {
#endif

size_t compute_count1() { return 0; }

int64_t default_command_f(int64_t, context*, const script_container*) { return 0; }

void assert_msg_fn(context*, const script_container*, const std::string_view&, const size_t) {}

#ifdef DEVILS_SCRIPT_INNER_NAMESPACE
}
#endif
}
