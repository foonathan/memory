// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "config.hpp"
#if FOONATHAN_HOSTED_IMPLEMENTATION

    #include "malloc_allocator.hpp"

    using namespace foonathan::memory;

    #if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        template class foonathan::memory::allocator_traits<malloc_allocator>;
    #endif

#endif // FOONATHAN_HOSTED_IMPLEMENTATION