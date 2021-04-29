// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::heap_allocator and related functions.

#include "detail/lowlevel_allocator.hpp"
#include "config.hpp"

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
#include "allocator_traits.hpp"
#endif

namespace foonathan
{
    namespace memory
    {
        struct allocator_info;

        /// Allocates heap memory.
        /// This function is used by the \ref heap_allocator to allocate the heap memory.
        /// It is not defined on a freestanding implementation, a definition must be provided by the library user.
        /// \requiredbe This function shall return a block of uninitialized memory that is aligned for \c max_align_t and has the given size.
        /// The size parameter will not be zero.
        /// It shall return a \c nullptr if no memory is available.
        /// It must be thread safe.
        /// \defaultbe On a hosted implementation this function uses OS specific facilities, \c std::malloc is used as fallback.
        /// \ingroup allocator
        void* heap_alloc(std::size_t size) noexcept;

        /// Deallocates heap memory.
        /// This function is used by the \ref heap_allocator to allocate the heap memory.
        /// It is not defined on a freestanding implementation, a definition must be provided by the library user.
        /// \requiredbe This function gets a pointer from a previous call to \ref heap_alloc with the same size.
        /// It shall free the memory.
        /// The pointer will not be zero.
        /// It must be thread safe.
        /// \defaultbe On a hosted implementation this function uses OS specific facilities, \c std::free is used as fallback.
        /// \ingroup allocator
        void heap_dealloc(void* ptr, std::size_t size) noexcept;

        namespace detail
        {
            struct heap_allocator_impl
            {
                static allocator_info info() noexcept;

                static void* allocate(std::size_t size, std::size_t) noexcept
                {
                    return heap_alloc(size);
                }

                static void deallocate(void* ptr, std::size_t size, std::size_t) noexcept
                {
                    heap_dealloc(ptr, size);
                }

                static std::size_t max_node_size() noexcept;
            };

            FOONATHAN_MEMORY_LL_ALLOCATOR_LEAK_CHECKER(heap_allocator_impl,
                                                       heap_alloator_leak_checker)
        } // namespace detail

        /// A stateless \concept{concept_rawallocator,RawAllocator} that allocates memory from the heap.
        /// It uses the two functions \ref heap_alloc and \ref heap_dealloc for the allocation,
        /// which default to \c std::malloc and \c std::free.
        /// \ingroup allocator
        using heap_allocator =
            FOONATHAN_IMPL_DEFINED(detail::lowlevel_allocator<detail::heap_allocator_impl>);

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class detail::lowlevel_allocator<detail::heap_allocator_impl>;
        extern template class allocator_traits<heap_allocator>;
#endif
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED
