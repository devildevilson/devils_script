#include "common.h"

#include "context.h"

#ifndef DEVILS_SCRIPT_INNER_NAMESPACE
#define DEVILS_SCRIPT_INNER_NAMESPACE devils_script
#endif // DEVILS_SCRIPT_INNER_NAMESPACE

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
namespace DEVILS_SCRIPT_OUTER_NAMESPACE {
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE

  namespace DEVILS_SCRIPT_INNER_NAMESPACE {
    void allocate_locals(context* ctx, const size_t add_count) {
      if (add_count == 0) return;
      ctx->locals_offset = ctx->locals.size();
      ctx->locals.resize(ctx->locals.size() + add_count);
    }

    allocate_additional_locals::allocate_additional_locals(context* ctx, const size_t add_count) noexcept : ctx(ctx), last_size(ctx->locals.size()), last_offset(ctx->locals_offset) {
      if (add_count == 0) return;
      ctx->locals_offset = ctx->locals.size();
      ctx->locals.resize(ctx->locals.size() + add_count);
    }

    allocate_additional_locals::~allocate_additional_locals() noexcept {
      ctx->locals_offset = last_offset;
      while (ctx->locals.size() > last_size) { ctx->locals.pop_back(); }
    }
  }

#ifdef DEVILS_SCRIPT_OUTER_NAMESPACE
}
#endif // DEVILS_SCRIPT_OUTER_NAMESPACE
