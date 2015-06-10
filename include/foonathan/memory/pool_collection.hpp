// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_POOL_COLLECTION_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_COLLECTION_HPP_INCLUDED

/// \file
/// \brief A class managing pools of different sizes.

#include <type_traits>

#include "detail/align.hpp"
#include "detail/block_list.hpp"
#include "detail/memory_stack.hpp"
#include "detail/free_list_array.hpp"
#include "allocator_traits.hpp"
#include "debugging.hpp"
#include "default_allocator.hpp"
#include "pool_type.hpp"

namespace foonathan { namespace memory
{
    /// @{
    /// \brief Tag type defining the bucket (i.e. pool) distribution of a \ref memory_pool_collection.
    /// \details There are two different distribution policies:
    /// * \ref identity_buckets: there is a bucket for each size.
    /// * \ref log2_buckets: there is a bucket for each power of two size.
    /// \ingroup memory
    struct identity_buckets
    {
        using type = detail::identity_access_policy;
    };

    struct log2_buckets
    {
        using type = detail::log2_access_policy;
    };
    /// @}

    /// \brief Manages multiple memory pools, each with a fixed size.
    /// \details This allows allocating of nodes of various sizes.
    /// The pool type can be specified via the tag types in \ref pool_type.hpp.<br>
    /// The distribution of buckets (i.e. pools) can be specified via the policy tags above.
    /// They control for which sizes pools are created.<br>
    /// Otherwise behaves the same as \ref memory_pool.
    /// \ingroup memory
    template <class PoolType, class BucketDistribution,
            class RawAllocator = default_allocator>
    class memory_pool_collection
    : detail::leak_checker<memory_pool_collection<node_pool, identity_buckets, default_allocator>>
    {
        using free_list_array = detail::free_list_array<typename PoolType::type,
                                                        typename BucketDistribution::type>;
        using leak_checker =  detail::leak_checker<memory_pool_collection<node_pool,
                                                identity_buckets, default_allocator>>;
    public:
        using impl_allocator = RawAllocator;
        using pool_type = PoolType;
        using bucket_distribution = BucketDistribution;

        /// \brief Creates a new pool collection with given max node size the memory block size.
        /// \details It can handle node sizes up to a given size.<br>
        /// The first memory block is allocated, the block size can change.
        memory_pool_collection(std::size_t max_node_size, std::size_t block_size,
                    impl_allocator alloc = impl_allocator())
        : leak_checker("foonathan::memory::memory_pool_collection"),
          block_list_(block_size, std::move(alloc)),
          stack_(block_list_.allocate()),
          pools_(stack_, max_node_size)
        {}

        /// \brief Allocates a node of given size.
        /// \details It selects the smallest node pool with sufficient size,
        /// the size must be smaller than the maximum node size.
        void* allocate_node(std::size_t node_size)
        {
            auto& pool = pools_.get(node_size);
            if (pool.empty())
                reserve_impl(pool, def_capacity());
            return pool.allocate();
        }

        /// \brief Allocates an array of given node size and number of elements.
        /// \details It selects the smallest node pool with sufficient size,
        /// the size must be smaller than the maximum node size.
        void* allocate_array(std::size_t count, std::size_t node_size)
        {
            auto& pool = pools_.get(node_size);
            if (pool.empty())
                reserve_impl(pool, def_capacity());
            return pool.allocate(count * node_size);
        }

        /// @{
        /// \brief Deallocates the memory into the appropriate pool.
        void deallocate_node(void *memory, std::size_t node_size) FOONATHAN_NOEXCEPT
        {
            pools_.get(node_size).deallocate(memory);
        }

        void deallocate_array(void *memory, std::size_t count, std::size_t node_size) FOONATHAN_NOEXCEPT
        {
            auto& pool = pools_.get(node_size);
            pool.deallocate(memory, count * node_size);
        }
        /// @}

        /// \brief Reserves memory for the pool for a given node size.
        void reserve(std::size_t node_size, std::size_t capacity)
        {
            auto& pool = pools_.get(node_size);
            reserve_impl(pool, capacity);
        }

        /// \brief Returns the maximum node size for which there is a pool.
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT
        {
            return pools_.max_node_size();
        }

        /// \brief Returns the capacity available in the pool for a given node size.
        /// \details This is the amount of nodes available inside the given pool.
        std::size_t pool_capacity(std::size_t node_size) const FOONATHAN_NOEXCEPT
        {
            return pools_.get(node_size).capacity();
        }

        /// \brief Returns the capacity available outside the pools.
        /// \details This is the amount of memory that can be given to the pools after they are exhausted.
        std::size_t capacity() const FOONATHAN_NOEXCEPT
        {
            return stack_.end() - stack_.top();
        }

        /// \brief Returns the size of the next memory block.
        /// \details This is the new capacity after \ref capacity() is exhausted.<br>
        /// This is also the maximum array size.
        std::size_t next_capacity() const FOONATHAN_NOEXCEPT
        {
            return block_list_.next_block_size();
        }

        /// \brief Returns the \ref impl_allocator.
        impl_allocator& get_impl_allocator() FOONATHAN_NOEXCEPT
        {
            return block_list_.get_allocator();
        }

    private:
        std::size_t def_capacity() const FOONATHAN_NOEXCEPT
        {
            return block_list_.next_block_size() / pools_.size();
        }

        void reserve_impl(typename pool_type::type &pool, std::size_t capacity)
        {
            auto mem = stack_.allocate(capacity, detail::max_alignment);
            if (!mem)
            {
                // insert rest
                if (stack_.end() - stack_.top() != 0u)
                    pool.insert(stack_.top(), stack_.end() - stack_.top());
                stack_ = detail::fixed_memory_stack(block_list_.allocate());
                // allocate ensuring alignment
                mem = stack_.allocate(capacity,  detail::max_alignment);
                assert(mem);
            }
            // insert new
            pool.insert(mem, capacity);
        }

        detail::block_list<RawAllocator> block_list_;
        detail::fixed_memory_stack stack_;
        free_list_array pools_;
    };

    /// \brief A bucket allocator.
    /// \detail It is a typedef \ref memory_pool_collection with \ref identity_buckets.
    /// \ingroup memory
    template <class PoolType, class ImplAllocator = default_allocator>
    using bucket_allocator = memory_pool_collection<PoolType, identity_buckets, ImplAllocator>;

    /// \brief Specialization of the \ref allocator_traits for a \ref memory_pool_collection.
    /// \details This allows passing a pool directly as allocator to container types.
    /// \note This interface does leak checking, if you allocate through it, you need to deallocate.
    /// Do not mix the two interfaces, e.g. allocate here and deallocate on the original interface!
    /// \ingroup memory
    template <class Pool, class BucketDist, class RawAllocator>
    class allocator_traits<memory_pool_collection<Pool, BucketDist, RawAllocator>>
    {
    public:
        using allocator_type = memory_pool_collection<Pool, BucketDist, RawAllocator>;
        using is_stateful = std::true_type;

        static void* allocate_node(allocator_type &state,
                                std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            assert(alignment <=  detail::max_alignment && "invalid alignment");
            auto mem = state.allocate_node(size);
            state.on_allocate(size);
            return mem;
        }

        static void* allocate_array(allocator_type &state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            assert(alignment <=  detail::max_alignment && "invalid alignment");
            assert(count * size <= max_array_size(state) && "invalid array count");
            auto mem = state.allocate_array(count, size);
            state.on_allocate(count * size);
            return mem;
        }

        static void deallocate_node(allocator_type &state,
                    void *node, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.deallocate_node(node, size);
            state.on_deallocate(size);
        }

        static void deallocate_array(allocator_type &state,
                    void *array, std::size_t count, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.deallocate_array(array, count, size);
            state.on_deallocate(count * size);
        }

        /// \brief Maximum size of a node is the maximum pool collections node size.
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.max_node_size();
        }

        /// \brief Maximum size of an array is the capacity in the next block of the pool.
        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.next_capacity();
        }

        /// \brief Maximum alignment is alignof(std::max_align_t).
        static std::size_t max_alignment(const allocator_type &) FOONATHAN_NOEXCEPT
        {
            return  detail::max_alignment;
        }
    };
}} // namespace foonathan::portal

#endif // FOONATHAN_MEMORY_POOL_COLLECTION_HPP_INCLUDED
