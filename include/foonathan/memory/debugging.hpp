// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
#define FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED

/// \file
/// \brief Methods for debugging errors.

#include <type_traits>

#include "config.hpp"
#include "error.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <cstring>
#endif

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
        /// \details This memory can also serve as \ref fence_memory.
        alignment_memory = 0xED,
        /// \brief Marks buffer memory used to protect against overflow - "fence memory".
        /// \details It is only filled, if \ref FOONATHAN_MEMORY_DEBUG_FENCE is set accordingly.
        fence_memory = 0xFD
    };

    /// \brief The leak handler.
    /// \details It will be called when a memory leak is detected.
    /// It gets the \ref allocator_info and the amount of memory leaked.<br>
    /// It must not throw any exceptions since it is called in the cleanup process.<br>
    /// This function only gets called if \ref FOONATHAN_MEMORY_DEBUG_LEAK_CHECK is \c true.
    /// Leak checking is only done through the unified \c RawAllocator interface,
    /// if you use the allocator directly, any leaks are considered on purpose
    /// since you know the type of the allocator and that a leak might not be bad.<br>
    /// The default handler writes the information to \c stderr and continues execution,
    /// unless on a free standing implementation where it does nothing.
    /// \ingroup memory
    using leak_handler = void(*)(const allocator_info &info, std::size_t amount);

    /// \brief Exchanges the \ref leak_handler.
    /// \details This function is thread safe.
    /// \ingroup memory
    leak_handler set_leak_handler(leak_handler h);

    /// \brief Returns the current \ref leak_handler.
    /// \ingroup memory
    leak_handler get_leak_handler();

    /// \brief The invalid pointer handler.
    /// \details It will be called when an invalid pointer passed to a deallocate function is detected.
    /// It gets the \ref allocator_info and the invalid pointer.<br>
    /// It must not throw any exceptions since it might be called in the cleanup process.<br>
    /// This function only gets called if \ref FOONATHAN_MEMORY_DEBUG_POINTER_CHECK is \c true.<br>
    /// The default handler writes the information to \c stderr and aborts the program,
    /// unless on a freestanding implementation where it only aborts.
    /// \note The instance pointer is just used as identification, it is different for each allocator,
    /// but may refer to subobjects, don't cast it.
    /// \ingroup memory
    using invalid_pointer_handler = void(*)(const allocator_info &info, const void *ptr);

    /// \brief Exchanges the \ref invalid_pointer_handler.
    /// \details This function is thread safe.
    /// \ingroup memory
    invalid_pointer_handler set_invalid_pointer_handler(invalid_pointer_handler h);

    /// \brief Returns the current \ref invalid_pointer_handler.
    /// \ingroup memory
    invalid_pointer_handler get_invalid_pointer_handler();

    /// \brief The buffer overflow handler.
    /// \details It will be called when a buffer overflow (or underflow) on allocated memory is detected when it is freed again.
    /// It gets the pointer to the memory block, its size and the position where it has been detected.<br>
    /// It must not throw any exceptions since it might be called in the cleanup process.<br>
    /// This function only gets called if \ref FOONATHAN_MEMORY_DEBUG_FENCE has a non-zero value
    /// and \ref FOONATHAN_MEMORY_DEBUG_FILL is \c true.<br>
    /// The default handler writes the information to \c stderr and aborts the program,
    /// unless on a freestanding implementation where it only aborts.
    /// \note The instance pointer is just used as identification, it is different for each allocator,
    /// but may refer to subobjects, don't cast it.
    /// \ingroup memory
    using buffer_overflow_handler = void(*)(const void *memory, std::size_t size, const void *write_ptr);

    /// \brief Exchanges the \ref buffer_overflow_handler.
    /// \details This function is thread safe.
    /// \ingroup memory
    buffer_overflow_handler set_buffer_overflow_handler(buffer_overflow_handler h);

    /// \brief Returns the current \ref buffer_overflow_handler.
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
