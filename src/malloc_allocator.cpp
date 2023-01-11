// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "config.hpp"
#if FOONATHAN_HOSTED_IMPLEMENTATION

#include "malloc_allocator.hpp"

#include "error.hpp"

using namespace foonathan::memory;

allocator_info detail::malloc_allocator_impl::info() noexcept
{
    return {FOONATHAN_MEMORY_LOG_PREFIX "::malloc_allocator", nullptr};
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class detail::lowlevel_allocator<detail::malloc_allocator_impl>;
template class foonathan::memory::allocator_traits<malloc_allocator>;
#endif

#endif // FOONATHAN_HOSTED_IMPLEMENTATION
