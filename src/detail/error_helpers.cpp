// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/error_helpers.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <cstdio>
#endif

#include <cstdlib>

#include "error.hpp"

using namespace foonathan::memory;
using namespace detail;

void detail::check_allocation_size(std::size_t passed, std::size_t supported, const allocator_info &info)
{
    if (passed > supported)
        FOONATHAN_THROW(bad_allocation_size(info, passed, supported));
}

void detail::handle_out_of_memory(const allocator_info &info, std::size_t amount)
{
    FOONATHAN_THROW(out_of_memory(info, amount));
}

void detail::handle_failed_assert(const char *msg, const char *file, int line, const char *fnc) FOONATHAN_NOEXCEPT
{
#if FOONATHAN_HOSTED_IMPLEMENTATION
    std::fprintf(stderr, "[%s] Assertion failure in function %s (%s:%d): %s.",
                     FOONATHAN_MEMORY_LOG_PREFIX, fnc, file, line, msg);
#endif
    std::abort();
}
