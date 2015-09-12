// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_POOL_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_POOL_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_pool and its \ref foonathan::memory::allocator_traits specialization.

#include <type_traits>

#include "detail/align.hpp"
#include "detail/block_list.hpp"
#include "detail/free_list.hpp"
#include "detail/small_free_list.hpp"
#include "allocator_traits.hpp"
#include "config.hpp"
#include "debugging.hpp"
#include "error.hpp"
#include "default_allocator.hpp"
#include "memory_pool_type.hpp"

namespace foonathan { namespace memory
{
    /// A stateful \concept{concept_rawallocator,RawAllocator} that manages \concept{concept_node,nodes} of fixed size.
    /// It allocates huge memory blocks serving as arena from a given \c RawAllocator defaulting to \ref default_allocator,
    /// subdivides them in small nodes of given size and puts them onto a free list.
    /// Allocation and deallocation simply remove or add nodes from this list and are thus fast.
    /// The way the list is maintained can be controlled via the \c PoolType
    /// which is either \ref node_pool, \ref array_pool or \ref small_node_pool.<br>
    /// This kind of allocator is ideal for fixed size allocations and deallocations in any order,
    /// for example in a node based container like \c std::list.
    /// It is not so good for different allocation sizes and has some drawbacks for arrays
    /// as described in \ref memory_pool_type.hpp.
    /// \ingroup memory
    template <typename PoolType = node_pool, class RawAllocator = default_allocator>
    class memory_pool
    : FOONATHAN_EBO(detail::leak_checker<memory_pool<node_pool, default_allocator>>)
    {
        using free_list = typename PoolType::type;
        using leak_checker = detail::leak_checker<memory_pool<node_pool, default_allocator>>;

    public:
        using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;
        using pool_type = PoolType;

        static FOONATHAN_CONSTEXPR std::size_t min_node_size = FOONATHAN_IMPL_DEFINED(free_list::min_element_size);

        /// \effects Creates it by specifying the size each \concept{concept_node,node} will have,
        /// the initial block size for the arena and the implementation allocator.
        /// If the \c node_size is less than the \c min_node_size, the \c min_node_size will be the actual node size.
        /// It will allocate an initial memory block with given size from the implementation allocator
        /// and puts it onto the free list.
        /// \requires \c node_size must be a valid \concept{concept_node,node size}
        /// and \c block_size must be a non-zero value.
        memory_pool(std::size_t node_size, std::size_t block_size,
                    allocator_type allocator = allocator_type())
        : leak_checker(info().name),
          block_list_(block_size, detail::move(allocator)),
          free_list_(node_size)
        {
            allocate_block();
        }

        /// \effects Destroys the \ref memory_pool by returning all memory blocks,
        /// regardless of properly deallocated back to the implementation allocator.
        ~memory_pool() FOONATHAN_NOEXCEPT = default;

        /// @{
        /// \effects Moving a \ref memory_pool object transfers ownership over the free list,
        /// i.e. the moved from pool is completely empty and the new one has all its memory.
        /// That means that it is not allowed to call \ref deallocate_node() on a moved-from allocator
        /// even when passing it memory that was previously allocated by this object.
        memory_pool(memory_pool &&other) FOONATHAN_NOEXCEPT
        : detail::leak_checker<memory_pool<node_pool, default_allocator>>(detail::move(other)),
          block_list_(detail::move(other.block_list_)),
          free_list_(detail::move(other.free_list_)){}

        memory_pool& operator=(memory_pool &&other) FOONATHAN_NOEXCEPT
        {
            detail::leak_checker<memory_pool<node_pool, default_allocator>>::operator=(detail::move(other));
            block_list_ = std::move(other.block_list_);
            block_list_ = std::move(other.block_list_);
            free_list_ = std::move(other.free_list_);
        }
        /// @}

        /// \effects Allocates a single \concept{concept_node,node} by removing it from the free list.
        /// If the free list is empty, a new memory block will be allocated and put onto it.
        /// The new block size will be \ref next_capacity() big.
        /// \returns A node of size \ref node_size() suitable aligned,
        /// i.e. suitable for any type where <tt>sizeof(T) < node_size()</tt>.
        /// \throws Anything thrown by the used implementation allocator's allocation function if a growth is needed.
        void* allocate_node()
        {
            if (free_list_.empty())
                allocate_block();
            FOONATHAN_MEMORY_ASSERT(!free_list_.empty());
            return free_list_.allocate();
        }

        /// \effects Allocates an \concept{concept_array,array} of nodes by searching for \c n continuous nodes on the list and removing them.
        /// Depending on the \c PoolType this can be a slow operation or not allowed at all.
        /// This can sometimes lead to a growth, even if technically there is enough continuous memory on the free list.
        /// \returns An array of \c n nodes of size \ref node_size() suitable aligned.
        /// \throws Anything thrown by the used implementation allocator's allocation function if a growth is needed,
        /// or \ref bad_allocation_size if <tt>n * node_size()</tt> is too big.
        /// \requires The \c PoolType must support array allocations, otherwise the body of this function will not compile.
        /// \c n must be valid \concept{concept_array,array count}.
        void* allocate_array(std::size_t n)
        {
            static_assert(pool_type::value,
                        "does not support array allocations");
            return allocate_array(n, node_size());
        }

        /// \effects Deallocates a single \concept{concept_node,node} by putting it back onto the free list.
        /// \requires \c ptr must be a result from a previous call to \ref allocate_node() on the same free list,
        /// i.e. either this allocator object or a new object created by moving this to it.
        void deallocate_node(void *ptr) FOONATHAN_NOEXCEPT
        {
            free_list_.deallocate(ptr);
        }

        /// \effects Deallocates an \concept{concept_array,array} by putting it back onto the free list.
        /// \requires \c ptr must be a result from a previous call to \ref allocate_array() with the same \c n on the same free list,
        /// i.e. either this allocator object or a new object created by moving this to it.
        void deallocate_array(void *ptr, std::size_t n) FOONATHAN_NOEXCEPT
        {
            static_assert(pool_type::value,
                        "does not support array allocations");
            deallocate_array(ptr, n, node_size());
        }

        /// \returns The size of each \concept{concept_node,node} in the pool,
        /// this is either the same value as in the constructor or \c min_node_size if the value was too small.
        std::size_t node_size() const FOONATHAN_NOEXCEPT
        {
            return free_list_.node_size();
        }

        /// \effects Returns the total amount of byts remaining on the free list.
        /// Divide it by \ref node_size() to get the number of nodes that can be allocated without growing the arena.
        /// \note Array allocations may lead to a growth even if the capacity is big enough.
        std::size_t capacity() const FOONATHAN_NOEXCEPT
        {
            return free_list_.capacity() * node_size();
        }

        /// \returns The size of the next memory block after the free list gets empty and the arena grows.
        /// \note Due to fence memory, alignment buffers and the like this may not be the exact result \ref capacity() will return,
        /// but it is an upper bound to it.
        std::size_t next_capacity() const FOONATHAN_NOEXCEPT
        {
            return block_list_.next_block_size();
        }

        /// \returns A reference to the implementation allocator used for managing the arena.
        /// \requires It is undefined behavior to move this allocator out into another object.
        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return block_list_.get_allocator();
        }

    private:
        allocator_info info() const FOONATHAN_NOEXCEPT
        {
            return {FOONATHAN_MEMORY_LOG_PREFIX "::memory_pool", this};
        }

        void allocate_block()
        {
            auto mem = block_list_.allocate();
            auto offset = detail::align_offset(mem.memory, detail::max_alignment);
            detail::debug_fill(mem.memory, offset, debug_magic::alignment_memory);
            free_list_.insert(static_cast<char*>(mem.memory) + offset, mem.size - offset);
        }

        void* allocate_array(std::size_t n, std::size_t node_size)
        {
            detail::check_allocation_size(n * node_size, next_capacity(),
                                          info());

            auto mem = free_list_.empty() ? nullptr
                                          : free_list_.allocate(n * node_size);
            if (!mem)
            {
                allocate_block();
                mem = free_list_.allocate(n * node_size);
                if (!mem)
                    FOONATHAN_THROW(bad_allocation_size(info(),
                                                        n * node_size, capacity()));
            }
            return mem;
        }

        void deallocate_array(void *ptr, std::size_t n, std::size_t node_size) FOONATHAN_NOEXCEPT
        {
            free_list_.deallocate(ptr, n * node_size);
        }

        detail::block_list<allocator_type> block_list_;
        free_list free_list_;

        friend allocator_traits<memory_pool<PoolType, RawAllocator>>;
    };

    template <class Type, class Alloc>
    FOONATHAN_CONSTEXPR std::size_t memory_pool<Type, Alloc>::min_node_size;

    /// Specialization of the \ref allocator_traits for \ref memory_pool classes.
    /// \note It is not allowed to mix calls through the specialization and through the member functions,
    /// i.e. \ref memory_pool::allocate_node() and this \c allocate_node().
    /// \ingroup memory
    template <typename PoolType, class ImplRawAllocator>
    class allocator_traits<memory_pool<PoolType, ImplRawAllocator>>
    {
    public:
        using allocator_type = memory_pool<PoolType, ImplRawAllocator>;
        using is_stateful = std::true_type;

        /// \returns The result of \ref memory_pool::allocate_node().
        /// \throws Anything thrown by the pool allocation function
        /// or \ref bad_allocation_size if \c size / \c alignment exceeds \ref max_node_size() / \ref max_alignment().
        static void* allocate_node(allocator_type &state,
                                std::size_t size, std::size_t alignment)
        {
            detail::check_allocation_size(size, max_node_size(state), state.info());
            detail::check_allocation_size(alignment, max_alignment(state), state.info());
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
        static void* allocate_array(allocator_type &state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            detail::check_allocation_size(size, max_node_size(state), state.info());
            detail::check_allocation_size(alignment, max_alignment(state), state.info());
            // array size already checked
            return allocate_array(PoolType{}, state, count, size);
        }

        /// \effects Just forwards to \ref memory_pool::deallocate_node().
        static void deallocate_node(allocator_type &state,
                    void *node, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            state.deallocate_node(node);
            state.on_deallocate(size);
        }

        /// \effects Forwards to \ref memory_pool::deallocate_array() with the same size adjustment.
        static void deallocate_array(allocator_type &state,
                    void *array, std::size_t count, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            deallocate_array(PoolType{}, state, array, count , size);
        }

        /// \returns The maximum size of each node which is \ref memory_pool::node_size().
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.node_size();
        }

        /// \returns An upper bound on the maximum array size which is \ref memory_pool::next_capacity().
        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.next_capacity();
        }

        /// \returns The maximum alignment which is the next bigger power of two if less than \c alignof(std::max_align_t)
        /// or the maximum alignment itself otherwise.
        static std::size_t max_alignment(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return state.free_list_.alignment();
        }

    private:
        static void* allocate_array(std::false_type, allocator_type &,
                                    std::size_t, std::size_t)
        {
            FOONATHAN_MEMORY_UNREACHABLE("array allocations not supported");
            return nullptr;
        }

        static void* allocate_array(std::true_type, allocator_type &state,
                                    std::size_t count, std::size_t size)
        {
            auto mem = state.allocate_array(count, size);
            state.on_allocate(count * size);
            return mem;
        }

        static void deallocate_array(std::false_type, allocator_type &,
                                void *, std::size_t, std::size_t)
        {
            FOONATHAN_MEMORY_UNREACHABLE("array allocations not supported");
        }

        static void deallocate_array(std::true_type, allocator_type &state,
                                void *array, std::size_t count, std::size_t size)
        {
            state.deallocate_array(array, count, size);
            state.on_deallocate(count * size);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_POOL_HPP_INCLUDED
