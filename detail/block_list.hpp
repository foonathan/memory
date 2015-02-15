#ifndef FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED

#include <cstddef>
#include <utility>

#include "../tracking.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // information about a memory block
        struct block_info
        {
            void *memory;
            std::size_t size;
            
            block_info(void *memory, std::size_t size) noexcept
            : memory(memory), size(size) {}
        };
        
        // implementation for block_list to get rid of template
        class block_list_impl
        {
        public:        
            block_list_impl() noexcept = default;
            block_list_impl(block_list_impl &&other) noexcept
            : head_(other.head_), size_(other.size_)
            {
                other.head_ = nullptr;
                other.size_ = 0u;
            }
            ~block_list_impl() noexcept = default;
            
            block_list_impl& operator=(block_list_impl &&other) noexcept
            {
                head_ = other.head_;
                other.head_ = nullptr;
                size_ = other.size_;
                other.size_ = 0u;
            }
            
            // inserts a new memory block, returns the size needed for the implementation
            std::size_t push(void* &memory, std::size_t size) noexcept;
            block_info pop() noexcept;
            
            std::size_t size() const noexcept
            {
                return size_;
            }
            
            bool empty() const noexcept
            {
                return head_ == nullptr;
            }
            
        private:
            struct node;
            node *head_ = nullptr;
            std::size_t size_ = 0u;
        };
        
        // manages a collection of memory blocks
        // acts like a stack, new memory blocks are pushed
        // and can be popped in lifo order
        template <class RawAllocator>
        class block_list : RawAllocator 
        {
            static constexpr auto growth_factor = 2u;
        public:        
            // gives it an initial block size
            // the blocks get large and large the more are needed
            block_list(std::size_t block_size,
                    RawAllocator allocator)
            : RawAllocator(std::move(allocator)), cur_block_size_(block_size) {}
            block_list(block_list &&) = default;
            ~block_list() noexcept
            {
                clear();
            }
            
            block_list& operator=(block_list &&) = default;
            
            RawAllocator& get_allocator() noexcept
            {
                return *this;
            }
        
            // allocates a new block and returns it and its size
            // name is used for the growth tracker
            block_info allocate(const char *name)
            {
                if (!list_.empty())
                    detail::on_allocator_growth(name, this, cur_block_size_ / growth_factor);
                auto memory = get_allocator().
                    allocate_node(cur_block_size_, alignof(std::max_align_t));
                auto size = cur_block_size_ - list_.push(memory, cur_block_size_);
                cur_block_size_ *= growth_factor;
                return {memory, size};
            }
            
            // deallocates the last allocate block
            void deallocate() noexcept
            {
                auto block = list_.pop();
                get_allocator().
                    deallocate_node(block.memory, block.size, alignof(std::max_align_t));
            }
            
            // deallocates all blocks
            void clear() noexcept
            {
                while (!list_.empty())
                    deallocate();
            }
            
            // returns the next block size
            std::size_t next_block_size() const noexcept
            {
                return cur_block_size_;
            }
            
        private:
            block_list_impl list_;
            std::size_t cur_block_size_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED` 