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
    /// This function only gets called if \ref FOONATHAN_MEMORY_DEBUG_LEAK_CHECK is \c true.
    /// Leak checking is only done through the unified \c RawAllocator interface,
    /// if you use the allocator directly, any leaks are considered on purpose
    /// since you know the type of the allocator and that a leak might not be bad.<br>
    /// The default handler writes the information to \c stderr and continues execution.
    /// \ingroup memory
    using leak_handler = void(*)(const char *name, void *allocator, std::size_t amount);

    /// \brief Exchanges the \ref leak_handler.
    /// \details This function is thread safe.
    /// \ingroup memory
    leak_handler set_leak_handler(leak_handler h);

    /// \brief Returns the current \ref leak_handler.
    /// \ingroup memory
    leak_handler get_leak_handler();

    /// \brief The invalid pointer handler.
    /// \details It will be called when an invalid pointer passed to a deallocate function is detected.
    /// It gets a descriptive string of the allocator, a pointer to the instance (\c nullptr for stateless)
    /// and the invalid pointer.<br>
    /// It must not throw any exceptions since it might be called in the cleanup process.<br>
    /// This function only gets called if \ref FOONATHAN_MEMORY_DEBUG_POINTER_CHECK is \c true.<br>
    /// The default handler writes the information to \c stderr and aborts the program.
    /// \note The instance pointer is just used as identification, it is different for each allocator,
    /// but may refer to subobjects, don't cast it.
    /// \ingroup memory
    using invalid_pointer_handler = void(*)(const char *name, void *allocator, void *ptr);

    /// \brief Exchanges the \ref invalid_pointer_handler.
    /// \details This function is thread safe.
    /// \ingroup memory
    invalid_pointer_handler set_invalid_pointer_handler(invalid_pointer_handler h);

    /// \brief Returns the current \ref invalid_pointer_handler.
    /// \ingroup memory
    invalid_pointer_handler get_invalid_pointer_handler();

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
        constexpr std::size_t debug_fence_size = 0u;

        // no need to use a macro, GCC will already completely remove the code on -O1
        // this includes parameter calculations
        inline void debug_fill(void*, std::size_t, debug_magic) FOONATHAN_NOEXCEPT {}
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
                    get_leak_handler()(name_, this, allocated_);
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
            ~leak_checker() FOONATHAN_NOEXCEPT = default;

            void on_allocate(std::size_t size) FOONATHAN_NOEXCEPT {}
            void on_deallocate(std::size_t size) FOONATHAN_NOEXCEPT {}
        };
    #endif

    #if FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
        #define FOONATHAN_MEMORY_IMPL_POINTER_CHECK(Cond, Name, Alloc, Ptr) \
            if (Cond) {} else {get_invalid_pointer_handler()(Name, Alloc, Ptr);}
    #else
        #define FOONATHAN_MEMORY_IMPL_POINTER_CHECK(Cond, Name, Alloc, Ptr)
    #endif
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DEBUGGING_HPP_INCLUDED
