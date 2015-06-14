// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A heap allocator.

#include <type_traits>

#include "config.hpp"
#include "raw_allocator_base.hpp"

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

    /// \brief A \ref concept::RawAllocator that allocates memory via std::malloc/free.
    /// \ingroup memory
    class heap_allocator : public raw_allocator_base<heap_allocator>
    {
    public:
        using is_stateful = std::false_type;

        //=== allocation/deallocation ===//
        /// \brief Allocates raw memory from the heap.
        /// \details If it fails, it behaves like \c ::operator \c new
        /// (retry in loop calling \c std::new_handler, etc.),
        /// but if the new handler is \c null, it calls the \ref out_of_memory_handler
        /// prior to throwing \c std::bad_alloc.
        void* allocate_node(std::size_t size, std::size_t alignment);

        /// \brief Deallocates raw memory.
        void deallocate_node(void *ptr, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT;
    };
}} // namespace foonathan::memory

#endif // PORTAL_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED
