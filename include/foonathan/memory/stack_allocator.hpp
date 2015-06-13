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
#include "debugging.hpp"
#include "default_allocator.hpp"
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
            char *top;
            const char *end;

            stack_marker(std::size_t i, const detail::fixed_memory_stack &s) FOONATHAN_NOEXCEPT
            : index(i), top(s.top()), end(s.end()) {}

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
    template <class RawAllocator = default_allocator>
    class memory_stack : detail::leak_checker<memory_stack<default_allocator>>
    {
    public:
        /// \brief The implementation allocator.
        using impl_allocator = RawAllocator;

        /// \brief Constructs it with a given start block size.
        /// \details The first memory block is allocated, the block size can change.
        explicit memory_stack(std::size_t block_size,
                        impl_allocator allocator = impl_allocator())
        : detail::leak_checker<memory_stack<default_allocator>>("foonathan::memory::memory_stack"),
          list_(block_size, std::move(allocator))
        {
            allocate_block();
        }

        /// \brief Allocates a memory block of given size and alignment.
        /// \details If it does not fit into the current block, a new one will be allocated.
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
        marker top() const FOONATHAN_NOEXCEPT
        {
            return {list_.size() - 1, stack_};
        }

        /// \brief Unwinds the stack to a certain marker.
        /// \details It must be less than the previous one.
        /// Any access blocks are not directly freed but cached.
        /// Use \ref shrink_to_fit() to actually free them.
        void unwind(marker m) FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_IMPL_POINTER_CHECK(m.index > list_.size() - 1,
                            "foonathan::memory::stack_allocator", this, m.top);

            if (std::size_t to_deallocate = (list_.size() - 1) - m.index) // different index
            {
                list_.deallocate(stack_.top()); // top block only used up to the top of the stack
                for (std::size_t i = 1; i != to_deallocate; ++i)
                    list_.deallocate(); // other blocks fully used

                FOONATHAN_MEMORY_IMPL_POINTER_CHECK(m.end != list_.top().end(),
                            "foonathan::memory::stack_allocator", this, m.top);

                // mark memory from new top to end of the block as freed
                detail::debug_fill(m.top, std::size_t(m.end - m.top), debug_magic::freed_memory);
                stack_ = {m.top, m.end};
            }
            else // same index
            {
                FOONATHAN_MEMORY_IMPL_POINTER_CHECK(stack_.top() < m.top,
                            "foonathan::memory::stack_allocator", this, m.top);
                stack_.unwind(m.top);
            }
        }

        /// \brief Returns the capacity remaining in the current block.
        std::size_t capacity() const FOONATHAN_NOEXCEPT
        {
            return std::size_t(stack_.end() - stack_.top());
        }

        /// \brief Returns the size of the next memory block.
        /// \details This is the new capacity after \ref capacity() is exhausted.<br>
        /// This is also the maximum array size.
        std::size_t next_capacity() const FOONATHAN_NOEXCEPT
        {
            return list_.next_block_size();
        }

        /// \brief Frees all unused memory blocks.
        void shrink_to_fit() FOONATHAN_NOEXCEPT
        {
            list_.shrink_to_fit();
        }

        /// \brief Returns the \ref impl_allocator.
        impl_allocator& get_impl_allocator() FOONATHAN_NOEXCEPT
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

        friend allocator_traits<memory_stack<impl_allocator>>;
    };

    /// \brief Specialization of the \ref allocator_traits for a \ref memory_stack.
    /// \details This allows passing a state directly as allocator to container types.
    /// \note This interface provides leak checking, if you call their allocate functions, you need to call deallocate.
    /// Use the direct stack interface to prevent it.
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
            state.on_allocate(size);
            return state.allocate(size, alignment);
        }

        static void* allocate_array(allocator_type &state, std::size_t count,
                                std::size_t size, std::size_t alignment)
        {
            state.on_allocate(count * size);
            return allocate_node(state, count * size, alignment);
        }
        /// @}

        /// @{
        /// \brief Deallocation functions do nothing, use unwinding on the stack to free memory.
        static void deallocate_node(allocator_type &state,
                    void *, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.on_deallocate(size);
        }

        static void deallocate_array(allocator_type &state,
                    void *, std::size_t count , std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.on_deallocate(count * size);
        }
        /// @}

        /// @{
        /// \brief The maximum size is the equivalent of the \ref next_capacity().
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.next_capacity();
        }

        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.next_capacity();
        }
        /// @}

        /// \brief There is no maximum alignment (except indirectly through \ref next_capacity()).
        static std::size_t max_alignment(const allocator_type &) FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED
