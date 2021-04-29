// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_LOWLEVEL_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_LOWLEVEL_ALLOCATOR_HPP_INCLUDED

#include <type_traits>

#include "../config.hpp"
#include "../error.hpp"
#include "align.hpp"
#include "debug_helpers.hpp"
#include "assert.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            template <class Functor>
            struct lowlevel_allocator_leak_handler
            {
                void operator()(std::ptrdiff_t amount)
                {
                    debug_handle_memory_leak(Functor::info(), amount);
                }
            };

            // Functor controls low-level allocation:
            // static allocator_info info()
            // static void* allocate(std::size_t size, std::size_t alignment);
            // static void deallocate(void *memory, std::size_t size, std::size_t alignment);
            // static std::size_t max_node_size();
            template <class Functor>
            class lowlevel_allocator : global_leak_checker<lowlevel_allocator_leak_handler<Functor>>
            {
            public:
                using is_stateful = std::false_type;

                lowlevel_allocator() noexcept {}
                lowlevel_allocator(lowlevel_allocator&&) noexcept {}
                ~lowlevel_allocator() noexcept {}

                lowlevel_allocator& operator=(lowlevel_allocator&&) noexcept
                {
                    return *this;
                }

                void* allocate_node(std::size_t size, std::size_t alignment)
                {
                    auto actual_size = size + (debug_fence_size ? 2 * max_alignment : 0u);

                    auto memory = Functor::allocate(actual_size, alignment);
                    if (!memory)
                        FOONATHAN_THROW(out_of_memory(Functor::info(), actual_size));

                    this->on_allocate(actual_size);

                    return debug_fill_new(memory, size, max_alignment);
                }

                void deallocate_node(void* node, std::size_t size, std::size_t alignment) noexcept
                {
                    auto actual_size = size + (debug_fence_size ? 2 * max_alignment : 0u);

                    auto memory = debug_fill_free(node, size, max_alignment);
                    Functor::deallocate(memory, actual_size, alignment);

                    this->on_deallocate(actual_size);
                }

                std::size_t max_node_size() const noexcept
                {
                    return Functor::max_node_size();
                }
            };

#define FOONATHAN_MEMORY_LL_ALLOCATOR_LEAK_CHECKER(functor, var_name)                              \
    FOONATHAN_MEMORY_GLOBAL_LEAK_CHECKER(lowlevel_allocator_leak_handler<functor>, var_name)
        } // namespace detail
    }     // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_DETAIL_LOWLEVEL_ALLOCATOR_HPP_INCLUDED
