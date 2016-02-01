// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::heap_allocator and related functions.

#include <type_traits>

#include "config.hpp"
#include "allocator_traits.hpp"

namespace foonathan { namespace memory
{
#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
    namespace detail
    {
        static struct heap_allocator_leak_checker_initializer_t
        {
            heap_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT;
            ~heap_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT;
        } heap_allocator_leak_checker_initializer;
    } // namespace detail
#endif

    /// Allocates heap memory.
    /// This function is used by the \ref heap_allocator to allocate the heap memory.
    /// It is not defined on a freestanding implementation, a definition must be provided by the library user.
    /// \requiredbe This function shall return a block of uninitialized memory that is aligned for \c max_align_t and has the given size.
    /// The size parameter will not be zero.
    /// It shall return a \c nullptr if no memory is available.
    /// It must be thread safe.
    /// \defaultbe On a hosted implementation this function uses OS specific facilities, \c std::malloc is used as fallback.
    void* heap_alloc(std::size_t size) FOONATHAN_NOEXCEPT;

    /// Deallocates heap memory.
    /// This function is used by the \ref heap_allocator to allocate the heap memory.
    /// It is not defined on a freestanding implementation, a definition must be provided by the library user.
    /// \requiredbe This function gets a pointer from a previous call to \ref heap_alloc with the same size.
    /// It shall free the memory.
    /// The pointer will not be zero.
    /// It must be thread safe.
    /// \defaultbe On a hosted implementation this function uses OS specific facilities, \c std::free is used as fallback.
    void heap_dealloc(void *ptr, std::size_t size) FOONATHAN_NOEXCEPT;

    /// A stateless \concept{concept_rawallocator,RawAllocator} that allocates memory from the heap.
    /// It uses the two functions \ref heap_alloc and \ref heap_dealloc for the allocation,
    /// which default to \c std::malloc and \c std::free.
    /// \ingroup memory
    class heap_allocator
    {
    public:
        using is_stateful = std::false_type;

        heap_allocator() FOONATHAN_NOEXCEPT = default;
        heap_allocator(heap_allocator&&) FOONATHAN_NOEXCEPT {}
        ~heap_allocator() FOONATHAN_NOEXCEPT = default;

        heap_allocator& operator=(heap_allocator &&) FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        /// \effects A \concept{concept_rawallocator,RawAllocator} allocation function.
        /// It uses \ref heap_alloc.
        /// \returns A pointer to a \concept{concept_node,node}, it will never be \c nullptr.
        /// \throws An exception of type \ref out_of_memory or whatever is thrown by its handler if \ref heap_alloc returns a \c nullptr.
        void* allocate_node(std::size_t size, std::size_t alignment);

        /// \effects A \concept{concept_rawallocator,RawAllocator} deallocation function.
        /// It uses \ref heap_dealloc.
        void deallocate_node(void *ptr, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT;

        /// \returns The maximum node size by forwarding to \c std::allocator<char>::max_size(), an OS specific facility
        /// or the maximum value on a freestanding implementation.
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT;
    };

    extern template class allocator_traits<heap_allocator>;
}} // namespace foonathan::memory

#endif // PORTAL_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED
