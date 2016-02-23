// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_MEMORY_STACK_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_MEMORY_STACK_HPP_INCLUDED

#include <cstddef>

#include "../config.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // simple memory stack implementation that does not support growing
        class fixed_memory_stack
        {
        public:
            fixed_memory_stack() FOONATHAN_NOEXCEPT
            : fixed_memory_stack(nullptr) {}

            // gives it the current pointer, the end pointer must be maintained seperataly
            explicit fixed_memory_stack(void *memory) FOONATHAN_NOEXCEPT
            : cur_(static_cast<char*>(memory)) {}

            fixed_memory_stack(fixed_memory_stack &&other) FOONATHAN_NOEXCEPT;

            ~fixed_memory_stack() FOONATHAN_NOEXCEPT = default;

            fixed_memory_stack& operator=(fixed_memory_stack &&other) FOONATHAN_NOEXCEPT;

            // allocates memory by advancing the stack, returns nullptr if insufficient
            // debug: mark memory as new_memory, put fence in front and back
            void* allocate(const char *end, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT;

            // unwindws the stack to a certain older position
            // debug: marks memory from new top to old top as freed
            // doesn't check for invalid pointer
            void unwind(char *top) FOONATHAN_NOEXCEPT;

            // returns the current top
            char* top() const FOONATHAN_NOEXCEPT
            {
                return cur_;
            }

        private:
            char *cur_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_MEMORY_STACK_HPP_INCLUDED
