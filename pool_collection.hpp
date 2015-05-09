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
#include "detail/free_list.hpp"
#include "detail/memory_stack.hpp"
#include "allocator_traits.hpp"
#include "default_allocator.hpp"
#include "pool_type.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {        
        class free_list_array
        {
        public:
            free_list_array(fixed_memory_stack &stack,
                        std::size_t max_node_size) FOONATHAN_NOEXCEPT;          
            
            free_memory_list& get_node(std::size_t node_size) FOONATHAN_NOEXCEPT;
            const free_memory_list& get_node(std::size_t node_size) const FOONATHAN_NOEXCEPT;
            
            free_memory_list& get_array(std::size_t node_size) FOONATHAN_NOEXCEPT;
            const free_memory_list& get_array(std::size_t node_size) const FOONATHAN_NOEXCEPT;
            
            // no of pools
            std::size_t size() const FOONATHAN_NOEXCEPT
            {
                return std::size_t(arrays_ - nodes_);
            }
            
            std::size_t max_node_size() const FOONATHAN_NOEXCEPT;
            
        private:
            free_memory_list *nodes_, *arrays_;
            // arrays_ - nodes_ == number of memory lists after both pointers
        };
    } // namespace detail
    
    /// \brief Manages multiple memory pools, each with a fixed size.
    /// \details This allows allocating of nodes of various sizes.<br>
    /// Otherwise behaves the same as \ref memory_pool.
    /// \ingroup memory
    template <class RawAllocator = default_allocator>
    class memory_pool_collection
    { 
    public:
        using impl_allocator = RawAllocator;
        
        /// \brief Creates a new pool collection with given max node size the memory block size.
        /// \details It can handle node sizes up to a given size.<br>
        /// The first memory block is allocated, the block size can change.
        memory_pool_collection(std::size_t max_node_size, std::size_t block_size,
                    impl_allocator alloc = impl_allocator())
        : block_list_(block_size, std::move(alloc)),
          stack_(block_list_.allocate()),
          pools_(stack_, max_node_size)
        {}
        
        /// \brief Allocates a node of given size.
        /// \details It selects the smallest node pool with sufficient size,
        /// the size must be smaller than the maximum node size.
        void* allocate_node(std::size_t node_size)
        {
            auto& pool = pools_.get_node(node_size);
            if (pool.empty())
                reserve_impl(pool, def_capacity(), &detail::free_memory_list::insert);
            return pool.allocate();
        }
        
        /// \brief Allocates an array of given node size and number of elements.
        /// \details It selects the smallest node pool with sufficient size,
        /// the size must be smaller than the maximum node size.
        void* allocate_array(std::size_t count, std::size_t node_size)
        {
            auto& pool = pools_.get_array(node_size);
            if (pool.empty())
                reserve_impl(pool, def_capacity(), &detail::free_memory_list::insert_ordered);
            auto n = detail::free_memory_list::calc_block_count
                            (pool.node_size(), count, node_size);
            return pool.allocate(n);
        }
        
        /// @{
        /// \brief Deallocates the memory into the appropriate pool.
        void deallocate_node(void *memory, std::size_t node_size) FOONATHAN_NOEXCEPT
        {
            pools_.get_node(node_size).deallocate(memory);
        }
        
        void deallocate_array(void *memory, std::size_t count, std::size_t node_size) FOONATHAN_NOEXCEPT
        {
            auto& pool = pools_.get_array(node_size);
            auto n = detail::free_memory_list::calc_block_count
                            (pool.node_size(), count, node_size);
            pool.deallocate_ordered(memory, n);
        }
        /// @}
        
        /// @{
        /// \brief Reserves memory for the node/array pool for a given node size.
        /// \details Use the \ref node_pool or \ref array_pool parameter to check it.
        void reserve(node_pool, std::size_t node_size, std::size_t capacity)
        {
            auto& pool = pools_.get_node(node_size);
            reserve_impl(pool, capacity, &detail::free_memory_list::insert);
        }
        
        void reserve(array_pool, std::size_t node_size, std::size_t capacity)
        {
            auto& pool = pools_.get_array(node_size);
            reserve_impl(pool, capacity, &detail::free_memory_list::insert_ordered);
        }
        /// @}
        
        /// \brief Returns the maximum node size for which there is a pool.
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT
        {
            return pools_.max_node_size();
        }
        
        /// @{ 
        /// \brief Returns the capacity available in the node/array pool for a given node size.
        /// \details This is the amount of nodes available inside the given pool.<br>
        /// Use the \ref node_pool or \ref array_pool parameter to check it.
        std::size_t pool_capacity(node_pool, std::size_t node_size) const FOONATHAN_NOEXCEPT
        {
            return pools_.get_node(node_size).capacity();
        }
        
        std::size_t pool_capacity(array_pool, std::size_t node_size) const FOONATHAN_NOEXCEPT
        {
            return pools_.get_array(node_size).capacity();
        }
        /// @}
        
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
            return block_list_.next_block_size() / (pools_.size() * 2);
        }
    
        void reserve_impl(detail::free_memory_list &pool, std::size_t capacity,
                        void (detail::free_memory_list::*insert)(void*, std::size_t))
        {
            auto mem = stack_.allocate(capacity,  detail::max_alignment);
            if (!mem)
            {
                // insert rest
                if (stack_.end() - stack_.top() != 0u)
                    (pool.*insert)(stack_.top(), stack_.end() - stack_.top());
                stack_ = detail::fixed_memory_stack(block_list_.allocate());
                // allocate ensuring alignment
                mem = stack_.allocate(capacity,  detail::max_alignment);
                assert(mem);
            }
            // insert new
            (pool.*insert)(mem, capacity);
        }
    
        detail::block_list<RawAllocator> block_list_;
        detail::fixed_memory_stack stack_;
        detail::free_list_array pools_;
    };
    
    /// \brief Specialization of the \ref allocator_traits for a \ref memory_pool_collection.
    /// \details This allows passing a pool directly as allocator to container types.
    /// \ingroup memory
    template <class RawAllocator>
    class allocator_traits<memory_pool_collection<RawAllocator>>
    {
    public:
        using allocator_type = memory_pool_collection<RawAllocator>;
        using is_stateful = std::true_type;
        
        static void* allocate_node(allocator_type &state,
                                std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            assert(alignment <=  detail::max_alignment && "invalid alignment");
            return state.allocate_node(size);
        }
        
        static void* allocate_array(allocator_type &state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            assert(alignment <=  detail::max_alignment && "invalid alignment");
            assert(count * size <= max_array_size(state) && "invalid array count");
            return state.allocate_array(count, size);
        }
        
        static void deallocate_node(allocator_type &state,
                    void *node, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.deallocate_node(node, size);
        }
        
        static void deallocate_array(allocator_type &state,
                    void *array, std::size_t count, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.deallocate_array(array, count, size);
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
