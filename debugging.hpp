// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
#define FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED

/// \file
/// \brief Methods for debugging errors.

#include <cstring>
#include <type_traits>

#include "config.hpp"

namespace foonathan { namespace memory
{
    /// \brief Magic values used to mark certain memory blocks in debug mode.
    /// \ingroup memory
    enum class debug_magic : unsigned char
    {
        /// \brief Marks internal memory used by the allocator - "allocated block".
        internal_memory = 0xAB,
        /// \brief Marks allocated, but not yet used memory - "clean memory".
        new_memory = 0xCD,
        /// \brief Marks freed memory - "dead memory".
        freed_memory = 0xDD,
        /// \brief Marks buffer memory used to ensure proper alignment.
        /// \detail This memory can also serve as \ref fence_memory.
        alignment_memory = 0xED,
        /// \brief Marks buffer memory used to protect against overflow - "fence memory".
        /// \details It is only filled, if \ref FOONATHAN_MEMORY_DEBUG_FENCE is set accordingly.
        fence_memory = 0xFD,
    };
    
    /// \brief The leak handler.
    /// \details It will be called when a memory leak is detected.
    /// It gets a descriptive string of the allocator, a pointer to the allocator instance (\c nullptr for stateless)
    /// and the amount of memory leaked.<br>
    /// It must not throw any exceptions since it is called in the cleanup process.<br>
    /// This function only gets called if \ref FOONATHAN_MEMORY_DEBUG_LEAK_CHECK is \c true.<br>
    /// The default handler writes the information to \c stderr and continues execution.
    using leak_handler = void(*)(const char *name, void *allocator, std::size_t amount);
    
    /// \brief Exchanges the \ref leak_handler.
    /// \details This function is thread safe.
    leak_handler set_leak_handler(leak_handler h);
    
    /// \brief Returns the current \ref leak_handler.
    leak_handler get_leak_handler();

    namespace detail
    {
    #if FOONATHAN_MEMORY_DEBUG_FILL
        using debug_fill_enabled = std::true_type;
        constexpr std::size_t debug_fence_size = FOONATHAN_MEMORY_DEBUG_FENCE;

        inline void debug_fill(void *memory, std::size_t size, debug_magic m) FOONATHAN_NOEXCEPT
        {
            std::memset(memory, static_cast<int>(m), size);
        }
    #else
        using debug_fill_enabled = std::false_type;
        constexpr std::size_T debug_fence_size = 0u;

        // no need to use a macro, GCC will already completely remove the code on -O1
        // this includes parameter calculations
        inline void debug_fill(void*, std::size_t, debug_magic) FOONATHAN_NOEXCEPT {}
    #endif
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
