// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
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
#include <memory>

#include "detail/lowlevel_allocator.hpp"

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
#include "allocator_traits.hpp"
#endif

namespace foonathan
{
    namespace memory
    {
        struct allocator_info;

        namespace detail
        {
            struct malloc_allocator_impl
            {
                static allocator_info info() noexcept;

                static void* allocate(std::size_t size, std::size_t) noexcept
                {
                    return std::malloc(size);
                }

                static void deallocate(void* ptr, std::size_t, std::size_t) noexcept
                {
                    std::free(ptr);
                }

                static std::size_t max_node_size() noexcept
                {
                    return std::allocator_traits<std::allocator<char>>::max_size({});
                }
            };

            FOONATHAN_MEMORY_LL_ALLOCATOR_LEAK_CHECKER(malloc_allocator_impl,
                                                       malloc_alloator_leak_checker)
        } // namespace detail

        /// A stateless \concept{concept_rawallocator,RawAllocator} that allocates memory using <tt>std::malloc()</tt>.
        /// It throws \ref out_of_memory when the allocation fails.
        /// \ingroup allocator
        using malloc_allocator =
            FOONATHAN_IMPL_DEFINED(detail::lowlevel_allocator<detail::malloc_allocator_impl>);

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class detail::lowlevel_allocator<detail::malloc_allocator_impl>;
        extern template class allocator_traits<malloc_allocator>;
#endif
    } // namespace memory
} // namespace foonathan

#endif //FOONATHAN_MEMORY_MALLOC_ALLOCATOR_HPP_INCLUDED
