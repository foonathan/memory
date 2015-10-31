// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_VIRTUAL_MEMORY_HPP_INCLUDED
#define FOONATHAN_MEMORY_VIRTUAL_MEMORY_HPP_INCLUDED

#include <cstddef>
#include <type_traits>

#include "config.hpp"

namespace foonathan { namespace memory
{
#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
    namespace detail
    {
        static struct virtual_memory_allocator_leak_checker_initializer_t
        {
            virtual_memory_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT;
            ~virtual_memory_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT;
        } virtual_memory_allocator_leak_checker_initializer;
    } // namespace detail
#endif

    /// The page size of the virtual memory.
    /// All virtual memory allocations must be multiple of this size.
    /// It is usually 4KiB.
    /// \ingroup memory
    extern const std::size_t virtual_memory_page_size;

    /// Reserves virtual memory.
    /// \effects Reserves the given number of pages.
    /// Each page is \ref virtual_memory_page_size big.
    /// \returns The address of the first reserved page,
    /// or \c nullptr in case of error.
    /// \note The memory may not be used, it must first be commited.
    /// \ingroup memory
    void* virtual_memory_reserve(std::size_t no_pages) FOONATHAN_NOEXCEPT;

    /// Releases reserved virtual memory.
    /// \effects Returns previously reserved pages to the system.
    /// \requires \c pages must come from a previous call to \ref virtual_memory_reserve with the same \c calc_no_pages,
    /// it must not be \c nullptr.
    /// \ingroup memory
    void virtual_memory_release(void *pages, std::size_t no_pages) FOONATHAN_NOEXCEPT;

    /// Commits reserved virtual memory.
    /// \effects Marks \c calc_no_pages pages starting at the given address available for use.
    /// \returns The beginning of the committed area, i.e. \c memory, or \c nullptr in case of error.
    /// \requires The memory must be previously reserved.
    /// \ingroup memory
    void* virtual_memory_commit(void *memory, std::size_t no_pages) FOONATHAN_NOEXCEPT;

    /// Decommits commited virtual memory.
    /// \effects Puts commited memory back in the reserved state.
    /// \requires \c memory must come from a previous call to \ref virtual_memory_commit with the same \c calc_no_pages
    /// it must not be \c nullptr.
    /// \ingroup memory
    void virtual_memory_decommit(void *memory, std::size_t no_pages) FOONATHAN_NOEXCEPT;

    /// A stateless \concept{concept_rawallocator,RawAllocator} that allocates memory using the virtual memory allocation functions.
    /// It does not prereserve any memory and will always reserve and commit combined.
    /// \ingroup memory
    class virtual_memory_allocator
    {
    public:
        using is_stateful = std::false_type;

        virtual_memory_allocator() FOONATHAN_NOEXCEPT = default;
        virtual_memory_allocator(virtual_memory_allocator&&) FOONATHAN_NOEXCEPT {}
        ~virtual_memory_allocator() FOONATHAN_NOEXCEPT = default;

        virtual_memory_allocator& operator=(virtual_memory_allocator &&) FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        /// \effects A \concept{concept_rawallocator,RawAllocator} allocation function.
        /// It uses \ref virtual_memory_reserve followed by \ref virtual_memory_commit for the allocation.
        /// The number of pages allocated will be the minimum to hold \c size continuous bytes,
        /// i.e. \c size will be rounded up to the next multiple.
        /// If debug fences are activated, one additional page before and after the memory will be allocated.
        /// \returns A pointer to a \concept{concept_node,node}, it will never be \c nullptr.
        /// It will always be aligned on a fence boundary, regardless of the alignment parameter.
        /// \throws An exception of type \ref out_of_memory or whatever is thrown by its handler if the allocation fails.
        void* allocate_node(std::size_t size, std::size_t alignment);

        /// \effects A \concept{concept_rawallocator,RawAllocator} deallocation function.
        /// It calls \ref virtual_memory_decommit followed by \ref virtual_memory_release for the deallocation.
        void deallocate_node(void *node, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT;

        /// \returns The maximum node size by returning the maximum value.
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT;

        /// \returns The maximum alignment which is the same as the \ref virtual_memory_page_size.
        std::size_t max_alignment() const FOONATHAN_NOEXCEPT;
    };
}} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_VIRTUAL_MEMORY_HPP_INCLUDED
