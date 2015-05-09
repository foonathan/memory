// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED

#include <cstddef>
#include <utility>

#include "align.hpp"
#include "../debugging.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // information about a memory block
        struct block_info
        {
            void *memory;
            std::size_t size;
            
            block_info(void *mem, std::size_t s) FOONATHAN_NOEXCEPT
            : memory(mem), size(s) {}
        };
        
        // simple intrusive list managing memory blocks
        // does not deallocate blocks
        class block_list_impl
        {
        public:        
            // the size needed for the implementation
            static std::size_t impl_offset();
            
            block_list_impl() FOONATHAN_NOEXCEPT = default;
            block_list_impl(block_list_impl &&other) FOONATHAN_NOEXCEPT
            : head_(other.head_)
            {
                other.head_ = nullptr;
            }
            ~block_list_impl() FOONATHAN_NOEXCEPT = default;
            
            block_list_impl& operator=(block_list_impl &&other) FOONATHAN_NOEXCEPT
            {
                head_ = other.head_;
                other.head_ = nullptr;
                return *this;
            }
            
            // inserts a new memory block, returns the size needed for the implementation
            std::size_t push(void* &memory, std::size_t size) FOONATHAN_NOEXCEPT;
            
            // inserts the top memory block of another list, pops it from the other one
            // returns the memory block
            // its size is the usable memory size
            block_info push(block_list_impl &other) FOONATHAN_NOEXCEPT;
            
            // pops the memory block at the top
            // its size is the original size passed to push
            block_info pop() FOONATHAN_NOEXCEPT;
            
            // returns the memory block at the top
            // its size is the usable memory size
            block_info top() const FOONATHAN_NOEXCEPT;
            
            bool empty() const FOONATHAN_NOEXCEPT
            {
                return head_ == nullptr;
            }
            
        private:
            struct node;
            node *head_ = nullptr;
        };
        
        // manages a collection of memory blocks
        // acts like a stack, new memory blocks are pushed
        // and can be popped in lifo order
        template <class RawAllocator>
        class block_list : RawAllocator 
        {
            static FOONATHAN_CONSTEXPR auto growth_factor = 2u;
        public:        
            // gives it an initial block size and allocates it
            // the blocks get large and large the more are needed
            block_list(std::size_t block_size,
                    RawAllocator allocator)
            : RawAllocator(std::move(allocator)), cur_block_size_(block_size) {}
            
            block_list(block_list &&other) FOONATHAN_NOEXCEPT
            : used_(std::move(other.used_)), free_(std::move(other.free_)),
              size_(other.size_), cur_block_size_(other.cur_block_size_)
            {
                other.size_ = other.cur_block_size_ = 0u;
            }
            
            ~block_list() FOONATHAN_NOEXCEPT
            {
                shrink_to_fit();
                while (!used_.empty())
                {
                    auto block = used_.pop();
                    get_allocator().
                        deallocate_node(block.memory, block.size, detail::max_alignment);
                }
            }
            
            block_list& operator=(block_list &&other) FOONATHAN_NOEXCEPT
            {
                used_ = std::move(other.used_);
                free_ = std::move(other.free_);
                size_ = other.size_;
                cur_block_size_ = other.cur_block_size_;
                other.size_ = other.cur_block_size_ = 0u;
                return *this;
            }
            
            RawAllocator& get_allocator() FOONATHAN_NOEXCEPT
            {
                return *this;
            }
        
            // allocates a new block and returns it and its size
            // name is used for the growth tracker
            // debug: mark returned block as internal_memory
            block_info allocate()
            {
                if (free_.empty())
                {
                    auto memory = get_allocator().
                        allocate_node(cur_block_size_, detail::max_alignment);
                    ++size_;
                    auto size = cur_block_size_ - used_.push(memory, cur_block_size_);
                    cur_block_size_ *= growth_factor;
                    detail::debug_fill(memory, size, debug_magic::internal_memory);
                    return {memory, size};
                }
                ++size_;
                // already block cached in free list
                auto block = used_.push(free_);
                detail::debug_fill(block.memory, block.size, debug_magic::internal_memory);
                return block;
            }
            
            // deallocates the last allocated block
            // does not free memory, caches the block for future use
            // debug: don't mark as freed, it might not be fully used
            void deallocate() FOONATHAN_NOEXCEPT
            {
                --size_;
                free_.push(used_);
            }
            
            // the top block, this is the block that was allocated last
            block_info top() const FOONATHAN_NOEXCEPT
            {
                return used_.top();
            }
            
            // deallocates all unused cached blocks
            void shrink_to_fit() FOONATHAN_NOEXCEPT
            {
                while (!free_.empty())
                {
                    auto block = free_.pop();
                    get_allocator().
                        deallocate_node(block.memory, block.size, detail::max_alignment);
                }
            }
            
            // returns the next block size
            std::size_t next_block_size() const FOONATHAN_NOEXCEPT
            {
                return cur_block_size_ - block_list_impl::impl_offset();
            }
            
            std::size_t size() const FOONATHAN_NOEXCEPT
            {
                return size_;
            }
            
        private:
            block_list_impl used_, free_;
            std::size_t size_, cur_block_size_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED` 
