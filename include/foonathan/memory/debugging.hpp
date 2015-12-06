// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
#define FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED

/// \file
/// Debugging facilities.

#include <type_traits>

#include "config.hpp"
#include "error.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <cstring>
#endif

namespace foonathan { namespace memory
{
    /// The magic values that are used for debug filling.
    /// If \ref FOONATHAN_MEMORY_DEBUG_FILL is \c true, memory will be filled to help detect use-after-free or missing initialization errors.
    /// These are the constants for the different types.
    /// \ingroup memory
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
    /// \requiredbe A leak handler shall log the leak, abort the program, do nothing or anything else that seems appropriate.
    /// It must not throw any exceptions since it is called in the cleanup process.
    /// \defaultbe On a hosted implementation it logs the leak to \c stderr and returns, continuing execution.
    /// On a freestanding implementation it does nothing.
    /// \ingroup memory
    using leak_handler = void(*)(const allocator_info &info, std::size_t amount);

    /// Exchanges the \ref leak_handler.
    /// \effects Sets \c h as the new \ref leak_handler in an atomic operation.
    /// A \c nullptr sets the default \ref leak_handler.
    /// \returns The previous \ref leak_handler. This is never \c nullptr.
    /// \ingroup memory
    leak_handler set_leak_handler(leak_handler h);

    /// Returns the \ref leak_handler.
    /// \returns The current \ref leak_handler. This is never \c nullptr.
    /// \ingroup memory
    leak_handler get_leak_handler();

    /// The type of the handler called when an invalid pointer is passed to a deallocation function.
    /// Pointer checking can be controlled via the options \ref FOONATHAN_MEMORY_DEBUG_POINTER_CHECK and \ref FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK.
    /// The handler gets the \ref allocator_info and the invalid pointer.
    /// \requiredbe An invalid pointer handler shall terminate the program.
    /// It must not throw any exceptions since it might be called in the cleanup process.
    /// \defaultbe On a hosted implementation it logs the information to \c stderr and calls \c std::abort().
    /// On a freestanding implementation it only calls \c std::abort().
    /// \ingroup memory
    using invalid_pointer_handler = void(*)(const allocator_info &info, const void *ptr);

    /// Exchanges the \ref invalid_pointer_handler.
    /// \effects Sets \c h as the new \ref invalid_pointer_handler in an atomic operation.
    /// A \c nullptr sets the default \ref invalid_pointer_handler.
    /// \returns The previous \ref invalid_pointer_handler. This is never \c nullptr.
    /// \ingroup memory
    invalid_pointer_handler set_invalid_pointer_handler(invalid_pointer_handler h);

    /// Returns the \ref invalid_pointer_handler.
    /// \returns The current \ref invalid_pointer_handler. This is never \c nullptr.
    /// \ingroup memory
    invalid_pointer_handler get_invalid_pointer_handler();

    /// The type of the handler called when a buffer under/overflow is detected.
    /// If \ref FOONATHAN_MEMORY_DEBUG_FILL is \c true and \ref FOONATHAN_MEMORY_DEBUG_FENCE has a non-zero value
    /// the allocator classes check if a write into the fence has occured upon deallocation.
    /// The handler gets the memory block belonging to the corrupted fence, its size and the exact address.
    /// \requiredbe A buffer overflow handler shall terminate the program.
    /// It must not throw any exceptions since it me be called in the cleanup process.
    /// \defaultbe On a hosted implementation it logs the information to \c stderr and calls \c std::abort().
    /// On a freestanding implementation it only calls \c std::abort().
    /// \ingroup memory
    using buffer_overflow_handler = void(*)(const void *memory, std::size_t size, const void *write_ptr);

    /// Exchanges the \ref buffer_overflow_handler.
    /// \effects Sets \c h as the new \ref buffer_overflow_handler in an atomic operation.
    /// A \c nullptr sets the default \ref buffer_overflow_handler.
    /// \returns The previous \ref buffer_overflow_handler. This is never \c nullptr.
    /// \ingroup memory
    buffer_overflow_handler set_buffer_overflow_handler(buffer_overflow_handler h);

    /// Returns the \ref buffer_overflow_handler.
    /// \returns The current \ref buffer_overflow_handler. This is never \c nullptr.
    /// \ingroup memory
    buffer_overflow_handler get_buffer_overflow_handler();

    namespace detail
    {
    #if FOONATHAN_MEMORY_DEBUG_FILL
        using debug_fill_enabled = std::true_type;
        FOONATHAN_CONSTEXPR std::size_t debug_fence_size = FOONATHAN_MEMORY_DEBUG_FENCE;

        inline void debug_fill(void *memory, std::size_t size, debug_magic m) FOONATHAN_NOEXCEPT
        {
        #if FOONATHAN_HOSTED_IMPLEMENTATION
            std::memset(memory, static_cast<int>(m), size);
        #else
            // do the naive loop :(
            auto ptr = static_cast<unsigned char*>(memory);
            for (std::size_t i = 0u; i != size; ++i)
                *ptr++ = static_cast<unsigned char>(m);
        #endif
        }

        // fills fence, new and fence
        // returns after fence
        inline void* debug_fill_new(void *memory, std::size_t node_size,
                                    std::size_t fence_size = debug_fence_size) FOONATHAN_NOEXCEPT
        {
            if (!debug_fence_size)
                fence_size = 0u;
            auto mem = static_cast<char*>(memory);
            debug_fill(mem, fence_size, debug_magic::fence_memory);
            mem += fence_size;
            debug_fill(mem, node_size, debug_magic::new_memory);
            debug_fill(mem + node_size, fence_size, debug_magic::fence_memory);
            return mem;
        }

        // fills free memory and returns memory starting at fence
        inline char* debug_fill_free(void *memory, std::size_t node_size,
                                    std::size_t fence_size = debug_fence_size) FOONATHAN_NOEXCEPT
        {
            if (!debug_fence_size)
                fence_size = 0u;

            debug_fill(memory, node_size, debug_magic::freed_memory);

            auto pre_fence = static_cast<unsigned char*>(memory) - fence_size;
            for (auto cur = pre_fence; cur != static_cast<unsigned char*>(memory); ++cur)
                if (*cur != static_cast<unsigned char>(debug_magic::fence_memory))
                    get_buffer_overflow_handler()(memory, node_size, cur);

            auto end = static_cast<unsigned char*>(memory) + node_size;
            auto post_fence = end + fence_size;
            for (auto cur = end; cur != post_fence; ++cur)
                if (*cur != static_cast<unsigned char>(debug_magic::fence_memory))
                    get_buffer_overflow_handler()(memory, node_size, cur);

            return static_cast<char*>(memory) - fence_size;
        }
    #else
        using debug_fill_enabled = std::false_type;
        FOONATHAN_CONSTEXPR std::size_t debug_fence_size = 0u;

        // no need to use a macro, GCC will already completely remove the code on -O1
        // this includes parameter calculations
        inline void debug_fill(void*, std::size_t, debug_magic) FOONATHAN_NOEXCEPT {}

        inline void* debug_fill_new(void *memory, std::size_t, std::size_t = 0u) FOONATHAN_NOEXCEPT
        {
            return memory;
        }

        inline char* debug_fill_free(void *memory, std::size_t, std::size_t = 0u) FOONATHAN_NOEXCEPT
        {
            return static_cast<char*>(memory);
        }
    #endif

    // base class that performs the leak check
    // only for stateful allocators
    // it is a template to allow per class storage of the strings
    // allocators that give different strings need different parameters
    #if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
        template <class RawAllocator>
        class leak_checker
        {
        protected:
            leak_checker(const char *name) FOONATHAN_NOEXCEPT
            : allocated_(0)
            {
                name_ = name;
            }

            leak_checker(leak_checker &&other) FOONATHAN_NOEXCEPT
            : allocated_(other.allocated_)
            {
                other.allocated_ = 0u;
            }

            ~leak_checker() FOONATHAN_NOEXCEPT
            {
                if (allocated_ != 0u)
                    get_leak_handler()({name_, this}, allocated_);
            }

            leak_checker& operator=(leak_checker &&other) FOONATHAN_NOEXCEPT
            {
                allocated_ = other.allocated_;
                other.allocated_ = 0u;
                return *this;
            }

            void on_allocate(std::size_t size) FOONATHAN_NOEXCEPT
            {
                allocated_ += size;
            }

            void on_deallocate(std::size_t size) FOONATHAN_NOEXCEPT
            {
                allocated_ -= size;
            }

        private:
            static const char* name_;
            std::size_t allocated_;
        };

        template <class RawAllocator>
        const char* detail::leak_checker<RawAllocator>::name_ = "";
    #else
        template <class RawAllocator>
        class leak_checker
        {
        protected:
            leak_checker(const char *) FOONATHAN_NOEXCEPT {}
            leak_checker(leak_checker &&) FOONATHAN_NOEXCEPT {}
            ~leak_checker() FOONATHAN_NOEXCEPT {}

            leak_checker& operator=(leak_checker &&) FOONATHAN_NOEXCEPT {return *this;}

            void on_allocate(std::size_t) FOONATHAN_NOEXCEPT {}
            void on_deallocate(std::size_t) FOONATHAN_NOEXCEPT {}
        };
    #endif

    #if FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
        inline void check_pointer(bool condition, const allocator_info &info, void *ptr)
        {
            if (!condition)
                get_invalid_pointer_handler()(info, ptr);
        }
    #else
        inline void check_pointer(bool, const allocator_info&, void *) FOONATHAN_NOEXCEPT {}
    #endif
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
