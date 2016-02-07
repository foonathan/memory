// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "config.hpp"
#if FOONATHAN_HOSTED_IMPLEMENTATION

    #include "malloc_allocator.hpp"

    #include "error.hpp"

    using namespace foonathan::memory;

    const allocator_info& detail::malloc_allocator_impl::info() FOONATHAN_NOEXCEPT
    {
        static allocator_info info(FOONATHAN_MEMORY_LOG_PREFIX "::malloc_allocator", nullptr);
        return info;
    }

    #if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        template class foonathan::memory::allocator_traits<malloc_allocator>;
    #endif

#endif // FOONATHAN_HOSTED_IMPLEMENTATION
