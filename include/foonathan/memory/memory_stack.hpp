// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_stack and its \ref foonathan::memory::allocator_traits specialization.

#include <cstdint>
#include <type_traits>

#include "detail/memory_stack.hpp"
#include "allocator_traits.hpp"
#include "debugging.hpp"
#include "error.hpp"
#include "memory_arena.hpp"

namespace foonathan { namespace memory
{
#if !defined(DOXYGEN)
    template <class Impl>
    class memory_stack;
#endif

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

    /// A stateful \concept{concept_rawallocator,RawAllocator} that provides stack-like (LIFO) allocations.
    /// It uses a \ref memory_arena with a given \c BlockOrRawAllocator defaulting to \ref growing_block_allocator<default_allocator> to allocate huge blocks
    /// and saves a marker to the current top.
    /// Allocation simply moves this marker by the appropriate number of bytes and returns the pointer at the old marker position,
    /// deallocation is not directly supported, only setting the marker to a previously queried position.
    /// \ingroup memory
    template <class BlockOrRawAllocator = default_allocator>
    class memory_stack
    : FOONATHAN_EBO(detail::leak_checker<memory_stack<default_allocator>>)
    {
        using leak_checker = detail::leak_checker<memory_stack<default_allocator>>;
    public:
        using allocator_type = make_block_allocator_t<BlockOrRawAllocator>;

        /// \effects Creates it with a given initial block size and and other constructor arguments for the block allocator.
        /// It will allocate the first block and sets the top to its beginning.
        template <typename ... Args>
        explicit memory_stack(std::size_t block_size,
                        Args&&... args)
        : leak_checker(info().name),
          arena_(block_size, detail::forward<Args>(args)...)
        {
            allocate_block();
        }

        /// \effects Allocates a memory block of given size and alignment.
        /// It simply moves the top marker.
        /// If there is not enough space on the current memory block,
        /// a new one will be allocated by the block allocator or taken from a cache
        /// and used for the allocation.
        /// \returns A \concept{concept_node,node} with given size and alignment.
        /// \throws Anything thrown by the block allocator on growth
        /// or \ref bad_allocation_size if \c size is too big.
        /// \requires \c size and \c alignment must be valid.
        void* allocate(std::size_t size, std::size_t alignment)
        {
            detail::check_allocation_size(size, next_capacity(), info());
            auto mem = stack_.allocate(size, alignment);
            if (!mem)
            {
                allocate_block();
                mem = stack_.allocate(size, alignment);
                FOONATHAN_MEMORY_ASSERT(mem);
            }
            return mem;
        }

        /// The marker type that is used for unwinding.
        /// The exact type is implementation defined,
        /// it is only required that it is copyable.
        using marker = FOONATHAN_IMPL_DEFINED(detail::stack_marker);

        /// \returns A marker to the current top of the stack.
        marker top() const FOONATHAN_NOEXCEPT
        {
            return {arena_.size() - 1, stack_};
        }

        /// \effects Unwinds the stack to a certain marker position.
        /// This sets the top pointer of the stack to the position described by the marker
        /// and has the effect of deallocating all memory allocated since the marker was obtained.
        /// If any memory blocks are unused after the operation,
        /// they are not deallocated but put in a cache for later use,
        /// call \ref shrink_to_fit() to actually deallocate them.
        /// \requires The marker must point to memory that is still in use and was the whole time,
        /// i.e. it must have been pointed below the top at all time.
        void unwind(marker m) FOONATHAN_NOEXCEPT
        {
            detail::check_pointer(m.index <= arena_.size() - 1, info(), m.top);

            if (std::size_t to_deallocate = (arena_.size() - 1) - m.index) // different index
            {
                arena_.deallocate_block();
                for (std::size_t i = 1; i != to_deallocate; ++i)
                    arena_.deallocate_block();

            #if FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
                auto cur = arena_.current_block();
                detail::check_pointer(m.end == static_cast<char*>(cur.memory) + cur.size,
                                      info(), m.top);
            #endif

                // mark memory from new top to end of the block as freed
                detail::debug_fill(m.top, std::size_t(m.end - m.top), debug_magic::freed_memory);
                stack_ = {m.top, m.end};
            }
            else // same index
            {
                detail::check_pointer(stack_.top() >= m.top, info(), m.top);
                stack_.unwind(m.top);
            }
        }

        /// \effects \ref unwind() does not actually do any deallocation of blocks on the block allocator,
        /// unused memory is stored in a cache for later reuse.
        /// This function clears that cache.
        void shrink_to_fit() FOONATHAN_NOEXCEPT
        {
            arena_.shrink_to_fit();
        }

        /// \returns The amount of memory remaining in the current block.
        /// This is the number of bytes that are available for allocation
        /// before the cache or block allocator needs to be used.
        std::size_t capacity() const FOONATHAN_NOEXCEPT
        {
            return std::size_t(stack_.end() - stack_.top());
        }

        /// \returns The size of the next memory block after the free list gets empty and the arena grows.
        /// This function just forwards to the \ref memory_arena.
        /// \note Due to fence memory, alignment buffers and the like this may not be the exact result \ref capacity() will return,
        /// but it is an upper bound to it.
        std::size_t next_capacity() const FOONATHAN_NOEXCEPT
        {
            return arena_.next_block_size();
        }

        /// \returns A reference to the block allocator used for managing the arena.
        /// \requires It is undefined behavior to move this allocator out into another object.
        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return arena_.get_allocator();
        }

    private:
        allocator_info info() const FOONATHAN_NOEXCEPT
        {
            return {FOONATHAN_MEMORY_LOG_PREFIX "::memory_stack", this};
        }

        void allocate_block()
        {
            auto block = arena_.allocate_block();
            stack_ = detail::fixed_memory_stack(block.memory, block.size);
        }

        memory_arena<allocator_type> arena_;
        detail::fixed_memory_stack stack_;

        friend allocator_traits<memory_stack<BlockOrRawAllocator>>;
    };

    /// Specialization of the \ref allocator_traits for \ref memory_stack classes.
    /// \note It is not allowed to mix calls through the specialization and through the member functions,
    /// i.e. \ref memory_stack::allocate() and this \c allocate_node().
    /// \ingroup memory
    template <class ImplRawAllocator>
    class allocator_traits<memory_stack<ImplRawAllocator>>
    {
    public:
        using allocator_type = memory_stack<ImplRawAllocator>;
        using is_stateful = std::true_type;

        /// \returns The result of \ref memory_stack::allocate().
        static void* allocate_node(allocator_type &state, std::size_t size, std::size_t alignment)
        {
            auto mem = state.allocate(size, alignment);
            state.on_allocate(size);
            return mem;
        }

        /// \returns The result of \ref memory_stack::allocate().
        static void* allocate_array(allocator_type &state, std::size_t count,
                                std::size_t size, std::size_t alignment)
        {
            return allocate_node(state, count * size, alignment);
        }

        /// @{
        /// \effects Does nothing besides bookmarking for leak checking, if that is enabled.
        /// Actual deallocation can only be done via \ref memory_stack::unwind().
        static void deallocate_node(allocator_type &state,
                    void *, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.on_deallocate(size);
        }

        static void deallocate_array(allocator_type &state,
                    void *ptr, std::size_t count, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            deallocate_node(state, ptr, count * size, alignment);
        }
        /// @}

        /// @{
        /// \returns The maximum size which is \ref memory_stack::next_capacity().
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.next_capacity();
        }

        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.next_capacity();
        }
        /// @}

        /// \returns The maximum possible value since there is no alignment restriction
        /// (except indirectly through \ref memory_stack::next_capacity()).
        static std::size_t max_alignment(const allocator_type &) FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED
