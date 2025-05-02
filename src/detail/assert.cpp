// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "detail/assert.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
#include <cstdio>
#endif

#include <cstdlib>

#include "error.hpp"

using namespace foonathan::memory;
using namespace detail;

void detail::handle_failed_assert(const char* msg, const char* file, int line,
                                  const char* fnc) noexcept
{
#if FOONATHAN_HOSTED_IMPLEMENTATION
    std::fprintf(stderr, "[%s] Assertion failure in function %s (%s:%d): %s.\n",
                 FOONATHAN_MEMORY_LOG_PREFIX, fnc, file, line, msg);
#endif
    std::abort();
}

void detail::handle_warning(const char* msg, const char* file, int line, const char* fnc) noexcept
{
#if FOONATHAN_HOSTED_IMPLEMENTATION
    std::fprintf(stderr, "[%s] Warning triggered in function %s (%s:%d): %s.\n",
                 FOONATHAN_MEMORY_LOG_PREFIX, fnc, file, line, msg);
#endif
}
