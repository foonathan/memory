// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::new_allocator.

#include <type_traits>

#include "config.hpp"

namespace foonathan { namespace memory
{
#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
    namespace detail
    {
        static struct new_allocator_leak_checker_initializer_t
        {
            new_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT;
            ~new_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT;
        } new_allocator_leak_checker_initializer;
    } // namespace detail
#endif

    /// A stateless \concept{concept_rawallocator,RawAllocator} that allocates memory using <tt>operator new</tt>.
    /// \ingroup memory
    class new_allocator
    {
    public:
        using is_stateful = std::false_type;

        new_allocator() FOONATHAN_NOEXCEPT = default;
        new_allocator(new_allocator&&) FOONATHAN_NOEXCEPT {}
        ~new_allocator() FOONATHAN_NOEXCEPT = default;

        new_allocator& operator=(new_allocator &&) FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        /// \effects A \concept{concept_rawallocator,RawAllocator} allocation function.
        /// It uses the nothrow <tt>operator new</tt>,
        /// if it returns \c nullptr, it behaves like \c new and loops calling \c std::new_handler,
        /// but instead of throwing \c std::bad_alloc, it throws \ref out_of_memory at the end.
        /// \returns A pointer to a \concept{concept_node,node}, it will never be \c nullptr.
        /// \throws An exception of type \ref out_of_memory or whatever is thrown by its handler if the allocation fails.
        void* allocate_node(std::size_t size, std::size_t alignment);

        /// \effects A \concept{concept_rawallocator,RawAllocator} deallocation function.
        /// It uses <tt>operator delete</tt>.
        void deallocate_node(void *node, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT;

        /// \returns The maximum node size by forwarding to \c std::allocator<char>::max_size()
        /// or the maximum value on a freestanding implementation.
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT;
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED
