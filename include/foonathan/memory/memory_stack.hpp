// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_stack and its \ref foonathan::memory::allocator_traits specialization.

#include <cstdint>
#include <type_traits>

#include "detail/assert.hpp"
#include "detail/memory_stack.hpp"
#include "config.hpp"
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

            stack_marker(std::size_t i, const detail::fixed_memory_stack &s, const char *end) FOONATHAN_NOEXCEPT
            : index(i), top(s.top()), end(end) {}

            template <class Impl>
            friend class memory::memory_stack;
        };

        struct memory_stack_leak_handler
        {
            void operator()(std::ptrdiff_t amount);
        };
    } // namespace detail

    /// A stateful \concept{concept_rawallocator,RawAllocator} that provides stack-like (LIFO) allocations.
    /// It uses a \ref memory_arena with a given \c BlockOrRawAllocator defaulting to \ref growing_block_allocator to allocate huge blocks
    /// and saves a marker to the current top.
    /// Allocation simply moves this marker by the appropriate number of bytes and returns the pointer at the old marker position,
    /// deallocation is not directly supported, only setting the marker to a previously queried position.
    /// \ingroup memory allocator
    template <class BlockOrRawAllocator = default_allocator>
    class memory_stack
    : FOONATHAN_EBO(detail::default_leak_checker<detail::memory_stack_leak_handler>)
    {
    public:
        using allocator_type = make_block_allocator_t<BlockOrRawAllocator>;

        /// \effects Creates it with a given initial block size and and other constructor arguments for the \concept{concept_blockallocator,BlockAllocator}.
        /// It will allocate the first block and sets the top to its beginning.
        template <typename ... Args>
        explicit memory_stack(std::size_t block_size,
                        Args&&... args)
        : arena_(block_size, detail::forward<Args>(args)...),
          stack_(arena_.allocate_block().memory)
        {}

        /// \effects Allocates a memory block of given size and alignment.
        /// It simply moves the top marker.
        /// If there is not enough space on the current memory block,
        /// a new one will be allocated by the \concept{concept_blockallocator,BlockAllocator} or taken from a cache
        /// and used for the allocation.
        /// \returns A \concept{concept_node,node} with given size and alignment.
        /// \throws Anything thrown by the \concept{concept_blockallocator,BlockAllocator} on growth
        /// or \ref bad_allocation_size if \c size is too big.
        /// \requires \c size and \c alignment must be valid.
        void* allocate(std::size_t size, std::size_t alignment)
        {
            detail::check_allocation_size(size, next_capacity(), info());

            auto fence = detail::debug_fence_size ? detail::max_alignment : 0u;
            auto offset = detail::align_offset(stack_.top(), alignment);

            if (fence + offset + size + fence <= std::size_t(block_end() - stack_.top()))
            {
                stack_.bump(fence, debug_magic::fence_memory);
                stack_.bump(offset, debug_magic::alignment_memory);
            }
            else
            {
                auto block = arena_.allocate_block();
                FOONATHAN_MEMORY_ASSERT_MSG(fence + size + fence <= block.size, "new block size not big enough");

                stack_ = detail::fixed_memory_stack(block.memory);
                // no need to align, block should be aligned for maximum
                stack_.bump(fence, debug_magic::fence_memory);
            }

            FOONATHAN_MEMORY_ASSERT(detail::is_aligned(stack_.top(), alignment));
            return stack_.bump_return(size);
        }

        /// The marker type that is used for unwinding.
        /// The exact type is implementation defined,
        /// it is only required that it is copyable.
        using marker = FOONATHAN_IMPL_DEFINED(detail::stack_marker);

        /// \returns A marker to the current top of the stack.
        marker top() const FOONATHAN_NOEXCEPT
        {
            return {arena_.size() - 1, stack_, block_end()};
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
            detail::debug_check_pointer([&]
                                        {
                                            return m.index <= arena_.size() - 1;
                                        }, info(), m.top);

            if (std::size_t to_deallocate = (arena_.size() - 1) - m.index) // different index
            {
                arena_.deallocate_block();
                for (std::size_t i = 1; i != to_deallocate; ++i)
                    arena_.deallocate_block();

                detail::debug_check_pointer([&]
                                            {
                                                auto cur = arena_.current_block();
                                                return m.end == static_cast<char*>(cur.memory) + cur.size;
                                            }, info(), m.top);

                // mark memory from new top to end of the block as freed
                detail::debug_fill_free(m.top, std::size_t(m.end - m.top), 0);
                stack_ = detail::fixed_memory_stack(m.top);
            }
            else // same index
            {
                detail::debug_check_pointer([&]
                                            {
                                                return stack_.top() >= m.top;
                                            }, info(), m.top);
                stack_.unwind(m.top);
            }
        }

        /// \effects \ref unwind() does not actually do any deallocation of blocks on the \concept{concept_blockallocator,BlockAllocator},
        /// unused memory is stored in a cache for later reuse.
        /// This function clears that cache.
        void shrink_to_fit() FOONATHAN_NOEXCEPT
        {
            arena_.shrink_to_fit();
        }

        /// \returns The amount of memory remaining in the current block.
        /// This is the number of bytes that are available for allocation
        /// before the cache or \concept{concept_blockallocator,BlockAllocator} needs to be used.
        std::size_t capacity_left() const FOONATHAN_NOEXCEPT
        {
            return std::size_t(block_end() - stack_.top());
        }

        /// \returns The size of the next memory block after the free list gets empty and the arena grows.
        /// This function just forwards to the \ref memory_arena.
        /// \note Due to fence memory, alignment buffers and the like this may not be the exact result \ref capacity_left() will return,
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
            return {FOONATHAN_MEMORY_LOG_PREFIX "::memory_stack", this};
        }

        const char* block_end() const FOONATHAN_NOEXCEPT
        {
            auto block = arena_.current_block();
            return static_cast<const char*>(block.memory) + block.size;
        }

        memory_arena<allocator_type> arena_;
        detail::fixed_memory_stack stack_;

        friend allocator_traits<memory_stack<BlockOrRawAllocator>>;
    };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
    extern template class memory_stack<>;
#endif

    template <class Allocator>
    class allocator_traits;

    /// Specialization of the \ref allocator_traits for \ref memory_stack classes.
    /// \note It is not allowed to mix calls through the specialization and through the member functions,
    /// i.e. \ref memory_stack::allocate() and this \c allocate_node().
    /// \ingroup memory allocator
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

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
    extern template class allocator_traits<memory_stack<>>;
#endif
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED
