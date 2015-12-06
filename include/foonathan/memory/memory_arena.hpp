// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED

#include "detail/utility.hpp"
#include "config.hpp"

namespace foonathan { namespace memory
{
    struct memory_block
    {
        void *memory;
        std::size_t size;

        memory_block() FOONATHAN_NOEXCEPT
        : memory_block(nullptr, size) {}

        memory_block(void *mem, std::size_t size) FOONATHAN_NOEXCEPT
        : memory(mem), size(size) {}

        memory_block(void *begin, void *end) FOONATHAN_NOEXCEPT
        : memory_block(begin, static_cast<char*>(end) - static_cast<char*>(begin)) {}
    };

    namespace detail
    {
        // stores memory block in an intrusive linked list and allows LIFO access
        class memory_block_stack
        {
        public:
            memory_block_stack() FOONATHAN_NOEXCEPT
            : head_(nullptr) {}

            ~memory_block_stack() FOONATHAN_NOEXCEPT {}

            memory_block_stack(memory_block_stack &&other) FOONATHAN_NOEXCEPT
            : head_(other.head_)
            {
                other.head_ = nullptr;
            }

            memory_block_stack& operator=(memory_block_stack &&other) FOONATHAN_NOEXCEPT
            {
                memory_block_stack tmp(detail::move(other));
                swap(*this, tmp);
                return *this;
            }

            friend void swap(memory_block_stack &a, memory_block_stack &b) FOONATHAN_NOEXCEPT
            {
                detail::adl_swap(a.head_, b.head_);
            }

            // the raw allocated block returned from an allocator
            using allocated_mb = memory_block;

            // the inserted block slightly smaller to allow for the fixup value
            using inserted_mb = memory_block;

            // pushes a memory block
            void push(allocated_mb block) FOONATHAN_NOEXCEPT;

            // pops a memory block and returns the original block
            allocated_mb pop() FOONATHAN_NOEXCEPT;

            // steals the top block from another stack
            void steal_top(memory_block_stack &other) FOONATHAN_NOEXCEPT;

            // returns the last pushed() inserted memory block
            inserted_mb top() const FOONATHAN_NOEXCEPT;

            bool empty() const FOONATHAN_NOEXCEPT
            {
                return head_ == nullptr;
            }

        private:
            struct node;
            node *head_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
