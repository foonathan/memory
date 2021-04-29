// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_stack and its \ref foonathan::memory::allocator_traits specialization.

// Inform that foonathan::memory::memory_stack::min_block_size API is available
#define FOONATHAN_MEMORY_MEMORY_STACK_HAS_MIN_BLOCK_SIZE

#include <cstdint>
#include <type_traits>

#include "detail/assert.hpp"
#include "detail/memory_stack.hpp"
#include "config.hpp"
#include "error.hpp"
#include "memory_arena.hpp"

namespace foonathan
{
    namespace memory
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
                char*       top;
                const char* end;

                stack_marker(std::size_t i, const detail::fixed_memory_stack& s,
                             const char* e) noexcept
                : index(i), top(s.top()), end(e)
                {
                }

                friend bool operator==(const stack_marker& lhs, const stack_marker& rhs) noexcept
                {
                    if (lhs.index != rhs.index)
                        return false;
                    FOONATHAN_MEMORY_ASSERT_MSG(lhs.end == rhs.end, "you must not compare two "
                                                                    "stack markers from different "
                                                                    "stacks");
                    return lhs.top == rhs.top;
                }

                friend bool operator!=(const stack_marker& lhs, const stack_marker& rhs) noexcept
                {
                    return !(rhs == lhs);
                }

                friend bool operator<(const stack_marker& lhs, const stack_marker& rhs) noexcept
                {
                    if (lhs.index != rhs.index)
                        return lhs.index < rhs.index;
                    FOONATHAN_MEMORY_ASSERT_MSG(lhs.end == rhs.end, "you must not compare two "
                                                                    "stack markers from different "
                                                                    "stacks");
                    return lhs.top < rhs.top;
                }

                friend bool operator>(const stack_marker& lhs, const stack_marker& rhs) noexcept
                {
                    return rhs < lhs;
                }

                friend bool operator<=(const stack_marker& lhs, const stack_marker& rhs) noexcept
                {
                    return !(rhs < lhs);
                }

                friend bool operator>=(const stack_marker& lhs, const stack_marker& rhs) noexcept
                {
                    return !(lhs < rhs);
                }

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
        /// \ingroup allocator
        template <class BlockOrRawAllocator = default_allocator>
        class memory_stack
        : FOONATHAN_EBO(detail::default_leak_checker<detail::memory_stack_leak_handler>)
        {
        public:
            using allocator_type = make_block_allocator_t<BlockOrRawAllocator>;

            /// \returns The minimum block size required for a stack containing the given amount of memory.
            /// If a stack is created with the result of `min_block_size(n)`, the resulting capacity will be exactly `n`.
            /// \requires `byte_size` must be a positive number.
            /// \note Due to debug fence sizes, the actual amount of usable memory can vary.
            /// However, this is impossible to compute without knowing the exact allocation pattern before,
            /// so this is just a rough estimate.
            static constexpr std::size_t min_block_size(std::size_t byte_size) noexcept
            {
                return detail::memory_block_stack::implementation_offset() + byte_size;
            }

            /// \effects Creates it with a given initial block size and and other constructor arguments for the \concept{concept_blockallocator,BlockAllocator}.
            /// It will allocate the first block and sets the top to its beginning.
            /// \requires \c block_size must be at least \c min_block_size(1).
            template <typename... Args>
            explicit memory_stack(std::size_t block_size, Args&&... args)
            : arena_(block_size, detail::forward<Args>(args)...),
              stack_(arena_.allocate_block().memory)
            {
            }

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
                auto fence  = detail::debug_fence_size;
                auto offset = detail::align_offset(stack_.top() + fence, alignment);

                if (!stack_.top()
                    || fence + offset + size + fence > std::size_t(block_end() - stack_.top()))
                {
                    // need to grow
                    auto block = arena_.allocate_block();
                    stack_     = detail::fixed_memory_stack(block.memory);

                    // new alignment required for over-aligned types
                    offset = detail::align_offset(stack_.top() + fence, alignment);

                    auto needed = fence + offset + size + fence;
                    detail::check_allocation_size<bad_allocation_size>(needed, block.size, info());
                }

                return stack_.allocate_unchecked(size, offset);
            }

            /// \effects Allocates a memory block of given size and alignment,
            /// similar to \ref allocate().
            /// But it does not attempt a growth if the arena is empty.
            /// \returns A \concept{concept_node,node} with given size and alignment
            /// or `nullptr` if there wasn't enough memory available.
            void* try_allocate(std::size_t size, std::size_t alignment) noexcept
            {
                return stack_.allocate(block_end(), size, alignment);
            }

            /// The marker type that is used for unwinding.
            /// The exact type is implementation defined,
            /// it is only required that it is efficiently copyable
            /// and has all the comparision operators defined for two markers on the same stack.
            /// Two markers are equal, if they are copies or created from two `top()` calls without a call to `unwind()` or `allocate()`.
            /// A marker `a` is less than marker `b`, if after `a` was obtained, there was one or more call to `allocate()` and no call to `unwind()`.
            using marker = FOONATHAN_IMPL_DEFINED(detail::stack_marker);

            /// \returns A marker to the current top of the stack.
            marker top() const noexcept
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
            void unwind(marker m) noexcept
            {
                FOONATHAN_MEMORY_ASSERT(m <= top());
                detail::debug_check_pointer([&] { return m.index <= arena_.size() - 1; }, info(),
                                            m.top);

                if (std::size_t to_deallocate = (arena_.size() - 1) - m.index) // different index
                {
                    arena_.deallocate_block();
                    for (std::size_t i = 1; i != to_deallocate; ++i)
                        arena_.deallocate_block();

                    detail::debug_check_pointer(
                        [&] {
                            auto cur = arena_.current_block();
                            return m.end == static_cast<char*>(cur.memory) + cur.size;
                        },
                        info(), m.top);

                    // mark memory from new top to end of the block as freed
                    detail::debug_fill_free(m.top, std::size_t(m.end - m.top), 0);
                    stack_ = detail::fixed_memory_stack(m.top);
                }
                else // same index
                {
                    detail::debug_check_pointer([&] { return stack_.top() >= m.top; }, info(),
                                                m.top);
                    stack_.unwind(m.top);
                }
            }

            /// \effects \ref unwind() does not actually do any deallocation of blocks on the \concept{concept_blockallocator,BlockAllocator},
            /// unused memory is stored in a cache for later reuse.
            /// This function clears that cache.
            void shrink_to_fit() noexcept
            {
                arena_.shrink_to_fit();
            }

            /// \returns The amount of memory remaining in the current block.
            /// This is the number of bytes that are available for allocation
            /// before the cache or \concept{concept_blockallocator,BlockAllocator} needs to be used.
            std::size_t capacity_left() const noexcept
            {
                return std::size_t(block_end() - stack_.top());
            }

            /// \returns The size of the next memory block after the current block is exhausted and the arena grows.
            /// This function just forwards to the \ref memory_arena.
            /// \note All of it is available for the stack to use, but due to fences and alignment buffers,
            /// this may not be the exact amount of memory usable for the user.
            std::size_t next_capacity() const noexcept
            {
                return arena_.next_block_size();
            }

            /// \returns A reference to the \concept{concept_blockallocator,BlockAllocator} used for managing the arena.
            /// \requires It is undefined behavior to move this allocator out into another object.
            allocator_type& get_allocator() noexcept
            {
                return arena_.get_allocator();
            }

        private:
            allocator_info info() const noexcept
            {
                return {FOONATHAN_MEMORY_LOG_PREFIX "::memory_stack", this};
            }

            const char* block_end() const noexcept
            {
                auto block = arena_.current_block();
                return static_cast<const char*>(block.memory) + block.size;
            }

            memory_arena<allocator_type> arena_;
            detail::fixed_memory_stack   stack_;

            friend allocator_traits<memory_stack<BlockOrRawAllocator>>;
            friend composable_allocator_traits<memory_stack<BlockOrRawAllocator>>;
        };

        /// Simple utility that automatically unwinds a `Stack` to a previously saved location.
        /// A `Stack` is anything that provides a `marker`, a `top()` function returning a `marker`
        /// and an `unwind()` function to unwind to a `marker`,
        /// like a \ref foonathan::memory::memory_stack
        /// \ingroup allocator
        template <class Stack = memory_stack<>>
        class memory_stack_raii_unwind
        {
        public:
            using stack_type  = Stack;
            using marker_type = typename stack_type::marker;

            /// \effects Same as `memory_stack_raii_unwind(stack, stack.top())`.
            explicit memory_stack_raii_unwind(stack_type& stack) noexcept
            : memory_stack_raii_unwind(stack, stack.top())
            {
            }

            /// \effects Creates the unwinder by giving it the stack and the marker.
            /// \requires The stack must live longer than this object.
            memory_stack_raii_unwind(stack_type& stack, marker_type marker) noexcept
            : marker_(marker), stack_(&stack)
            {
            }

            /// \effects Move constructs the unwinder by taking the saved position from `other`.
            /// `other.will_unwind()` will return `false` after it.
            memory_stack_raii_unwind(memory_stack_raii_unwind&& other) noexcept
            : marker_(other.marker_), stack_(other.stack_)
            {
                other.stack_ = nullptr;
            }

            /// \effects Unwinds to the previously saved location,
            /// if there is any, by calling `unwind()`.
            ~memory_stack_raii_unwind() noexcept
            {
                if (stack_)
                    stack_->unwind(marker_);
            }

            /// \effects Move assigns the unwinder by taking the saved position from `other`.
            /// `other.will_unwind()` will return `false` after it.
            memory_stack_raii_unwind& operator=(memory_stack_raii_unwind&& other) noexcept
            {
                if (stack_)
                    stack_->unwind(marker_);

                marker_ = other.marker_;
                stack_  = other.stack_;

                other.stack_ = nullptr;

                return *this;
            }

            /// \effects Removes the location without unwinding it.
            /// `will_unwind()` will return `false`.
            void release() noexcept
            {
                stack_ = nullptr;
            }

            /// \effects Unwinds to the saved location explictly.
            /// \requires `will_unwind()` must return `true`.
            void unwind() noexcept
            {
                FOONATHAN_MEMORY_ASSERT(will_unwind());
                stack_->unwind(marker_);
            }

            /// \returns Whether or not the unwinder will actually unwind.
            /// \note It will not unwind if it is in the moved-from state.
            bool will_unwind() const noexcept
            {
                return stack_ != nullptr;
            }

            /// \returns The saved marker, if there is any.
            /// \requires `will_unwind()` must return `true`.
            marker_type get_marker() const noexcept
            {
                FOONATHAN_MEMORY_ASSERT(will_unwind());
                return marker_;
            }

            /// \returns The stack it will unwind.
            /// \requires `will_unwind()` must return `true`.
            stack_type& get_stack() const noexcept
            {
                FOONATHAN_MEMORY_ASSERT(will_unwind());
                return *stack_;
            }

        private:
            marker_type marker_;
            stack_type* stack_;
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class memory_stack<>;
        extern template class memory_stack_raii_unwind<memory_stack<>>;
#endif

        /// Specialization of the \ref allocator_traits for \ref memory_stack classes.
        /// \note It is not allowed to mix calls through the specialization and through the member functions,
        /// i.e. \ref memory_stack::allocate() and this \c allocate_node().
        /// \ingroup allocator
        template <class BlockAllocator>
        class allocator_traits<memory_stack<BlockAllocator>>
        {
        public:
            using allocator_type = memory_stack<BlockAllocator>;
            using is_stateful    = std::true_type;

            /// \returns The result of \ref memory_stack::allocate().
            static void* allocate_node(allocator_type& state, std::size_t size,
                                       std::size_t alignment)
            {
                auto mem = state.allocate(size, alignment);
                state.on_allocate(size);
                return mem;
            }

            /// \returns The result of \ref memory_stack::allocate().
            static void* allocate_array(allocator_type& state, std::size_t count, std::size_t size,
                                        std::size_t alignment)
            {
                return allocate_node(state, count * size, alignment);
            }

            /// @{
            /// \effects Does nothing besides bookmarking for leak checking, if that is enabled.
            /// Actual deallocation can only be done via \ref memory_stack::unwind().
            static void deallocate_node(allocator_type& state, void*, std::size_t size,
                                        std::size_t) noexcept
            {
                state.on_deallocate(size);
            }

            static void deallocate_array(allocator_type& state, void* ptr, std::size_t count,
                                         std::size_t size, std::size_t alignment) noexcept
            {
                deallocate_node(state, ptr, count * size, alignment);
            }
            /// @}

            /// @{
            /// \returns The maximum size which is \ref memory_stack::next_capacity().
            static std::size_t max_node_size(const allocator_type& state) noexcept
            {
                return state.next_capacity();
            }

            static std::size_t max_array_size(const allocator_type& state) noexcept
            {
                return state.next_capacity();
            }
            /// @}

            /// \returns The maximum possible value since there is no alignment restriction
            /// (except indirectly through \ref memory_stack::next_capacity()).
            static std::size_t max_alignment(const allocator_type&) noexcept
            {
                return std::size_t(-1);
            }
        };

        /// Specialization of the \ref composable_allocator_traits for \ref memory_stack classes.
        /// \ingroup allocator
        template <class BlockAllocator>
        class composable_allocator_traits<memory_stack<BlockAllocator>>
        {
        public:
            using allocator_type = memory_stack<BlockAllocator>;

            /// \returns The result of \ref memory_stack::try_allocate().
            static void* try_allocate_node(allocator_type& state, std::size_t size,
                                           std::size_t alignment) noexcept
            {
                return state.try_allocate(size, alignment);
            }

            /// \returns The result of \ref memory_stack::try_allocate().
            static void* try_allocate_array(allocator_type& state, std::size_t count,
                                            std::size_t size, std::size_t alignment) noexcept
            {
                return state.try_allocate(count * size, alignment);
            }

            /// @{
            /// \effects Does nothing.
            /// \returns Whether the memory will be deallocated by \ref memory_stack::unwind().
            static bool try_deallocate_node(allocator_type& state, void* ptr, std::size_t,
                                            std::size_t) noexcept
            {
                return state.arena_.owns(ptr);
            }

            static bool try_deallocate_array(allocator_type& state, void* ptr, std::size_t count,
                                             std::size_t size, std::size_t alignment) noexcept
            {
                return try_deallocate_node(state, ptr, count * size, alignment);
            }
            /// @}
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class allocator_traits<memory_stack<>>;
        extern template class composable_allocator_traits<memory_stack<>>;
#endif
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_MEMORY_STACK_HPP_INCLUDED
