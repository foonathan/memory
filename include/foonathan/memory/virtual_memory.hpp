// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_VIRTUAL_MEMORY_HPP_INCLUDED
#define FOONATHAN_MEMORY_VIRTUAL_MEMORY_HPP_INCLUDED

#include <cstddef>

#include "config.hpp"

namespace foonathan { namespace memory
{
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
    /// \requires \c pages must come from a previous call to \ref virtual_memory_reserve with the same \c no_pages,
    /// it must not be \c nullptr.
    /// \ingroup memory
    void virtual_memory_release(void *pages, std::size_t no_pages) FOONATHAN_NOEXCEPT;

    /// Commits reserved virtual memory.
    /// \effects Marks \c no_pages pages starting at the given address available for use.
    /// \returns The beginning of the commited area, i.e. \c memory, or \c nullptr in case of error.
    /// \ingroup memory
    void* virtual_memory_commit(void *memory, std::size_t no_pages) FOONATHAN_NOEXCEPT;

    /// Decommits commited virtual memory.
    /// \effects Puts commited memory back in the reserved state.
    /// \requires \c memory must come from a previous call to \ref virtual_memory_commit with the same \c no_pages
    /// it must not be \c nullptr.
    /// \ingroup memory
    void virtual_memory_decommit(void *memory, std::size_t no_pages) FOONATHAN_NOEXCEPT;
}} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_VIRTUAL_MEMORY_HPP_INCLUDED
