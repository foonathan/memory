// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A pool allocator.

#include <algorithm>
#include <cassert>
#include <memory>
#include <type_traits>

#include "detail/align.hpp"
#include "detail/block_list.hpp"
#include "detail/free_list.hpp"
#include "detail/small_free_list.hpp"
#include "allocator_traits.hpp"
#include "debugging.hpp"
#include "default_allocator.hpp"
#include "pool_type.hpp"

namespace foonathan { namespace memory
{
    /// \brief A memory pool.
    ///
    /// It manages nodes of fixed size.
    /// Allocating and deallocating such a node is really fast,
    /// but each has the given size.<br>
    /// There different types of pools:
    /// * \ref node_pool: doesn't support array allocations.
    /// * \ref array_pool: does support array allocations but slower.
    /// * \ref small_node_pool: optimized for small objects, low memory overhead, but slower.
    /// Does not support array allocations.<br>
    /// It is no \ref concept::RawAllocator, but the \ref allocator_traits are specialized for it.<br>
    /// It allocates big blocks from an implementation allocator.
    /// If their size is sufficient, allocations are fast.
    /// \ingroup memory
    template <typename PoolType, class RawAllocator = default_allocator>
    class memory_pool
    {
        static_assert(std::is_same<PoolType, node_pool>::value ||
                    std::is_same<PoolType, array_pool>::value ||
                    std::is_same<PoolType, small_node_pool>::value,
                    "invalid tag type");
        using free_list = typename std::conditional<
                            std::is_same<PoolType, small_node_pool>::value,
                            detail::small_free_memory_list,
                            detail::free_memory_list>::type;

    public:
        using impl_allocator = RawAllocator;

        /// \brief The type of the pool (\re_tef node_pool, \ref array_pool, \ref small_node_pool).
        using pool_type = PoolType;

        /// \brief The minimum node size due to implementation reasons.
        static FOONATHAN_CONSTEXPR std::size_t min_node_size = free_list::min_element_size;

        /// \brief Gives it the size of the nodes inside the pool and start block size.
        /// \details The first memory block is allocated, the block size can change.
        memory_pool(std::size_t node_size, std::size_t block_size,
                    impl_allocator allocator = impl_allocator())
        : block_list_(block_size, std::move(allocator)),
          free_list_(node_size)
        {
            allocate_block();
        }

        /// \brief Allocates a single node from the pool.
        /// \details It is aligned for \c std::min(node_size(), alignof(std::max_align_t).
        void* allocate_node()
        {
            if (free_list_.empty())
                allocate_block();
            assert(!free_list_.empty());
            return free_list_.allocate();
        }

        /// \brief Allocates an array from the pool.
        /// \details Returns \c n subsequent nodes.<br>
        /// If not \ref array_pool, may fail, throwing \c std::bad_alloc.
        void* allocate_array(std::size_t n)
        {
            static_assert(std::is_same<pool_type, array_pool>::value,
                        "does not support array allocations");
            return allocate_array(n, free_list_.node_size());
        }

        /// \brief Deallocates a single node from the pool.
        void deallocate_node(void *ptr) FOONATHAN_NOEXCEPT
        {
            detail::deallocate(pool_type{}, free_list_, ptr);
        }

        /// \brief Deallocates an array of nodes from the pool.
        void deallocate_array(void *ptr, std::size_t n) FOONATHAN_NOEXCEPT
        {
            static_assert(std::is_same<pool_type, array_pool>::value,
                        "does not support array allocations");
            free_list_.deallocate_ordered(ptr, n);
        }

        /// \brief Returns the size of each node in the pool.
        std::size_t node_size() const FOONATHAN_NOEXCEPT
        {
            return free_list_.node_size();
        }

        /// \brief Returns the capacity remaining in the current block.
        /// \details This is the number of bytes remaining.
        /// Divide it by the \ref node_size() to get the number of nodes.
        std::size_t capacity() const FOONATHAN_NOEXCEPT
        {
            return free_list_.capacity() * node_size();
        }

        /// \brief Returns the size of the next memory block.
        /// \details This is the new capacity after \ref capacity() is exhausted.<br>
        /// This is also the maximum array size.
        std::size_t next_capacity() const FOONATHAN_NOEXCEPT
        {
            return block_list_.next_block_size() / node_size();
        }

        /// \brief Returns the \ref impl_allocator.
        impl_allocator& get_impl_allocator() FOONATHAN_NOEXCEPT
        {
            return block_list_.get_allocator();
        }

    private:
        void allocate_block()
        {
            auto mem = block_list_.allocate();
            auto offset = detail::align_offset(mem.memory, detail::max_alignment);
            detail::debug_fill(mem.memory, offset, debug_magic::alignment_memory);
            detail::insert(pool_type{}, free_list_,
                        static_cast<char*>(mem.memory) + offset, mem.size - offset);
        }

        void* allocate_array(std::size_t n, std::size_t node_size)
        {
            auto empty = free_list_.empty();
            if (empty)
                allocate_block();
            auto mem = free_list_.allocate(n, node_size);
            if (!mem && !empty) // only one allocate_block() call
            {
                allocate_block();
                mem = free_list_.allocate(n, node_size);
            }
            assert(mem && "invalid array size");
            return mem;
        }

        detail::block_list<impl_allocator> block_list_;
        free_list free_list_;

        friend allocator_traits<memory_pool<PoolType, RawAllocator>>;
    };

    template <class Type, class Alloc>
    FOONATHAN_CONSTEXPR std::size_t memory_pool<Type, Alloc>::min_node_size;

    /// \brief Specialization of the \ref allocator_traits for a \ref memory_pool.
    /// \details This allows passing a pool directly as allocator to container types.
    /// \ingroup memory
    template <typename PoolType, class ImplRawAllocator>
    class allocator_traits<memory_pool<PoolType, ImplRawAllocator>>
    {
    public:
        using allocator_type = memory_pool<PoolType, ImplRawAllocator>;
        using is_stateful = std::true_type;

        /// @{
        /// \brief Allocation functions forward to the pool allocation functions.
        /// \details Size and alignment of the nodes are ignored, since the pool handles it.
        static void* allocate_node(allocator_type &state,
                                std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            assert(alignment <= std::min(size,  detail::max_alignment) && "invalid alignment");
            return state.allocate_node();
        }

        static void* allocate_array(allocator_type &state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            assert(alignment <= max_alignment(state) && "invalid alignment");
            assert(count * size <= max_array_size(state) && "invalid array size");
            return allocate_array(PoolType{}, state, count, size);
        }
        /// @}

        /// @{
        /// \brief Deallocation functions forward to the pool deallocation functions.
        static void deallocate_node(allocator_type &state,
                    void *node, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.deallocate_node(node);
        }

        static void deallocate_array(allocator_type &state,
                    void *array, std::size_t count, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            deallocate_array(PoolType{}, state, array, count, size);
        }
        /// @}

        /// \brief Maximum size of a node is the pool's node size.
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.node_size();
        }

        /// \brief Maximum size of an array is the capacity in the next block of the pool.
        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.next_capacity();
        }

        /// \brief Maximum alignment is \c std::min(node_size(), alignof(std::max_align_t).
        static std::size_t max_alignment(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return std::min(state.node_size(),  detail::max_alignment);
        }

    private:
        static void* allocate_array(std::false_type, allocator_type &,
                                    std::size_t, std::size_t)
        {
            assert(!"array allocations not supported");
            return nullptr;
        }

        static void* allocate_array(std::true_type, allocator_type &state,
                                    std::size_t count, std::size_t size)
        {
            return state.allocate_array(count, size);
        }

        static void deallocate_array(std::false_type, allocator_type &,
                                void *, std::size_t, std::size_t)
        {
            assert(!"array allocations not supported");
        }

        static void deallocate_array(std::true_type, allocator_type &state,
                                void *array, std::size_t count, std::size_t size)
        {
            state.free_list_.deallocate_ordered(array, count, size);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED
