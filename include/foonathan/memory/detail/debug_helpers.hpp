// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DEBUG_HELPERS_HPP_INCLUDED
#define FOONATHAN_MEMORY_DEBUG_HELPERS_HPP_INCLUDED

#include <atomic>
#include <type_traits>

#include "../config.hpp"

namespace foonathan
{
    namespace memory
    {
        enum class debug_magic : unsigned char;
        struct allocator_info;

        namespace detail
        {
            using debug_fill_enabled = std::integral_constant<bool, FOONATHAN_MEMORY_DEBUG_FILL>;
            constexpr std::size_t debug_fence_size =
                FOONATHAN_MEMORY_DEBUG_FILL ? FOONATHAN_MEMORY_DEBUG_FENCE : 0u;

#if FOONATHAN_MEMORY_DEBUG_FILL
            // fills size bytes of memory with debug_magic
            void debug_fill(void* memory, std::size_t size, debug_magic m) noexcept;

            // returns nullptr if memory is filled with debug_magic
            // else returns pointer to mismatched byte
            void* debug_is_filled(void* memory, std::size_t size, debug_magic m) noexcept;

            // fills fence, new and fence
            // returns after fence
            void* debug_fill_new(void* memory, std::size_t node_size,
                                 std::size_t fence_size = debug_fence_size) noexcept;

            // fills free memory and returns memory starting at fence
            void* debug_fill_free(void* memory, std::size_t node_size,
                                  std::size_t fence_size = debug_fence_size) noexcept;

            // fills internal memory
            void debug_fill_internal(void* memory, std::size_t size, bool free) noexcept;
#else
            inline void debug_fill(void*, std::size_t, debug_magic) noexcept {}

            inline void* debug_is_filled(void*, std::size_t, debug_magic) noexcept
            {
                return nullptr;
            }

            inline void* debug_fill_new(void* memory, std::size_t, std::size_t) noexcept
            {
                return memory;
            }

            inline void* debug_fill_free(void* memory, std::size_t, std::size_t) noexcept
            {
                return static_cast<char*>(memory);
            }

            inline void debug_fill_internal(void*, std::size_t, bool) noexcept {}
#endif

            void debug_handle_invalid_ptr(const allocator_info& info, void* ptr);

            // validates given ptr by evaluating the Functor
            // if the Functor returns false, calls the debug_leak_checker
            // note: ptr is just used as the information passed to the invalid ptr handler
            template <class Functor>
            void debug_check_pointer(Functor condition, const allocator_info& info, void* ptr)
            {
#if FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
                if (!condition())
                    debug_handle_invalid_ptr(info, ptr);
#else
                (void)ptr;
                (void)condition;
                (void)info;
#endif
            }

            // validates ptr by using a more expensive double-dealloc check
            template <class Functor>
            void debug_check_double_dealloc(Functor condition, const allocator_info& info,
                                            void* ptr)
            {
#if FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK
                debug_check_pointer(condition, info, ptr);
#else
                (void)condition;
                (void)info;
                (void)ptr;
#endif
            }

            void debug_handle_memory_leak(const allocator_info& info, std::ptrdiff_t amount);

            // does no leak checking, null overhead
            template <class Handler>
            class no_leak_checker
            {
            public:
                no_leak_checker() noexcept {}
                no_leak_checker(no_leak_checker&&) noexcept {}
                ~no_leak_checker() noexcept {}

                no_leak_checker& operator=(no_leak_checker&&) noexcept
                {
                    return *this;
                }

                void on_allocate(std::size_t) noexcept {}
                void on_deallocate(std::size_t) noexcept {}
            };

            // does leak checking per-object
            // leak is detected upon destructor
            template <class Handler>
            class object_leak_checker : Handler
            {
            public:
                object_leak_checker() noexcept : allocated_(0) {}

                object_leak_checker(object_leak_checker&& other) noexcept
                : allocated_(other.allocated_)
                {
                    other.allocated_ = 0;
                }

                ~object_leak_checker() noexcept
                {
                    if (allocated_ != 0)
                        this->operator()(allocated_);
                }

                object_leak_checker& operator=(object_leak_checker&& other) noexcept
                {
                    allocated_       = other.allocated_;
                    other.allocated_ = 0;
                    return *this;
                }

                void on_allocate(std::size_t size) noexcept
                {
                    allocated_ += std::ptrdiff_t(size);
                }

                void on_deallocate(std::size_t size) noexcept
                {
                    allocated_ -= std::ptrdiff_t(size);
                }

            private:
                std::ptrdiff_t allocated_;
            };

            // does leak checking on a global basis
            // call macro FOONATHAN_MEMORY_GLOBAL_LEAK_CHECKER(handler, var_name) in the header
            // when last counter gets destroyed, leak is detected
            template <class Handler>
            class global_leak_checker_impl
            {
            public:
                struct counter : Handler
                {
                    counter()
                    {
                        ++no_counter_objects_;
                    }

                    ~counter()
                    {
                        --no_counter_objects_;
                        if (no_counter_objects_ == 0u && allocated_ != 0u)
                            this->operator()(allocated_);
                    }
                };

                global_leak_checker_impl() noexcept {}
                global_leak_checker_impl(global_leak_checker_impl&&) noexcept {}
                ~global_leak_checker_impl() noexcept {}

                global_leak_checker_impl& operator=(global_leak_checker_impl&&) noexcept
                {
                    return *this;
                }

                void on_allocate(std::size_t size) noexcept
                {
                    allocated_ += std::ptrdiff_t(size);
                }

                void on_deallocate(std::size_t size) noexcept
                {
                    allocated_ -= std::ptrdiff_t(size);
                }

            private:
                static std::atomic<std::size_t>    no_counter_objects_;
                static std::atomic<std::ptrdiff_t> allocated_;
            };

            template <class Handler>
            std::atomic<std::size_t> global_leak_checker_impl<Handler>::no_counter_objects_(0u);

            template <class Handler>
            std::atomic<std::ptrdiff_t> global_leak_checker_impl<Handler>::allocated_(0);

#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
            template <class Handler>
            using global_leak_checker = global_leak_checker_impl<Handler>;

#define FOONATHAN_MEMORY_GLOBAL_LEAK_CHECKER(handler, var_name)                                    \
    static foonathan::memory::detail::global_leak_checker<handler>::counter var_name;
#else
            template <class Handler>
            using global_leak_checker = no_leak_checker<int>; // only one instantiation

#define FOONATHAN_MEMORY_GLOBAL_LEAK_CHECKER(handler, var_name)
#endif

#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
            template <class Handler>
            using default_leak_checker = object_leak_checker<Handler>;
#else
            template <class Handler>
            using default_leak_checker = no_leak_checker<Handler>;
#endif
        } // namespace detail
    }     // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_DEBUG_HELPERS_HPP_INCLUDED
