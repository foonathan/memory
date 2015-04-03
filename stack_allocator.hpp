// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A stack allocator.

#include <cassert>
#include <cstdint>
#include <type_traits>

#include "detail/block_list.hpp"
#include "detail/memory_stack.hpp"
#include "allocator_traits.hpp"
#include "heap_allocator.hpp"
#include "raw_allocator_base.hpp"

namespace foonathan { namespace memory
{    
    template <class Impl>
    class memory_stack;
    
    namespace detail
    {
    	class stack_marker
        {
            std::size_t index;
            detail::fixed_memory_stack stack;
            
            stack_marker(std::size_t i, detail::fixed_memory_stack stack) noexcept
            : index(i), stack(stack) {}
            
            template <class Impl>
            friend class memory::memory_stack;
        };
    } // namespace detail
    
    /// \brief A memory stack.
    ///
    /// Allows fast memory allocations but deallocation is only possible via markers.
    /// All memory after a marker is then freed, too.<br>
    /// It is no \ref concept::RawAllocator, but the \ref allocator_traits are specialized for it.<br>
    /// It allocates big blocks from an implementation allocator.
    /// If their size is sufficient, allocations are fast.
    /// \ingroup memory
    template <class RawAllocator = heap_allocator>
    class memory_stack
    {
    public:
        /// \brief The implementation allocator.
        using impl_allocator = RawAllocator;
    
        /// \brief Constructs it with a given start block size.
        /// \detail The first memory block is allocated, the block size can change.
        explicit memory_stack(std::size_t block_size,
                        impl_allocator allocator = impl_allocator())
        : list_(block_size, std::move(allocator))
        {
            allocate_block();
        }
        
        /// \brief Allocates a memory block of given size and alignment.
        /// \detail If it does not fit into the current block, a new one will be allocated.
        /// The new block must be big enough for the requested memory.
        void* allocate(std::size_t size, std::size_t alignment)
        {
            auto mem = stack_.allocate(size, alignment);
            if (!mem)
            {
                allocate_block();
                mem = stack_.allocate(size, alignment);
                assert(mem && "block size still too small");
            }
            return mem;
        }
        
        /// \brief Marker type for unwinding.
        using marker = detail::stack_marker;
        
        /// \brief Returns a marker to the current top of the stack.
        marker top() const noexcept
        {
            return {list_.size() - 1, stack_};
        }
        
        /// \brief Unwinds the stack to a certain marker.
        /// \detail It must be less than the previous one.
        /// Any access blocks are freed.
        void unwind(marker m) noexcept
        {
            auto diff = list_.size() - m.index - 1;
            for (auto i = 0u; i != diff; ++i)
                list_.deallocate();
            stack_ = m.stack;
        }
        
        /// \brief Returns the capacity remaining in the current block.
        std::size_t capacity() const noexcept
        {
            return stack_.end() - stack_.top();
        }
        
        /// \brief Returns the size of the next memory block.
        /// \detail This is the new capacity after \ref capacity() is exhausted.<br>
        /// This is also the maximum array size.
        std::size_t next_capacity() const noexcept
        {
            return list_.next_block_size();
        }
        
        /// \brief Frees all unused memory blocks.
        void shrink_to_fit() noexcept
        {
            list_.shrink_to_fit();
        }
        
        /// \brief Returns the \ref impl_allocator.
        impl_allocator& get_impl_allocator() noexcept
        {
            return list_.get_allocator();
        }
        
    private:        
        void allocate_block()
        {
            auto block = list_.allocate();
            stack_ = detail::fixed_memory_stack(block.memory, block.size);
        }
    
        detail::block_list<impl_allocator> list_;
        detail::fixed_memory_stack stack_;
    };
    
    /// \brief Specialization of the \ref allocator_traits for a \ref memory_stack.
    /// \detail This allows passing a state directly as allocator to container types.
    /// \ingroup memory
    template <class ImplRawAllocator>
    class allocator_traits<memory_stack<ImplRawAllocator>>
    {
    public:
        using allocator_type = memory_stack<ImplRawAllocator>;
        using is_stateful = std::true_type;
        
        /// @{
        /// \brief Allocation function forward to the stack for array and node.
        static void* allocate_node(allocator_type &state, std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            return state.allocate(size, alignment);
        }
        
        static void* allocate_array(allocator_type &state, std::size_t count,
                                std::size_t size, std::size_t alignment)
        {
            return allocate_node(state, count * size, alignment);
        }
        /// @}
        
        /// @{
        /// \brief Deallocation functions do nothing, use unwinding on the stack to free memory.
        static void deallocate_node(const allocator_type &,
                    void *, std::size_t, std::size_t) noexcept {}
        
        static void deallocate_array(const allocator_type &,
                    void *, std::size_t, std::size_t, std::size_t) noexcept {}
        /// @}
        
        /// @{
        /// \brief The maximum size is the equivalent of the \ref next_capacity().
        static std::size_t max_node_size(const allocator_type &state) noexcept
        {
            return state.next_capacity();
        }
        
        static std::size_t max_array_size(const allocator_type &state) noexcept
        {
            return state.next_capacity();
        }
        /// @}
        
        /// \brief There is no maximum alignment (except indirectly through \ref next_capacity()).
        static std::size_t max_alignment(const allocator_type &) noexcept
        {
            return std::size_t(-1);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED
