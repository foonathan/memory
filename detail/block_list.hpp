#ifndef FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED

#include <cstddef>
#include <utility>

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
        
        // simple intrusive list managing memory blocks
        // does not deallocate blocks
        class block_list_impl
        {
        public:        
            // the size needed for the implementation
            static std::size_t impl_offset();
            
            block_list_impl() noexcept = default;
            block_list_impl(block_list_impl &&other) noexcept
            : head_(other.head_)
            {
                other.head_ = nullptr;
            }
            ~block_list_impl() noexcept = default;
            
            block_list_impl& operator=(block_list_impl &&other) noexcept
            {
                head_ = other.head_;
                other.head_ = nullptr;
            }
            
            // inserts a new memory block, returns the size needed for the implementation
            std::size_t push(void* &memory, std::size_t size) noexcept;
            
            // inserts the top memory block of another list, pops it from the other one
            // returns the memory block
            // its size is the usable memory size
            block_info push(block_list_impl &other) noexcept;
            
            // pops the memory block at the top
            // its size is the original size passed to push
            block_info pop() noexcept;
            
            bool empty() const noexcept
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
            static constexpr auto growth_factor = 2u;
        public:        
            // gives it an initial block size and allocates it
            // the blocks get large and large the more are needed
            block_list(std::size_t block_size,
                    RawAllocator allocator)
            : RawAllocator(std::move(allocator)), cur_block_size_(block_size) {}
            block_list(block_list &&) = default;
            ~block_list() noexcept
            {
                shrink_to_fit();
            }
            
            block_list& operator=(block_list &&) = default;
            
            RawAllocator& get_allocator() noexcept
            {
                return *this;
            }
        
            // allocates a new block and returns it and its size
            // name is used for the growth tracker
            block_info allocate()
            {
                if (free_.empty())
                {
                    auto memory = get_allocator().
                        allocate_node(cur_block_size_, alignof(std::max_align_t));
                    auto size = cur_block_size_ - used_.push(memory, cur_block_size_);
                    cur_block_size_ *= growth_factor;
                    return {memory, size};
                }
                // already block cached in free list
                return used_.push(free_);
            }
            
            // deallocates the last allocate block
            // does not free memory, caches the block for future use
            void deallocate() noexcept
            {
                free_.push(used_);
            }
            
            // deallocates all unused cached blocks
            void shrink_to_fit() noexcept
            {
                while (!free_.empty())
                {
                    auto block = free_.pop();
                    get_allocator().
                        deallocate_node(block.memory, block.size, alignof(std::max_align_t));
                }
            }
            
            // returns the next block size
            std::size_t next_block_size() const noexcept
            {
                return cur_block_size_ - block_list_impl::impl_offset();
            }
            
        private:
            block_list_impl used_, free_;
            std::size_t cur_block_size_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_BLOCK_LIST_HPP_INCLUDED` 
