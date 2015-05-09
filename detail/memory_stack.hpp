// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_MEMORY_STACK_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_MEMORY_STACK_HPP_INCLUDED

#include <cstddef>

#include "align.hpp"
#include "block_list.hpp"

namespace foonathan { namespace memory
{
	namespace detail
    {
    	// simple memory stack implementation that does not support growing
        class fixed_memory_stack
        {
        public:
            // gives it a memory block
            fixed_memory_stack(void *memory, std::size_t size) FOONATHAN_NOEXCEPT
            : cur_(static_cast<char*>(memory)), end_(cur_ + size) {}
            
            fixed_memory_stack(block_info info) FOONATHAN_NOEXCEPT
            : fixed_memory_stack(info.memory, info.size) {}
            
            fixed_memory_stack() FOONATHAN_NOEXCEPT
            : fixed_memory_stack(nullptr, 0) {}
            
            // allocates memory by advancing the stack, returns nullptr if insufficient
            void* allocate(std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                auto offset = align_offset(cur_, alignment);
                if (std::ptrdiff_t(offset + size) > end_ - cur_)
                    return nullptr;
                cur_ += offset;
                auto memory = cur_;
                cur_ += size;
                return memory;
            }
            
            // returns the current top
            char* top() const FOONATHAN_NOEXCEPT
            {
                return cur_;
            }
            
            // returns the end of the stack
            const char* end() const FOONATHAN_NOEXCEPT
            {
                return end_;
            }
            
        private:
            char *cur_, *end_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_MEMORY_STACK_HPP_INCLUDED
