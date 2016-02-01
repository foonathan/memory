// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MALLOC_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_MALLOC_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::malloc_allocator.
/// \note Only available on a hosted implementation.

#include <type_traits>

#include "config.hpp"
#if !FOONATHAN_HOSTED_IMPLEMENTATION
    #error "This header is only available for a hosted implementation."
#endif

#include "allocator_traits.hpp"

namespace foonathan { namespace memory
{
#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
    namespace detail
    {
        static struct malloc_allocator_leak_checker_initializer_t
        {
            malloc_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT;
            ~malloc_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT;
        } malloc_allocator_leak_checker_initializer;
    } // namespace detail
#endif

    /// A stateless \concept{concept_rawallocator,RawAllocator} that allocates memory using <tt>std::malloc()</tt>.
    /// \ingroup memory
    class malloc_allocator
    {
    public:
        using is_stateful = std::false_type;

        malloc_allocator() FOONATHAN_NOEXCEPT = default;
        malloc_allocator(malloc_allocator&&) FOONATHAN_NOEXCEPT {}
        ~malloc_allocator() FOONATHAN_NOEXCEPT = default;

        malloc_allocator& operator=(malloc_allocator &&) FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        /// \effects A \concept{concept_rawallocator,RawAllocator} allocation function.
        /// It uses <tt>std::malloc()</tt> for the allocation.
        /// \returns A pointer to a \concept{concept_node,node}, it will never be \c nullptr.
        /// \throws An exception of type \ref out_of_memory or whatever is thrown by its handler if the allocation fails.
        void* allocate_node(std::size_t size, std::size_t alignment);

        /// \effects A \concept{concept_rawallocator,RawAllocator} deallocation function.
        /// It uses <tt>std::free()</tt>.
        void deallocate_node(void *node, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT;

        /// \returns The maximum node size by forwarding to \c std::allocator<char>::max_size().
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT;
    };

    extern template class allocator_traits<malloc_allocator>;
}} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_MALLOC_ALLOCATOR_HPP_INCLUDED
