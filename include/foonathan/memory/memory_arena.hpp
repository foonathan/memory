// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED

/// \file
/// Class \ref memory_arena and related functionality regarding \concept{concept_blockallocator,BlockAllocators}.

#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "config.hpp"
#include "debugging.hpp"
#include "default_allocator.hpp"
#include "error.hpp"

namespace foonathan { namespace memory
{
    /// A memory block.
    /// It is defined by its starting address and size.
    /// \ingroup memory
    struct memory_block
    {
        void *memory; ///< The address of the memory block (might be \c nullptr).
        std::size_t size; ///< The size of the memory block (might be \c 0).

        /// \effects Creates an invalid memory block with starting address \c nullptr and size \c 0.
        memory_block() FOONATHAN_NOEXCEPT
        : memory_block(nullptr, size) {}

        /// \effects Creates a memory block from a given starting address and size.
        memory_block(void *mem, std::size_t size) FOONATHAN_NOEXCEPT
        : memory(mem), size(size) {}

        /// \effects Creates a memory block from a [begin,end) range.
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

    /// A memory arena that manages huge memory blocks for a higher-level allocator.
    /// Some allocators like \ref memory_stack work on huge memory blocks,
    /// this class manages them fro those allocators.
    /// It uses a \concept{concept_blockallocator,BlockAllocator} for the allocation of those blocks.
    /// The memory blocks in use are put onto a stack like structure, deallocation will pop from the top,
    /// so it is only possible to deallocate the last allocated block of the arena.
    /// \ingroup memory
    template <class BlockAllocator>
    class memory_arena : FOONATHAN_EBO(BlockAllocator)
    {
        static_assert(!detail::is_nested_arena<BlockAllocator>::value,
                      "memory_arena must not be instantiated with itself");
    public:
        using allocator_type = BlockAllocator;

        /// \effects Creates it by giving it the size and other arguments for the \concept{concept_blockallocator,BlockAllocator}.
        /// It forwards these arguments to its constructor.
        /// \requires \c block_size must be greater than \c 0 and other requirements depending on the \concept{concept_blockallocator,BlockAllocator}.
        /// \throws Anything thrown by the constructor of the \c BlockAllocator.
        template <typename ... Args>
        explicit memory_arena(std::size_t block_size, Args&&... args)
        : allocator_type(block_size, detail::forward<Args>(args)...),
          no_used_(0u), no_cached_(0u)
        {}

        /// \effects Deallocates all memory blocks that where requested back to the \concept{concept_blockallocator,BlockAllocator}.
        ~memory_arena() FOONATHAN_NOEXCEPT
        {
            // push all cached to used_ to reverse order
            while (!cached_.empty())
                used_.steal_top(cached_);
            // now deallocate everything
            while (!used_.empty())
                allocator_type::deallocate_block(used_.pop());
        }

        /// @{
        /// \effects Moves the arena.
        /// The new arena takes ownership over all the memory blocks from the other arena object,
        /// which is empty after that.
        /// This does not invalidate any memory blocks.
        memory_arena(memory_arena &&other) FOONATHAN_NOEXCEPT
                : allocator_type(detail::move(other)),
                  used_(detail::move(other.used_)), cached_(detail::move(other.cached_)),
                  no_used_(other.no_used_), no_cached_(other.no_cached_)
        {
            other.no_used_ = other.no_cached_ = 0;
        }

        memory_arena& operator=(memory_arena &&other) FOONATHAN_NOEXCEPT
        {
            memory_arena tmp(detail::move(other));
            swap(*this, tmp);
            return *this;
        }
        /// @}

        /// \effects Swaps to memory arena objects.
        /// This does not invalidate any memory blocks.
        friend void swap(memory_arena &a, memory_arena &b) FOONATHAN_NOEXCEPT
        {
            detail::adl_swap(a.used_, b.used_);
            detail::adl_swap(a.cached_, b.cached_);
            detail::adl_swap(a.no_used_, b.no_used_);
            detail::adl_swap(a.no_cached_, b.no_cached_);
        }

        /// \effects Allocates a new memory block.
        /// It first uses a cache of previously deallocated blocks,
        /// if it is empty, allocates a new one.
        /// \returns The new \ref memory_block.
        /// \throws Anything thrown by the \concept{concept_blockallocator,BlockAllocator} allocation function.
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

        /// \returns The current memory block.
        /// This is the memory block that will be deallocated by the next call to \ref deallocate_block().
        memory_block current_block() const FOONATHAN_NOEXCEPT
        {
            return used_.top();
        }

        /// \effects Deallocates the current memory block.
        /// The current memory block is the block on top of the stack of blocks.
        /// It does not really deallocate it but puts it onto a cache for later use,
        /// use \ref shrink_to_fit() to purge that cache.
        void deallocate_block() FOONATHAN_NOEXCEPT
        {
            --no_used_;
            ++no_cached_;
            auto block = used_.top();
            detail::debug_fill(block.memory, block.size, debug_magic::internal_freed_memory);
            cached_.steal_top(used_);
        }

        /// \effects Purges the cache of unused memory blocks by returning them.
        /// The memory blocks will be deallocated in reversed order of allocation.
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

        /// \returns The capacity of the arena, i.e. how many blocks are used and cached.
        std::size_t capacity() const FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(bool(no_cached_) != cached_.empty());
            return no_cached_ + no_used_;
        }

        /// \returns The size of the arena, i.e. how many blocks are in use.
        /// It is always smaller or equal to the \ref capacity().
        std::size_t size() const FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(bool(no_used_) != used_.empty());
            return no_used_;
        }

        /// \returns The size of the next memory block,
        /// i.e. of the next call to \ref allocate_block() if it does not use the cache.
        /// This simply forwards to the \concept{concept_blockallocator,BlockAllocator}.
        std::size_t next_block_size() const FOONATHAN_NOEXCEPT
        {
            return allocator_type::next_block_size();
        }

        /// \returns A reference of the \concept{concept_blockallocator,BlockAllocator} object.
        /// \requires It is undefined behavior to move this allocator out into another object.
        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

    private:
        detail::memory_block_stack used_, cached_;
        std::size_t no_used_, no_cached_;
    };

    /// A \concept{concept_blockallocator,BlockAllocator} that uses a given \concept{concept_rawallocator,RawAllocator} for allocating the blocks.
    /// It calls the \c allocate_array() function with a node of size \c 1 and maximum alignment on the used allocator for the block allocation.
    /// The size of the next memory block will grow by a given factor after each allocation,
    /// allowing an amortized constant allocation time in the higher level allocator.
    /// \ingroup memory
    template <class RawAllocator = default_allocator>
    class growing_block_allocator
    : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
    {
        using traits = allocator_traits<RawAllocator>;
    public:
        using allocator_type = typename traits::allocator_type;

        /// \effects Creates it by giving it the initial block size, the allocator object and the growth factor.
        /// By default, it uses a default-constructed allocator object and a growth factor of \c 2.
        /// \requires \c block_size must be greater than 0, and the \c factor not less than \c 1.
        explicit growing_block_allocator(std::size_t block_size,
                                         allocator_type alloc = allocator_type(),
                                         float growth_factor = 2.f) FOONATHAN_NOEXCEPT
        : allocator_type(detail::move(alloc)),
          block_size_(block_size), growth_factor_(growth_factor) {}

        /// \effects Allocates a new memory block and increases the block size for the next allocation.
        /// \returns The new \ref memory_block.
        /// \throws Anything thrown by the \c allocate_array() function of the \concept{concept_rawallocator,RawAllocator}.
        memory_block allocate_block()
        {
            auto memory = traits::allocate_array(get_allocator(), block_size_,
                                                 1, detail::max_alignment);
            memory_block block(memory, block_size_);
            block_size_ *= growth_factor_;
            return block;
        }

        /// \effects Deallocates a previously allocated memory block.
        /// This does not decrease the block size.
        /// \requires \c block must be previously returned by a call to \ref allocate_block().
        void deallocate_block(memory_block block) FOONATHAN_NOEXCEPT
        {
            traits::deallocate_array(get_allocator(), block.memory,
                                    block.size, 1, detail::max_alignment);
        }

        /// \returns The size of the memory block returned by the next call to \ref allocate_block().
        std::size_t next_block_size() const FOONATHAN_NOEXCEPT
        {
            return block_size_;
        }

        /// \returns A reference to the used \concept{concept_rawallocator,RawAllocator} object.
        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        /// \returns The growth factor.
        float growth_factor() const FOONATHAN_NOEXCEPT
        {
            return growth_factor_;
        }

        /// \effects Sets the growth factor to a new value.
        /// \requires \c f must not be lass than \c 1.
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

    /// Takes either a \concept{concept_blockallocator,BlockAllocator} or a \concept{concept_rawallocator,RawAllocator}.
    /// In the first case simply aliases the type unchanged, in the second to \ref growing_block_allocator with the \concept{concept_rawallocator,RawAllocator}.
    /// Using this allows passing normal \concept{concept_rawallocator,RawAllocators} as \concept{concept_blockallocator,BlockAllocators}.
    /// \ingroup memory
    template <class BlockOrRawAllocator>
    using make_block_allocator_t
        = decltype(detail::make_block_allocator<BlockOrRawAllocator>(0,
                        0, std::declval<BlockOrRawAllocator>()));

    /// Helper function make a \concept{concept_blockallocator,BlockAllocator}.
    /// \returns A \concept{concept_blockallocator,BlockAllocator} of the given type created with the given arguments.
    /// \requires Same requirements as the constructor.
    template <class BlockOrRawAllocator, typename ... Args>
    make_block_allocator_t<BlockOrRawAllocator> make_block_allocator(std::size_t block_size, Args&&... args)
    {
        return detail::make_block_allocator(0, block_size, detail::forward<Args>(args)...);
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
