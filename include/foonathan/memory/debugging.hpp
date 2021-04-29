// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
#define FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED

/// \file
/// Debugging facilities.

#include "config.hpp"

namespace foonathan
{
    namespace memory
    {
        struct allocator_info;

        /// The magic values that are used for debug filling.
        /// If \ref FOONATHAN_MEMORY_DEBUG_FILL is \c true, memory will be filled to help detect use-after-free or missing initialization errors.
        /// These are the constants for the different types.
        /// \ingroup core
        enum class debug_magic : unsigned char
        {
            /// Marks internal memory used by the allocator - "allocated block".
            internal_memory = 0xAB,
            /// Marks internal memory currently not used by the allocator - "freed block".
            internal_freed_memory = 0xFB,
            /// Marks allocated, but not yet used memory - "clean memory".
            new_memory = 0xCD,
            /// Marks freed memory - "dead memory".
            freed_memory = 0xDD,
            /// Marks buffer memory used to ensure proper alignment.
            /// This memory can also serve as \ref debug_magic::fence_memory.
            alignment_memory = 0xED,
            /// Marks buffer memory used to protect against overflow - "fence memory".
            /// The option \ref FOONATHAN_MEMORY_DEBUG_FENCE controls the size of a memory fence that will be placed before or after a memory block.
            /// It helps catching buffer overflows.
            fence_memory = 0xFD
        };

        /// The type of the handler called when a memory leak is detected.
        /// Leak checking can be controlled via the option \ref FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
        /// and only affects calls through the \ref allocator_traits, not direct calls.
        /// The handler gets the \ref allocator_info and the amount of memory leaked.
        /// This can also be negative, meaning that more memory has been freed than allocated.
        /// \requiredbe A leak handler shall log the leak, abort the program, do nothing or anything else that seems appropriate.
        /// It must not throw any exceptions since it is called in the cleanup process.
        /// \defaultbe On a hosted implementation it logs the leak to \c stderr and returns, continuing execution.
        /// On a freestanding implementation it does nothing.
        /// \ingroup core
        using leak_handler = void (*)(const allocator_info& info, std::ptrdiff_t amount);

        /// Exchanges the \ref leak_handler.
        /// \effects Sets \c h as the new \ref leak_handler in an atomic operation.
        /// A \c nullptr sets the default \ref leak_handler.
        /// \returns The previous \ref leak_handler. This is never \c nullptr.
        /// \ingroup core
        leak_handler set_leak_handler(leak_handler h);

        /// Returns the \ref leak_handler.
        /// \returns The current \ref leak_handler. This is never \c nullptr.
        /// \ingroup core
        leak_handler get_leak_handler();

        /// The type of the handler called when an invalid pointer is passed to a deallocation function.
        /// Pointer checking can be controlled via the options \ref FOONATHAN_MEMORY_DEBUG_POINTER_CHECK and \ref FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK.
        /// The handler gets the \ref allocator_info and the invalid pointer.
        /// \requiredbe An invalid pointer handler shall terminate the program.
        /// It must not throw any exceptions since it might be called in the cleanup process.
        /// \defaultbe On a hosted implementation it logs the information to \c stderr and calls \c std::abort().
        /// On a freestanding implementation it only calls \c std::abort().
        /// \ingroup core
        using invalid_pointer_handler = void (*)(const allocator_info& info, const void* ptr);

        /// Exchanges the \ref invalid_pointer_handler.
        /// \effects Sets \c h as the new \ref invalid_pointer_handler in an atomic operation.
        /// A \c nullptr sets the default \ref invalid_pointer_handler.
        /// \returns The previous \ref invalid_pointer_handler. This is never \c nullptr.
        /// \ingroup core
        invalid_pointer_handler set_invalid_pointer_handler(invalid_pointer_handler h);

        /// Returns the \ref invalid_pointer_handler.
        /// \returns The current \ref invalid_pointer_handler. This is never \c nullptr.
        /// \ingroup core
        invalid_pointer_handler get_invalid_pointer_handler();

        /// The type of the handler called when a buffer under/overflow is detected.
        /// If \ref FOONATHAN_MEMORY_DEBUG_FILL is \c true and \ref FOONATHAN_MEMORY_DEBUG_FENCE has a non-zero value
        /// the allocator classes check if a write into the fence has occured upon deallocation.
        /// The handler gets the memory block belonging to the corrupted fence, its size and the exact address.
        /// \requiredbe A buffer overflow handler shall terminate the program.
        /// It must not throw any exceptions since it me be called in the cleanup process.
        /// \defaultbe On a hosted implementation it logs the information to \c stderr and calls \c std::abort().
        /// On a freestanding implementation it only calls \c std::abort().
        /// \ingroup core
        using buffer_overflow_handler = void (*)(const void* memory, std::size_t size,
                                                 const void* write_ptr);

        /// Exchanges the \ref buffer_overflow_handler.
        /// \effects Sets \c h as the new \ref buffer_overflow_handler in an atomic operation.
        /// A \c nullptr sets the default \ref buffer_overflow_handler.
        /// \returns The previous \ref buffer_overflow_handler. This is never \c nullptr.
        /// \ingroup core
        buffer_overflow_handler set_buffer_overflow_handler(buffer_overflow_handler h);

        /// Returns the \ref buffer_overflow_handler.
        /// \returns The current \ref buffer_overflow_handler. This is never \c nullptr.
        /// \ingroup core
        buffer_overflow_handler get_buffer_overflow_handler();
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
