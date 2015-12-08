// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_POOL_COLLECTION_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_POOL_COLLECTION_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_pool_collection and related classes.

#include <type_traits>

#include "detail/align.hpp"
#include "detail/memory_stack.hpp"
#include "detail/free_list_array.hpp"
#include "allocator_traits.hpp"
#include "debugging.hpp"
#include "error.hpp"
#include "memory_arena.hpp"
#include "memory_pool_type.hpp"

namespace foonathan { namespace memory
{
    /// A \c BucketDistribution for \ref memory_pool_collection defining that there is a bucket, i.e. pool, for each size.
    /// That means that for each possible size up to an upper bound there will be a seperate free list.
    /// Allocating a node will not waste any memory.
    struct identity_buckets
    {
        using type = detail::identity_access_policy;
    };

    /// A \c BucketDistribution for \ref memory_pool_collection defining that there is a bucket, i.e. pool, for each power of two.
    /// That means for each power of two up to an upper bound there will be a separate free list.
    /// Allocating a node will only waste half of the memory.
    struct log2_buckets
    {
        using type = detail::log2_access_policy;
    };

    /// A stateful \concept{concept_rawallocator,RawAllocator} that behaves as a collection of multiple \ref memory_pool objects.
    /// It maintains a list of multiple free lists, whose types are controlled via the \c PoolType tags defined in \ref memory_pool_type.hpp,
    /// each of a different size as defined in the \c BucketDistribution (\ref identity_buckets or \ref log2_buckets).
    /// Allocating a node of given size will use the appropriate free list.<br>
    /// This allocator is ideal for \concept{concept_node,node} allocations in any order but with a predefined set of sizes,
    /// not only one size like \ref memory_pool.
    /// \ingroup memory
    template <class PoolType, class BucketDistribution,
            class BlockOrRawAllocator = default_allocator>
    class memory_pool_collection
    : FOONATHAN_EBO(detail::leak_checker<memory_pool_collection<node_pool, identity_buckets, default_allocator>>)
    {
        using free_list_array = detail::free_list_array<typename PoolType::type,
                                                        typename BucketDistribution::type>;
        using leak_checker =  detail::leak_checker<memory_pool_collection<node_pool,
                                                identity_buckets, default_allocator>>;
    public:
        using allocator_type = make_block_allocator_t<BlockOrRawAllocator>;
        using pool_type = PoolType;
        using bucket_distribution = BucketDistribution;

        /// \effects Creates it by giving it the maximum node size it should be able to allocate,
        /// the size of the initial memory block and other constructor arguments for the \concept{concept_blockallocator,BlockAllocator}.
        /// The \c BucketDistribution controls how many free lists are created,
        /// but unlike in \ref memory_pool all free lists are initially empty and the first memory block queued.
        /// \requires \c max_node_size must be a valid \concept{concept_node,node} size
        /// and \c block_size must be non-zero.
        template <typename ... Args>
        memory_pool_collection(std::size_t max_node_size, std::size_t block_size,
                    Args&&... args)
        : leak_checker(info().name),
          arena_(block_size, detail::forward<Args>(args)...),
          stack_(allocate_block()),
          pools_(stack_, block_end(), max_node_size)
        {}

        /// \effects Destroys the \ref memory_pool_collection by returning all memory blocks,
        /// regardless of properly deallocated back to the \concept{concept_blockallocator,BlockAllocator}.
        ~memory_pool_collection() FOONATHAN_NOEXCEPT = default;

        /// @{
        /// \effects Moving a \ref memory_pool_collection object transfers ownership over the free lists,
        /// i.e. the moved from pool is completely empty and the new one has all its memory.
        /// That means that it is not allowed to call \ref deallocate_node() on a moved-from allocator
        /// even when passing it memory that was previously allocated by this object.
        memory_pool_collection(memory_pool_collection &&other) FOONATHAN_NOEXCEPT
        : detail::leak_checker<memory_pool_collection<node_pool, identity_buckets, default_allocator>>(detail::move(other)),
          arena_(detail::move(other.arena_)),
          stack_(detail::move(other.stack_)),
          pools_(detail::move(other.pools_))
        {}

        memory_pool_collection& operator=(memory_pool_collection &&other) FOONATHAN_NOEXCEPT
        {
            detail::leak_checker<memory_pool_collection<node_pool, identity_buckets, default_allocator>>::operator=(detail::move(other));
            arena_ = detail::move(other.arena_);
            stack_ = detail::move(other.stack_);
            pools_ = detail::move(other.pools_);
            return *this;
        }
        /// @}

        /// \effects Allocates a \concept{concept_node,node} of given size.
        /// It first finds the appropriate free list as defined in the \c BucketDistribution.
        /// If it is empty, it will use an implementation defined amount of memory from the arena
        /// and inserts it in it.
        /// If the arena is empty too, it will request a new memory block from the \concept{concept_blockallocator,BlockAllocator}
        /// of size \ref next_capacity() and puts part of it onto this free list.
        /// Then it removes a node from it.
        /// \returns A \concept{concept_node,node} of given size suitable aligned,
        /// i.e. suitable for any type where <tt>sizeof(T) < node_size</tt>.
        /// \throws Anything thrown by the \concept{concept_blockallocator,BlockAllocator} if a growth is needed.
        /// \requires \c node_size must be a valid \concept{concept_node,node size} less than or equal to \ref max_node_size().
        void* allocate_node(std::size_t node_size)
        {
            auto& pool = pools_.get(node_size);
            if (pool.empty())
                reserve_impl(pool, def_capacity());
            return pool.allocate();
        }

        /// \effects Allocates an \concept{concept_array,array} of nodes by searching for \c n continuous nodes on the appropriate free list and removing them.
        /// Depending on the \c PoolType this can be a slow operation or not allowed at all.
        /// This can sometimes lead to a growth on the free list, even if technically there is enough continuous memory on the free list.
        /// Otherwise has the same behavior as \ref allocate_node().
        /// \returns An array of \c n nodes of size \c node_size suitable aligned.
        /// \throws Anything thrown by the used \concept{concept_blockallocator,BlockAllocator}'s allocation function if a growth is needed,
        /// or \ref bad_allocation_size if <tt>n * node_size()</tt> is too big.
        /// \requires The \c PoolType must support array allocations, otherwise the body of this function will not compile.
        /// \c count must be valid \concept{concept_array,array count} and
        /// \c node_size must be valid \concept{concept_node,node size} less than or equal to \ref max_node_size().
        void* allocate_array(std::size_t count, std::size_t node_size)
        {
            static_assert(PoolType::value, "array allocations not supported");
            detail::check_allocation_size(count * node_size, next_capacity(),
                                          info());
            auto& pool = pools_.get(node_size);
            if (pool.empty())
                reserve_impl(pool, def_capacity());
            auto mem = pool.allocate(count * node_size);
            if (!mem)
            {
                reserve_impl(pool, count * node_size);
                mem = pool.allocate(count * node_size);
                if (!mem)
                    FOONATHAN_THROW(bad_allocation_size(info(),
                                                        count * node_size, next_capacity()));
            }
            return mem;
        }

        /// \effects Deallocates a \concept{concept_node,node} by putting it back onto the appropriate free list.
        /// \requires \c ptr must be a result from a previous call to \ref allocate_node() with the same size on the same free list,
        /// i.e. either this allocator object or a new object created by moving this to it.
        void deallocate_node(void *ptr, std::size_t node_size) FOONATHAN_NOEXCEPT
        {
            pools_.get(node_size).deallocate(ptr);
        }

        /// \effects Deallocates an \concept{concept_array,array} by putting it back onto the free list.
        /// \requires \c ptr must be a result from a previous call to \ref allocate_array() with the same sizes on the same free list,
        /// i.e. either this allocator object or a new object created by moving this to it.
        void deallocate_array(void *ptr, std::size_t count, std::size_t node_size) FOONATHAN_NOEXCEPT
        {
            static_assert(PoolType::value, "array allocations not supported");
            auto& pool = pools_.get(node_size);
            pool.deallocate(ptr, count * node_size);
        }

        /// \effects Inserts more memory on the free list for nodes of given size.
        /// It will try to put \c capacity bytes from the arena onto the free list defined over the \c BucketDistribution,
        /// if the arena is empty, a new memory block is requested from the \concept{concept_blockallocator,BlockAllocator}
        /// and it will be used.
        /// \throws Anything thrown by the \concept{concept_blockallocator,BlockAllocator} if a growth is needed.
        /// \requires \c node_size must be valid \concept{concept_node,node size} less than or equal to \ref max_node_size(),
        /// \c capacity must be less than \ref next_capacity().
        void reserve(std::size_t node_size, std::size_t capacity)
        {
            auto& pool = pools_.get(node_size);
            reserve_impl(pool, capacity);
        }

        /// \returns The maximum node size for which is a free list.
        /// This is the value passed to it in the constructor.
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT
        {
            return pools_.max_node_size();
        }

        /// \returns The amount of nodes available in the free list for nodes of given size
        /// as defined over the \c BucketDistribution.
        /// This is the number of nodes that can be allocated without the free list requesting more memory from the arena.
        /// \note Array allocations may lead to a growth even if the capacity is big enough.
        std::size_t pool_capacity(std::size_t node_size) const FOONATHAN_NOEXCEPT
        {
            return pools_.get(node_size).capacity();
        }

        /// \returns The amount of memory available in the arena not inside the free lists.
        /// This is the number of bytes that can be inserted into the free lists
        /// without requesting more memory from the \concept{concept_blockallocator,BlockAllocator}.
        /// \note Array allocations may lead to a growth even if the capacity is big enough.
        std::size_t capacity() const FOONATHAN_NOEXCEPT
        {
            return std::size_t(block_end() - stack_.top());
        }

        /// \returns The size of the next memory block after the free list gets empty and the arena grows.
        /// This function just forwards to the \ref memory_arena.
        /// \note Due to fence memory, alignment buffers and the like this may not be the exact result \ref capacity() will return,
        /// but it is an upper bound to it.
        std::size_t next_capacity() const FOONATHAN_NOEXCEPT
        {
            return arena_.next_block_size();
        }

        /// \returns A reference to the \concept{concept_blockallocator,BlockAllocator} used for managing the arena.
        /// \requires It is undefined behavior to move this allocator out into another object.
        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return arena_.get_allocator();
        }

    private:
        allocator_info info() const FOONATHAN_NOEXCEPT
        {
            return {FOONATHAN_MEMORY_LOG_PREFIX "::memory_pool_collection", this};
        }

        std::size_t def_capacity() const FOONATHAN_NOEXCEPT
        {
            return arena_.next_block_size() / pools_.size();
        }

        detail::fixed_memory_stack allocate_block()
        {
            return detail::fixed_memory_stack(arena_.allocate_block().memory);
        }

        const char* block_end() const FOONATHAN_NOEXCEPT
        {
            auto block = arena_.current_block();
            return static_cast<const char*>(block.memory) + block.size;
        }

        void reserve_impl(typename pool_type::type &pool, std::size_t capacity)
        {
            auto mem = stack_.allocate(block_end(), capacity, detail::max_alignment);
            if (!mem)
            {
                // insert rest
                if (auto remaining = std::size_t(block_end() - stack_.top()))
                {
                    auto offset = detail::align_offset(stack_.top(), detail::max_alignment);
                    if (offset < remaining)
                    {
                        detail::debug_fill(stack_.top(), offset,
                                            debug_magic::alignment_memory);
                        pool.insert(stack_.top() + offset, remaining - offset);
                    }
                }
                // get new block
                stack_ = allocate_block();

                // allocate ensuring alignment
                mem = stack_.allocate(block_end(), capacity, detail::max_alignment);
                FOONATHAN_MEMORY_ASSERT(mem);
            }
            // insert new
            pool.insert(mem, capacity);
        }

        memory_arena<allocator_type> arena_;
        detail::fixed_memory_stack stack_;
        free_list_array pools_;
    };

    /// An alias for \ref memory_pool_collection using the \ref identity_buckets policy
    /// and a \c PoolType defaulting to \ref node_pool.
    /// \ingroup memory
    template <class PoolType = node_pool, class ImplAllocator = default_allocator>
    FOONATHAN_ALIAS_TEMPLATE(bucket_allocator,
                             memory_pool_collection<PoolType, identity_buckets, ImplAllocator>);

    /// Specialization of the \ref allocator_traits for \ref memory_pool_collection classes.
    /// \note It is not allowed to mix calls through the specialization and through the member functions,
    /// i.e. \ref memory_pool_collection::allocate_node() and this \c allocate_node().
    /// \ingroup memory
    template <class Pool, class BucketDist, class RawAllocator>
    class allocator_traits<memory_pool_collection<Pool, BucketDist, RawAllocator>>
    {
    public:
        using allocator_type = memory_pool_collection<Pool, BucketDist, RawAllocator>;
        using is_stateful = std::true_type;

        /// \returns The result of \ref memory_pool_collection::allocate_node().
        /// \throws Anything thrown by the pool allocation function
        /// or \ref bad_allocation_size if \c size / \c alignment exceeds \ref max_node_size() / the suitable alignment value,
        /// i.e. the node is over-aligned.
        static void* allocate_node(allocator_type &state,
                                std::size_t size, std::size_t alignment)
        {
            detail::check_allocation_size(size, max_node_size(state), state.info());
            detail::check_allocation_size(alignment, detail::alignment_for(size), state.info());
            auto mem = state.allocate_node(size);
            state.on_allocate(size);
            return mem;
        }

        /// \returns The result of \ref memory_pool_collection::allocate_array().
        /// \throws Anything thrown by the pool allocation function.
        /// \requires The \ref memory_pool_collection has to support array allocations.
        static void* allocate_array(allocator_type &state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            detail::check_allocation_size(size, max_node_size(state), state.info());
            detail::check_allocation_size(alignment, max_alignment(state), state.info());
            return allocate_array(Pool{}, state, count, size);
        }

        /// \effects Calls \ref memory_pool_collection::deallocate_node().
        static void deallocate_node(allocator_type &state,
                    void *node, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.deallocate_node(node, size);
            state.on_deallocate(size);
        }

        /// \effects Calls \ref memory_pool_collection::deallocate_array().
        /// \requires The \ref memory_pool_collection has to support array allocations.
        static void deallocate_array(allocator_type &state,
                    void *array, std::size_t count, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            deallocate_array(Pool{}, state, array, count, size);
        }

        /// \returns The maximum size of each node which is \ref memory_pool_collection::max_node_size().
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.max_node_size();
        }

        /// \returns An upper bound on the maximum array size which is \ref memory_pool::next_capacity().
        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.next_capacity();
        }

        /// \returns Just \c alignof(std::max_align_t) since the actual maximum alignment depends on the node size,
        /// the nodes must not be over-aligned.
        static std::size_t max_alignment(const allocator_type &) FOONATHAN_NOEXCEPT
        {
            return detail::max_alignment;
        }

    private:
        static void* allocate_array(std::false_type, allocator_type &,
                                    std::size_t, std::size_t)
        {
            FOONATHAN_MEMORY_UNREACHABLE("array allocations not supported");
            return nullptr;
        }

        static void* allocate_array(std::true_type, allocator_type &state,
                                    std::size_t count, std::size_t size)
        {
            auto mem = state.allocate_array(count, size);
            state.on_allocate(count * size);
            return mem;
        }

        static void deallocate_array(std::false_type, allocator_type &,
                                     void *, std::size_t, std::size_t)
        {
            FOONATHAN_MEMORY_UNREACHABLE("array allocations not supported");
        }

        static void deallocate_array(std::true_type, allocator_type &state,
                                     void *array, std::size_t count, std::size_t size)
        {
            state.deallocate_array(array, count, size);
            state.on_deallocate(count * size);
        }
    };
}} // namespace foonathan::portal

#endif // FOONATHAN_MEMORY_MEMORY_POOL_COLLECTION_HPP_INCLUDED
