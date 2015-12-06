// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED

#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "config.hpp"
#include "debugging.hpp"
#include "default_allocator.hpp"
#include "error.hpp"

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

#if !defined(DOXYGEN)
    template <class BlockAllocator>
    class memory_arena;
#endif

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

        template <class BlockAllocator>
        using is_nested_arena = is_instantiation_of<memory_arena, BlockAllocator>;
    } // namespace detail

    template <class BlockAllocator>
    class memory_arena : FOONATHAN_EBO(BlockAllocator)
    {
        static_assert(!detail::is_nested_arena<BlockAllocator>::value,
                      "memory_arena must not be instantiated with itself");
    public:
        using allocator_type = BlockAllocator;

        template <typename ... Args>
        explicit memory_arena(std::size_t block_size, Args&&... args) FOONATHAN_NOEXCEPT
        : allocator_type(block_size, detail::forward<Args>(args)...),
          no_used_(0u), no_cached_(0u)
        {}

        memory_arena(memory_arena &&other) FOONATHAN_NOEXCEPT
        : allocator_type(detail::move(other)),
          used_(detail::move(other)), cached_(detail::move(other)),
          no_used_(other.no_used_), no_cached_(other.no_cached_)
        {
            other.no_used_ = other.no_cached_ = 0;
        }

        ~memory_arena() FOONATHAN_NOEXCEPT
        {
            // push all cached to used_ to reverse order
            while (!cached_.empty())
                used_.steal_top(cached_);
            // now deallocate everything
            while (!used_.empty())
                allocator_type::deallocate_block(used_.pop());
        }

        memory_arena& operator=(memory_arena &&other) FOONATHAN_NOEXCEPT
        {
            memory_arena tmp(detail::move(other));
            swap(*this, tmp);
            return *this;
        }

        friend void swap(memory_arena &a, memory_arena &b) FOONATHAN_NOEXCEPT
        {
            detail::adl_swap(a.used_, b.used_);
            detail::adl_swap(a.cached_, b.cached_);
            detail::adl_swap(a.no_used_, b.no_used_);
            detail::adl_swap(a.no_cached_, b.no_cached_);
        }

        memory_block allocate_block()
        {
            if (capacity() == size())
                used_.push(allocator_type::allocate_block());
            else
            {
                used_.steal_top(cached_);
                --no_cached_;
            }
            ++no_used_;
            auto block = used_.top();
            detail::debug_fill(block.memory, block.size, debug_magic::internal_memory);
            return block;
        }

        void deallocate_block() FOONATHAN_NOEXCEPT
        {
            --no_used_;
            ++no_cached_;
            auto block = used_.top();
            detail::debug_fill(block.memory, block.size, debug_magic::internal_freed_memory);
            cached_.steal_top(used_);
        }

        void shrink_to_fit() FOONATHAN_NOEXCEPT
        {
            detail::memory_block_stack to_dealloc;
            // pop from cache and push to temporary stack
            // this revers order
            while (!cached_.empty())
                to_dealloc.steal_top(cached_);
            // now dealloc everything
            while (!to_dealloc.empty())
                allocator_type::deallocate_block(to_dealloc.pop());
            no_cached_ = 0u;
        }

        std::size_t capacity() const FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(bool(no_cached_) != cached_.empty());
            return no_cached_ + no_used_;
        }

        std::size_t size() const FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(bool(no_used_) != used_.empty());
            return no_used_;
        }

        std::size_t next_block_size() const FOONATHAN_NOEXCEPT
        {
            return allocator_type::next_block_size();
        }

        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

    private:
        detail::memory_block_stack used_, cached_;
        std::size_t no_used_, no_cached_;
    };

    template <class RawAllocator = default_allocator>
    class growing_block_allocator
    : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
    {
        using traits = allocator_traits<RawAllocator>;
    public:
        using allocator_type = typename traits::allocator_type;

        growing_block_allocator(std::size_t block_size,
                                allocator_type alloc = allocator_type(),
                                float growth_factor = 2.f) FOONATHAN_NOEXCEPT
        : allocator_type(detail::move(alloc)),
          block_size_(block_size), growth_factor_(growth_factor) {}

        memory_block allocate_block()
        {
            auto memory = traits::allocate_array(get_allocator(), block_size_,
                                                 1, detail::max_alignment);
            memory_block block(memory, block_size_);
            block_size_ *= growth_factor_;
            return block;
        }

        void deallocate_block(memory_block block) FOONATHAN_NOEXCEPT
        {
            traits::deallocate_array(get_allocator(), block.memory,
                                    block.size, 1, detail::max_alignment);
        }

        std::size_t next_block_size() const FOONATHAN_NOEXCEPT
        {
            return block_size_;
        }

        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        float growth_factor() const FOONATHAN_NOEXCEPT
        {
            return growth_factor_;
        }

        void set_growth_factor(float f) FOONATHAN_NOEXCEPT
        {
            growth_factor_ = f;
        }

    private:
        std::size_t block_size_;
        float growth_factor_;
    };

    namespace detail
    {
        template <class BlockAlloc, typename ... Args>
        BlockAlloc make_block_allocator(int,
            decltype(std::declval<BlockAlloc>().allocate_block(), std::size_t()) block_size,
            Args&&... args)
        {
            return BlockAlloc(block_size, detail::forward<Args>(args)...);
        }

        template <class RawAlloc>
        growing_block_allocator<RawAlloc> make_block_allocator(short,
            std::size_t block_size, RawAlloc alloc = RawAlloc(), float fac = 2.f,
            FOONATHAN_SFINAE(std::declval<RawAlloc>().allocate_node(1, 1)))
        {
            return growing_block_allocator<RawAlloc>(block_size, detail::move(alloc), fac);
        }
    } // namespace detail

    template <class BlockOrRawAllocator>
    using make_block_allocator_t
        = decltype(detail::make_block_allocator<BlockOrRawAllocator>(0,
                        0, std::declval<BlockOrRawAllocator>()));

    template <class BlockOrRawAllocator, typename ... Args>
    make_block_allocator_t<BlockOrRawAllocator> make_block_allocator(std::size_t block_size, Args&&... args)
    {
        return detail::make_block_allocator(0, block_size, detail::forward<Args>(args)...);
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
