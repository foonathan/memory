// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_LOWLEVEL_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_LOWLEVEL_ALLOCATOR_HPP_INCLUDED

#include <atomic>

#include "../config.hpp"
#include "align.hpp"
#include "debug_helpers.hpp"
#include "../debugging.hpp"
#include "../error.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // Functor controls low-level allocation:
        // static allocator_info info();
        // static void* allocate(std::size_t size, std::size_t alignment);
        // static void deallocate(void *memory, std::size_t size, std::size_t alignment);
        // static std::size_t max_node_size();
        template <class Functor>
        class lowlevel_allocator
        {
        public:
            // create a file-local, thread-local object inside the header
#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
            struct leak_checker
            {
                leak_checker() FOONATHAN_NOEXCEPT
                {
                    ++no_checker_objects_;
                }

                ~leak_checker() FOONATHAN_NOEXCEPT
                {
                    if (--no_checker_objects_ && allocations_ != 0u)
                        get_leak_handler()(Functor::info(), allocations_);
                }
            };
#endif
            using is_stateful = std::false_type;

            lowlevel_allocator() FOONATHAN_NOEXCEPT = default;
            lowlevel_allocator(lowlevel_allocator &&) FOONATHAN_NOEXCEPT {}
            ~lowlevel_allocator() FOONATHAN_NOEXCEPT = default;

            lowlevel_allocator& operator=(lowlevel_allocator &&) FOONATHAN_NOEXCEPT
            {
                return *this;
            }

            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                auto actual_size = size + (debug_fence_size ? 2 * max_alignment : 0u);

                auto memory = Functor::allocate(actual_size, alignment);
                if (!memory)
                    FOONATHAN_THROW(out_of_memory(Functor::info(), actual_size));

                on_allocate(actual_size);

                return debug_fill_new(memory, size, max_alignment);
            }

            void deallocate_node(void *node, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                auto actual_size = size + (debug_fence_size ? 2 * max_alignment : 0u);

                auto memory = debug_fill_free(node, size, max_alignment);
                Functor::deallocate(memory, actual_size, alignment);

                on_deallocate(actual_size);
            }

            std::size_t max_node_size() const FOONATHAN_NOEXCEPT
            {
                return Functor::max_node_size();
            }

        private:
#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
            static std::atomic<std::size_t> no_checker_objects_, allocations_;

            static void on_allocate(std::size_t size) FOONATHAN_NOEXCEPT
            {
                allocations_ += size;
            }

            static void on_deallocate(std::size_t size) FOONATHAN_NOEXCEPT
            {
                allocations_ -= size;
            }
#else
            static void on_allocate(std::size_t) FOONATHAN_NOEXCEPT {}
            static void on_deallocate(std::size_t) FOONATHAN_NOEXCEPT {}

#endif
        };

#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
        template <class Functor>
        std::atomic<std::size_t> lowlevel_allocator<Functor>::no_checker_objects_(0u);

        template <class Functor>
        std::atomic<std::size_t> lowlevel_allocator<Functor>::allocations_(0u);
#endif
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_LOWLEVEL_ALLOCATOR_HPP_INCLUDED
