// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_POOL_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_POOL_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_pool and its \ref foonathan::memory::allocator_traits specialization.

#include <type_traits>

#include "detail/align.hpp"
#include "detail/debug_helpers.hpp"
#include "detail/assert.hpp"
#include "config.hpp"
#include "error.hpp"
#include "memory_arena.hpp"
#include "memory_pool_type.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            struct memory_pool_leak_handler
            {
                void operator()(std::ptrdiff_t amount);
            };
        } // namespace detail

        /// A stateful \concept{concept_rawallocator,RawAllocator} that manages \concept{concept_node,nodes} of fixed size.
        /// It uses a \ref memory_arena with a given \c BlockOrRawAllocator defaulting to \ref growing_block_allocator,
        /// subdivides them in small nodes of given size and puts them onto a free list.
        /// Allocation and deallocation simply remove or add nodes from this list and are thus fast.
        /// The way the list is maintained can be controlled via the \c PoolType
        /// which is either \ref node_pool, \ref array_pool or \ref small_node_pool.<br>
        /// This kind of allocator is ideal for fixed size allocations and deallocations in any order,
        /// for example in a node based container like \c std::list.
        /// It is not so good for different allocation sizes and has some drawbacks for arrays
        /// as described in \ref memory_pool_type.hpp.
        /// \ingroup memory allocator
        template <typename PoolType = node_pool, class BlockOrRawAllocator = default_allocator>
        class memory_pool
            : FOONATHAN_EBO(detail::default_leak_checker<detail::memory_pool_leak_handler>)
        {
            using free_list    = typename PoolType::type;
            using leak_checker = detail::default_leak_checker<detail::memory_pool_leak_handler>;

        public:
            using allocator_type = make_block_allocator_t<BlockOrRawAllocator>;
            using pool_type      = PoolType;

            static FOONATHAN_CONSTEXPR std::size_t min_node_size =
                FOONATHAN_IMPL_DEFINED(free_list::min_element_size);

            /// \effects Creates it by specifying the size each \concept{concept_node,node} will have,
            /// the initial block size for the arena and other constructor arguments for the \concept{concept_blockallocator,BlockAllocator}.
            /// If the \c node_size is less than the \c min_node_size, the \c min_node_size will be the actual node size.
            /// It will allocate an initial memory block with given size from the \concept{concept_blockallocator,BlockAllocator}
            /// and puts it onto the free list.
            /// \requires \c node_size must be a valid \concept{concept_node,node size}
            /// and \c block_size must be a non-zero value.
            template <typename... Args>
            memory_pool(std::size_t node_size, std::size_t block_size, Args&&... args)
            : arena_(block_size, detail::forward<Args>(args)...), free_list_(node_size)
            {
                allocate_block();
            }

            /// \effects Destroys the \ref memory_pool by returning all memory blocks,
            /// regardless of properly deallocated back to the \concept{concept_blockallocator,BlockAllocator}.
            ~memory_pool() FOONATHAN_NOEXCEPT
            {
            }

            /// @{
            /// \effects Moving a \ref memory_pool object transfers ownership over the free list,
            /// i.e. the moved from pool is completely empty and the new one has all its memory.
            /// That means that it is not allowed to call \ref deallocate_node() on a moved-from allocator
            /// even when passing it memory that was previously allocated by this object.
            memory_pool(memory_pool&& other) FOONATHAN_NOEXCEPT
                : leak_checker(detail::move(other)),
                  arena_(detail::move(other.arena_)),
                  free_list_(detail::move(other.free_list_))
            {
            }

            memory_pool& operator=(memory_pool&& other) FOONATHAN_NOEXCEPT
            {
                leak_checker::operator=(detail::move(other));
                arena_                = detail::move(other.arena_);
                free_list_            = detail::move(other.free_list_);
                return *this;
            }
            /// @}

            /// \effects Allocates a single \concept{concept_node,node} by removing it from the free list.
            /// If the free list is empty, a new memory block will be allocated from the arena and put onto it.
            /// The new block size will be \ref next_capacity() big.
            /// \returns A node of size \ref node_size() suitable aligned,
            /// i.e. suitable for any type where <tt>sizeof(T) < node_size()</tt>.
            /// \throws Anything thrown by the used \concept{concept_blockallocator,BlockAllocator}'s allocation function if a growth is needed.
            void* allocate_node()
            {
                if (free_list_.empty())
                    allocate_block();
                FOONATHAN_MEMORY_ASSERT(!free_list_.empty());
                return free_list_.allocate();
            }

            /// \effects Allocates a single \concept{concept_node,node} similar to \ref allocate_node().
            /// But if the free list is empty, a new block will *not* be allocated.
            /// \returns A suitable aligned node of size \ref node_size() or `nullptr`.
            void* try_allocate_node() FOONATHAN_NOEXCEPT
            {
                return free_list_.empty() ? nullptr : free_list_.allocate();
            }

            /// \effects Allocates an \concept{concept_array,array} of nodes by searching for \c n continuous nodes on the list and removing them.
            /// Depending on the \c PoolType this can be a slow operation or not allowed at all.
            /// This can sometimes lead to a growth, even if technically there is enough continuous memory on the free list.
            /// \returns An array of \c n nodes of size \ref node_size() suitable aligned.
            /// \throws Anything thrown by the used \concept{concept_blockallocator,BlockAllocator}'s allocation function if a growth is needed,
            /// or \ref bad_array_size if <tt>n * node_size()</tt> is too big.
            /// \requires \c n must be valid \concept{concept_array,array count}.
            void* allocate_array(std::size_t n)
            {
                detail::check_allocation_size<bad_array_size>(n * node_size(),
                                                              [&] {
                                                                  return pool_type::value ?
                                                                             next_capacity() :
                                                                             0;
                                                              },
                                                              info());
                return allocate_array(n, node_size());
            }

            /// \effects Allocates an \concept{concept_array,array| of nodes similar to \ref allocate_array().
            /// But it will never allocate a new memory block.
            /// \returns An array of \c n nodes of size \ref node_size() suitable aligned
            /// or `nullptr`.
            void* try_allocate_array(std::size_t n) FOONATHAN_NOEXCEPT
            {
                return try_allocate_array(n, node_size());
            }

            /// \effects Deallocates a single \concept{concept_node,node} by putting it back onto the free list.
            /// \requires \c ptr must be a result from a previous call to \ref allocate_node() on the same free list,
            /// i.e. either this allocator object or a new object created by moving this to it.
            void deallocate_node(void* ptr) FOONATHAN_NOEXCEPT
            {
                free_list_.deallocate(ptr);
            }

            /// \effects Deallocates a single \concept{concept_node,node} but it does not be a result of a previous call to \ref allocate_node().
            /// \returns `true` if the node could be deallocated, `false` otherwise.
            /// \note Some free list implementations can deallocate any memory,
            /// doesn't matter where it is coming from.
            bool try_deallocate_node(void* ptr) FOONATHAN_NOEXCEPT
            {
                if (!arena_.owns(ptr))
                    return false;
                free_list_.deallocate(ptr);
                return true;
            }

            /// \effects Deallocates an \concept{concept_array,array} by putting it back onto the free list.
            /// \requires \c ptr must be a result from a previous call to \ref allocate_array() with the same \c n on the same free list,
            /// i.e. either this allocator object or a new object created by moving this to it.
            void deallocate_array(void* ptr, std::size_t n) FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_ASSERT_MSG(pool_type::value, "does not support array allocations");
                free_list_.deallocate(ptr, n * node_size());
            }

            /// \effects Deallocates an \concept{concept_array,array} but it does not be a result of a previous call to \ref allocate_array().
            /// \returns `true` if the node could be deallocated, `false` otherwise.
            /// \note Some free list implementations can deallocate any memory,
            /// doesn't matter where it is coming from.
            bool try_deallocate_array(void* ptr, std::size_t n) FOONATHAN_NOEXCEPT
            {
                return try_deallocate_array(ptr, n, node_size());
            }

            /// \returns The size of each \concept{concept_node,node} in the pool,
            /// this is either the same value as in the constructor or \c min_node_size if the value was too small.
            std::size_t node_size() const FOONATHAN_NOEXCEPT
            {
                return free_list_.node_size();
            }

            /// \effects Returns the total amount of bytes remaining on the free list.
            /// Divide it by \ref node_size() to get the number of nodes that can be allocated without growing the arena.
            /// \note Array allocations may lead to a growth even if the capacity_left left is big enough.
            std::size_t capacity_left() const FOONATHAN_NOEXCEPT
            {
                return free_list_.capacity() * node_size();
            }

            /// \returns The size of the next memory block after the free list gets empty and the arena grows.
            /// \ref capacity_left() will increase by this amount.
            /// \note Due to fence memory in debug mode this cannot be just divided by the \ref node_size() to get the number of nodes.
            std::size_t next_capacity() const FOONATHAN_NOEXCEPT
            {
                return free_list_.usable_size(arena_.next_block_size());
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
                return {FOONATHAN_MEMORY_LOG_PREFIX "::memory_pool", this};
            }

            void allocate_block()
            {
                auto mem = arena_.allocate_block();
                free_list_.insert(static_cast<char*>(mem.memory), mem.size);
            }

            void* allocate_array(std::size_t n, std::size_t node_size)
            {
                auto mem = free_list_.empty() ? nullptr : free_list_.allocate(n * node_size);
                if (!mem)
                {
                    allocate_block();
                    mem = free_list_.allocate(n * node_size);
                    if (!mem)
                        FOONATHAN_THROW(bad_array_size(info(), n * node_size, capacity_left()));
                }
                return mem;
            }

            void* try_allocate_array(std::size_t n, std::size_t node_size) FOONATHAN_NOEXCEPT
            {
                return !pool_type::value || free_list_.empty() ? nullptr :
                                                                 free_list_.allocate(n * node_size);
            }

            bool try_deallocate_array(void* ptr, std::size_t n,
                                      std::size_t node_size) FOONATHAN_NOEXCEPT
            {
                if (!pool_type::value || !arena_.owns(ptr))
                    return false;
                free_list_.deallocate(ptr, n * node_size);
                return true;
            }

            memory_arena<allocator_type, false> arena_;
            free_list free_list_;

            friend allocator_traits<memory_pool<PoolType, BlockOrRawAllocator>>;
            friend composable_allocator_traits<memory_pool<PoolType, BlockOrRawAllocator>>;
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class memory_pool<node_pool>;
        extern template class memory_pool<array_pool>;
        extern template class memory_pool<small_node_pool>;
#endif

        template <class Type, class Alloc>
        FOONATHAN_CONSTEXPR std::size_t memory_pool<Type, Alloc>::min_node_size;

        /// Specialization of the \ref allocator_traits for \ref memory_pool classes.
        /// \note It is not allowed to mix calls through the specialization and through the member functions,
        /// i.e. \ref memory_pool::allocate_node() and this \c allocate_node().
        /// \ingroup memory allocator
        template <typename PoolType, class ImplRawAllocator>
        class allocator_traits<memory_pool<PoolType, ImplRawAllocator>>
        {
        public:
            using allocator_type = memory_pool<PoolType, ImplRawAllocator>;
            using is_stateful    = std::true_type;

            /// \returns The result of \ref memory_pool::allocate_node().
            /// \throws Anything thrown by the pool allocation function
            /// or a \ref bad_allocation_size exception.
            static void* allocate_node(allocator_type& state, std::size_t size,
                                       std::size_t alignment)
            {
                detail::check_allocation_size<bad_node_size>(size, max_node_size(state),
                                                             state.info());
                detail::check_allocation_size<bad_alignment>(alignment,
                                                             [&] { return max_alignment(state); },
                                                             state.info());
                auto mem = state.allocate_node();
                state.on_allocate(size);
                return mem;
            }

            /// \effects Forwards to \ref memory_pool::allocate_array()
            /// with the number of nodes adjusted to be the minimum,
            /// i.e. when the \c size is less than the \ref memory_pool::node_size().
            /// \returns A \concept{concept_array,array} with specified properties.
            /// \requires The \ref memory_pool has to support array allocations.
            /// \throws Anything thrown by the pool allocation function.
            static void* allocate_array(allocator_type& state, std::size_t count, std::size_t size,
                                        std::size_t alignment)
            {
                detail::check_allocation_size<bad_node_size>(size, max_node_size(state),
                                                             state.info());
                detail::check_allocation_size<bad_alignment>(alignment,
                                                             [&] { return max_alignment(state); },
                                                             state.info());
                detail::check_allocation_size<bad_array_size>(count * size, max_array_size(state),
                                                              state.info());
                auto mem = state.allocate_array(count, size);
                state.on_allocate(count * size);
                return mem;
            }

            /// \effects Just forwards to \ref memory_pool::deallocate_node().
            static void deallocate_node(allocator_type& state, void* node, std::size_t size,
                                        std::size_t) FOONATHAN_NOEXCEPT
            {
                state.deallocate_node(node);
                state.on_deallocate(size);
            }

            /// \effects Forwards to \ref memory_pool::deallocate_array() with the same size adjustment.
            static void deallocate_array(allocator_type& state, void* array, std::size_t count,
                                         std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
            {
                state.free_list_.deallocate(array, count * size);
                state.on_deallocate(count * size);
            }

            /// \returns The maximum size of each node which is \ref memory_pool::node_size().
            static std::size_t max_node_size(const allocator_type& state) FOONATHAN_NOEXCEPT
            {
                return state.node_size();
            }

            /// \returns An upper bound on the maximum array size which is \ref memory_pool::next_capacity().
            static std::size_t max_array_size(const allocator_type& state) FOONATHAN_NOEXCEPT
            {
                return state.next_capacity();
            }

            /// \returns The maximum alignment which is the next bigger power of two if less than \c alignof(std::max_align_t)
            /// or the maximum alignment itself otherwise.
            static std::size_t max_alignment(const allocator_type& state) FOONATHAN_NOEXCEPT
            {
                return state.free_list_.alignment();
            }
        };

        /// Specialization of the \ref composable_allocator_traits for \ref memory_pool classes.
        /// \ingroup memory allocator
        template <typename PoolType, class BlockOrRawAllocator>
        class composable_allocator_traits<memory_pool<PoolType, BlockOrRawAllocator>>
        {
            using traits = allocator_traits<memory_pool<PoolType, BlockOrRawAllocator>>;

        public:
            using allocator_type = memory_pool<PoolType, BlockOrRawAllocator>;

            /// \returns The result of \ref memory_pool::try_allocate_node()
            /// or `nullptr` if the allocation size was too big.
            static void* try_allocate_node(allocator_type& state, std::size_t size,
                                           std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (size > traits::max_node_size(state) || alignment > traits::max_alignment(state))
                    return nullptr;
                return state.try_allocate_node();
            }

            /// \effects Forwards to \ref memory_pool::try_allocate_array()
            /// with the number of nodes adjusted to be the minimum,
            /// if the \c size is less than the \ref memory_pool::node_size().
            /// \returns A \concept{concept_array,array} with specified properties
            /// or `nullptr` if it was unable to allocate.
            static void* try_allocate_array(allocator_type& state, std::size_t count,
                                            std::size_t size,
                                            std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (size > traits::max_node_size(state)
                    || count * size > traits::max_array_size(state)
                    || alignment > traits::max_alignment(state))
                    return nullptr;
                return state.try_allocate_array(count, size);
            }

            /// \effects Just forwards to \ref memory_pool::try_deallocate_node().
            /// \returns Whether the deallocation was successful.
            static bool try_deallocate_node(allocator_type& state, void* node, std::size_t size,
                                            std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (size > traits::max_node_size(state) || alignment > traits::max_alignment(state))
                    return false;
                return state.try_deallocate_node(node);
            }

            /// \effects Forwards to \ref memory_pool::deallocate_array() with the same size adjustment.
            /// \returns Whether the deallocation was successful.
            static bool try_deallocate_array(allocator_type& state, void* array, std::size_t count,
                                             std::size_t size,
                                             std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (size > traits::max_node_size(state)
                    || count * size > traits::max_array_size(state)
                    || alignment > traits::max_alignment(state))
                    return false;
                return state.try_deallocate_array(array, count, size);
            }
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class allocator_traits<memory_pool<node_pool>>;
        extern template class allocator_traits<memory_pool<array_pool>>;
        extern template class allocator_traits<memory_pool<small_node_pool>>;

        extern template class composable_allocator_traits<memory_pool<node_pool>>;
        extern template class composable_allocator_traits<memory_pool<array_pool>>;
        extern template class composable_allocator_traits<memory_pool<small_node_pool>>;
#endif
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_POOL_HPP_INCLUDED
