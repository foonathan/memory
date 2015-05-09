// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
#define FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED

/// \file
/// \brief Methods for debugging errors.

#include <cstring>

#include "config.hpp"

namespace foonathan { namespace memory
{
	/// \brief Magic values used to mark certain memory blocks in debug mode.
    /// \ingroup memory
    enum class debug_magic : unsigned char
    {
        /// \brief Marks allocated, but not yet used memory - "clean memory".
        new_memory = 0xCD,
        /// \brief Marks freed memory - "dead memory".
        freed_memory = 0xDD,
        /// \brief Marks internal memory used by the allocator - "allocated block".
        internal_memory = 0xAB,
    };
    
    namespace detail
    {
    #if FOONATHAN_MEMORY_DEBUG_FILL
    	inline void debug_fill(void *memory, std::size_t size, debug_magic m) FOONATHAN_NOEXCEPT
        {
            std::memset(memory, static_cast<int>(m), size);
        }
    #else
        inline void debug_fill(void*, std::size_t, debug_magic) FOONATHAN_NOEXCEPT {}
    #endif
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
