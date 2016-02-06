// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MALLOC_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_MALLOC_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::malloc_allocator.
/// \note Only available on a hosted implementation.

#include "config.hpp"
#if !FOONATHAN_HOSTED_IMPLEMENTATION
#error "This header is only available for a hosted implementation."
#endif

#include <cstdlib>

#include "detail/lowlevel_allocator.hpp"
#include "allocator_traits.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        struct malloc_allocator_impl
        {
            static allocator_info info() FOONATHAN_NOEXCEPT
            {
                return {FOONATHAN_MEMORY_LOG_PREFIX "malloc_allocator", nullptr};
            }

            static void* allocate(std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
            {
                return std::malloc(size);
            }

            static void deallocate(void *ptr, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
            {
                std::free(ptr);
            }

            static std::size_t max_node_size() FOONATHAN_NOEXCEPT
            {
                return std::allocator<char>().max_size();
            }
        };

#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
        lowlevel_allocator<malloc_allocator_impl>::leak_checker malloc_allocator_checker;
#endif
    } // namespace detail

    /// A stateless \concept{concept_rawallocator,RawAllocator} that allocates memory using <tt>std::malloc()</tt>.
    /// It throws \ref out_of_memory() when the allocation fails.
    /// \ingroup memory
    using malloc_allocator = FOONATHAN_IMPL_DEFINED(detail::lowlevel_allocator<detail::malloc_allocator_impl>);

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
    extern template class allocator_traits<malloc_allocator>;
#endif
}} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_MALLOC_ALLOCATOR_HPP_INCLUDED
