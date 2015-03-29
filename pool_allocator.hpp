#ifndef FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A pool allocator.

#include <cassert>
#include <memory>
#include <type_traits>

#include "detail/align.hpp"
#include "detail/block_list.hpp"
#include "detail/free_list.hpp"
#include "allocator_traits.hpp"
#include "heap_allocator.hpp"
#include "pool_type.hpp"

namespace foonathan { namespace memory
{    
    /// \brief A memory pool.
    ///
    /// It manages nodes of fixed size.
    /// Allocating and deallocating such a node is really fast,
    /// but each has the given size.<br>
    /// There are two types: one that is faster, but does not support arrays,
    /// one that is slightly slower but does.
    /// Use the \ref node_pool or \ref array_pool type to select it.<br>
    /// It is no \ref concept::RawAllocator, but the \ref allocator_traits are specialized for it.<br>
    /// It allocates big blocks from an implementation allocator.
    /// If their size is sufficient, allocations are fast.
    /// \ingroup memory
    template <typename NodeOrArray, class RawAllocator = heap_allocator>
    class memory_pool
    {
        static_assert(std::is_same<NodeOrArray, node_pool>::value ||
                    std::is_same<NodeOrArray, array_pool>::value,
                    "invalid tag type");
    public:
        using impl_allocator = RawAllocator;
        
        /// \brief The type of the pool (\ref node_pool or \ref array_pool).
        // implementation node: pool_type::value is true for arrays
        using pool_type = NodeOrArray;
        
        /// \brief The minimum node size due to implementation reasons.
        static constexpr auto min_node_size = detail::free_memory_list::min_element_size;
        
        /// \brief Gives it the size of the nodes inside the pool and start block size.
        /// \detail The first memory block is allocated, the block size can change.
        memory_pool(std::size_t node_size, std::size_t block_size,
                    impl_allocator allocator = impl_allocator())
        : block_list_(block_size, std::move(allocator)),
          free_list_(node_size)
        {
            allocate_block();
        }
        
        /// \brief Allocates a single node from the pool.
        /// \detail It is aligned for \c std::min(node_size(), alignof(std::max_align_t).
        void* allocate_node()
        {
            if (free_list_.empty())
                allocate_block();
            return free_list_.allocate();
        }
        
        /// \brief Allocates an array from the pool.
        /// \detail Returns \c n subsequent nodes.<br>
        /// If not \ref array_pool, may fail, throwing \c std::bad_alloc.
        void* allocate_array(std::size_t n)
        {
            void *mem = nullptr;
            if (free_list_.empty())
            {
                allocate_block();
                mem = free_list_.allocate(n);
            }
            else
            {
                mem = free_list_.allocate(n);
                if (!mem)
                {
                    allocate_block();
                    mem = free_list_.allocate(n);
                }
            }
            assert(mem && "invalid array size");
            return mem;
        }
        
        /// \brief Deallocates a single node from the pool.
        void deallocate_node(void *ptr) noexcept
        {
            if (pool_type::value)
                free_list_.deallocate_ordered(ptr);
            else
                free_list_.deallocate(ptr);
        }
        
        /// \brief Deallocates an array of nodes from the pool.
        void deallocate_array(void *ptr, std::size_t n) noexcept
        {
            if (pool_type::value)
                free_list_.deallocate_ordered(ptr, n);
            else
                free_list_.deallocate(ptr, n);
        }
        
        /// \brief Returns the size of each node in the pool.
        std::size_t node_size() const noexcept
        {
            return free_list_.element_size();
        }
        
        /// \brief Returns the capacity remaining in the current block.
        /// \detail This is the pure byte size, divide it by \ref node_size() to get the number of bytes.
        std::size_t capacity() const noexcept
        {
            return free_list_.capacity();
        }
        
        /// \brief Returns the size of the next memory block.
        /// \detail This is the new capacity after \ref capacity() is exhausted.<br>
        /// This is also the maximum array size.
        std::size_t next_capacity() const noexcept
        {
            return block_list_.next_block_size();
        }
        
        /// \brief Returns the \ref impl_allocator.
        impl_allocator& get_impl_allocator() noexcept
        {
            return block_list_.get_allocator();
        }
        
    private:
        void allocate_block()
        {
            auto mem = block_list_.allocate("foonathan::memory::memory_pool");
            auto offset = detail::align_offset(mem.memory, alignof(std::max_align_t));
            mem.memory = static_cast<char*>(mem.memory) + offset;
            if (pool_type::value)
                free_list_.insert_ordered(mem.memory, mem.size);
            else
                free_list_.insert(mem.memory, mem.size);
            capacity_ = mem.size;
        }
    
        detail::block_list<impl_allocator> block_list_;
        detail::free_memory_list free_list_;
        std::size_t capacity_;
    };

    /// \brief Specialization of the \ref allocator_traits for a \ref memory_pool.
    /// \detail This allows passing a pool directly as allocator to container types.
    /// \ingroup memory
    template <typename NodeOrArray, class ImplRawAllocator>
    class allocator_traits<memory_pool<NodeOrArray, ImplRawAllocator>>
    {
    public:
        using allocator_type = memory_pool<NodeOrArray, ImplRawAllocator>;
        using is_stateful = std::true_type;
        
        /// @{
        /// \brief Allocation functions forward to the pool allocation functions.
        /// \detail Size and alignment of the nodes are ignored, since the pool handles it.
        static void* allocate_node(allocator_type &state,
                                std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            assert(alignment <= std::min(size, alignof(std::max_align_t)) && "invalid alignment");
            return state.allocate_node();
        }

        static void* allocate_array(allocator_type &state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            assert(alignment <= max_alignment(state) && "invalid alignment");
            assert(count * size <= max_array_size(state) && "invalid array size");
            return state.allocate_array(count);
        }
        /// @}

        /// @{
        /// \brief Deallocation functions forward to the pool deallocation functions.
        static void deallocate_node(allocator_type &state,
                    void *node, std::size_t, std::size_t) noexcept
        {
            state.deallocate_node(node);
        }

        static void deallocate_array(allocator_type &state,
                    void *array, std::size_t count, std::size_t, std::size_t) noexcept
        {
            state.deallocate_array(array, count);
        }
        /// @}

        /// \brief Maximum size of a node is the pool's node size.
        static std::size_t max_node_size(const allocator_type &state) noexcept
        {
            return state.node_size();
        }

        /// \brief Maximum size of an array is the capacity in the next block of the pool.
        static std::size_t max_array_size(const allocator_type &state) noexcept
        {
            return state.next_capacity();
        }
        
        /// \brief Maximum alignment is \c std::min(node_size(), alignof(std::max_align_t).
        static std::size_t max_alignment(const allocator_type &state) noexcept
        {
            return std::min(state.node_size(), alignof(std::max_align_t));
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED
