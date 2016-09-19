// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_POOL_COLLECTION_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_POOL_COLLECTION_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_pool_collection and related classes.

#include <type_traits>

#include "detail/align.hpp"
#include "detail/assert.hpp"
#include "detail/memory_stack.hpp"
#include "detail/free_list_array.hpp"
#include "config.hpp"
#include "debugging.hpp"
#include "error.hpp"
#include "memory_arena.hpp"
#include "memory_pool_type.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            struct memory_pool_collection_leak_handler
            {
                void operator()(std::ptrdiff_t amount);
            };
        } // namespace detail

        /// A \c BucketDistribution for \ref memory_pool_collection defining that there is a bucket, i.e. pool, for each size.
        /// That means that for each possible size up to an upper bound there will be a seperate free list.
        /// Allocating a node will not waste any memory.
        /// \ingroup memory allocator
        struct identity_buckets
        {
            using type = detail::identity_access_policy;
        };

        /// A \c BucketDistribution for \ref memory_pool_collection defining that there is a bucket, i.e. pool, for each power of two.
        /// That means for each power of two up to an upper bound there will be a separate free list.
        /// Allocating a node will only waste half of the memory.
        /// \ingroup memory allocator
        struct log2_buckets
        {
            using type = detail::log2_access_policy;
        };

        /// A stateful \concept{concept_rawallocator,RawAllocator} that behaves as a collection of multiple \ref memory_pool objects.
        /// It maintains a list of multiple free lists, whose types are controlled via the \c PoolType tags defined in \ref memory_pool_type.hpp,
        /// each of a different size as defined in the \c BucketDistribution (\ref identity_buckets or \ref log2_buckets).
        /// Allocating a node of given size will use the appropriate free list.<br>
        /// This allocator is ideal for \concept{concept_node,node} allocations in any order but with a predefined set of sizes,
        /// not only one size like \ref memory_pool.
        /// \ingroup memory allocator
        template <class PoolType, class BucketDistribution,
                  class BlockOrRawAllocator = default_allocator>
        class memory_pool_collection
            : FOONATHAN_EBO(
                  detail::default_leak_checker<detail::memory_pool_collection_leak_handler>)
        {
            using free_list_array =
                detail::free_list_array<typename PoolType::type, typename BucketDistribution::type>;
            using leak_checker =
                detail::default_leak_checker<detail::memory_pool_collection_leak_handler>;

        public:
            using allocator_type      = make_block_allocator_t<BlockOrRawAllocator>;
            using pool_type           = PoolType;
            using bucket_distribution = BucketDistribution;

            /// \effects Creates it by giving it the maximum node size it should be able to allocate,
            /// the size of the initial memory block and other constructor arguments for the \concept{concept_blockallocator,BlockAllocator}.
            /// The \c BucketDistribution controls how many free lists are created,
            /// but unlike in \ref memory_pool all free lists are initially empty and the first memory block queued.
            /// \requires \c max_node_size must be a valid \concept{concept_node,node} size
            /// and \c block_size must be non-zero.
            template <typename... Args>
            memory_pool_collection(std::size_t max_node_size, std::size_t block_size,
                                   Args&&... args)
            : arena_(block_size, detail::forward<Args>(args)...),
              stack_(allocate_block()),
              pools_(stack_, block_end(), max_node_size)
            {
            }

            /// \effects Destroys the \ref memory_pool_collection by returning all memory blocks,
            /// regardless of properly deallocated back to the \concept{concept_blockallocator,BlockAllocator}.
            ~memory_pool_collection() FOONATHAN_NOEXCEPT = default;

            /// @{
            /// \effects Moving a \ref memory_pool_collection object transfers ownership over the free lists,
            /// i.e. the moved from pool is completely empty and the new one has all its memory.
            /// That means that it is not allowed to call \ref deallocate_node() on a moved-from allocator
            /// even when passing it memory that was previously allocated by this object.
            memory_pool_collection(memory_pool_collection&& other) FOONATHAN_NOEXCEPT
                : leak_checker(detail::move(other)),
                  arena_(detail::move(other.arena_)),
                  stack_(detail::move(other.stack_)),
                  pools_(detail::move(other.pools_))
            {
            }

            memory_pool_collection& operator=(memory_pool_collection&& other) FOONATHAN_NOEXCEPT
            {
                leak_checker::operator=(detail::move(other));
                arena_                = detail::move(other.arena_);
                stack_                = detail::move(other.stack_);
                pools_                = detail::move(other.pools_);
                return *this;
            }
            /// @}

            /// \effects Allocates a \concept{concept_node,node} of given size.
            /// It first finds the appropriate free list as defined in the \c BucketDistribution.
            /// If it is empty, it will use an implementation defined amount of memory from the arena
            /// and inserts it in it.
            /// If the arena is empty too, it will request a new memory block from the \concept{concept_blockallocator,BlockAllocator}
            /// of size \ref next_capacity() and puts part of it onto this free list.
            /// Then it removes a node from it.
            /// \returns A \concept{concept_node,node} of given size suitable aligned,
            /// i.e. suitable for any type where <tt>sizeof(T) < node_size</tt>.
            /// \throws Anything thrown by the \concept{concept_blockallocator,BlockAllocator} if a growth is needed or a \ref bad_node_size exception if the node size is too big.
            void* allocate_node(std::size_t node_size)
            {
                detail::check_allocation_size<bad_node_size>(node_size,
                                                             [&] { return max_node_size(); },
                                                             info());
                auto& pool = pools_.get(node_size);
                if (pool.empty())
                {
                    auto block = reserve_memory(pool, def_capacity());
                    pool.insert(block.memory, block.size);
                }

                auto mem = pool.allocate();
                FOONATHAN_MEMORY_ASSERT(mem);
                return mem;
            }

            /// \effects Allocates a \concept{concept_node,node} of given size.
            /// It is similar to \ref allocate_node() but will return `nullptr` on any failure,
            /// instead of growing the arnea and possibly throwing.
            /// \returns A \concept{concept_node,node| of given size suitable aligned
            /// or `nullptr` in case of failure.
            void* try_allocate_node(std::size_t node_size) FOONATHAN_NOEXCEPT
            {
                if (node_size > max_node_size())
                    return nullptr;
                auto& pool = pools_.get(node_size);
                if (pool.empty())
                {
                    try_reserve_memory(pool, def_capacity());
                    return pool.empty() ? nullptr : pool.allocate();
                }
                else
                    return pool.allocate();
            }

            /// \effects Allocates an \concept{concept_array,array} of nodes by searching for \c n continuous nodes on the appropriate free list and removing them.
            /// Depending on the \c PoolType this can be a slow operation or not allowed at all.
            /// This can sometimes lead to a growth on the free list, even if technically there is enough continuous memory on the free list.
            /// Otherwise has the same behavior as \ref allocate_node().
            /// \returns An array of \c n nodes of size \c node_size suitable aligned.
            /// \throws Anything thrown by the used \concept{concept_blockallocator,BlockAllocator}'s allocation function if a growth is needed,
            /// or a \ref bad_allocation_size exception.
            /// \requires \c count must be valid \concept{concept_array,array count} and
            /// \c node_size must be valid \concept{concept_node,node size}.
            void* allocate_array(std::size_t count, std::size_t node_size)
            {
                detail::check_allocation_size<bad_node_size>(node_size,
                                                             [&] { return max_node_size(); },
                                                             info());

                auto& pool = pools_.get(node_size);

                // try allocating if not empty
                // for pools without array allocation support, allocate() will always return nullptr
                auto mem = pool.empty() ? nullptr : pool.allocate(count * node_size);
                if (!mem)
                {
                    // use stack for allocation
                    detail::check_allocation_size<bad_array_size>(count * node_size,
                                                                  [&] {
                                                                      return next_capacity()
                                                                             - pool.alignment() + 1;
                                                                  },
                                                                  info());
                    mem = reserve_memory(pool, count * node_size).memory;
                    FOONATHAN_MEMORY_ASSERT(mem);
                }

                return mem;
            }

            /// \effects Allocates a \concept{concept_array,array} of given size.
            /// It is similar to \ref allocate_node() but will return `nullptr` on any failure,
            /// instead of growing the arnea and possibly throwing.
            /// \returns A \concept{concept_array,array| of given size suitable aligned
            /// or `nullptr` in case of failure.
            void* try_allocate_array(std::size_t count, std::size_t node_size) FOONATHAN_NOEXCEPT
            {
                if (!pool_type::value || node_size > max_node_size())
                    return nullptr;
                auto& pool = pools_.get(node_size);
                if (pool.empty())
                {
                    try_reserve_memory(pool, def_capacity());
                    return pool.empty() ? nullptr : pool.allocate(count * node_size);
                }
                else
                    return pool.allocate(count * node_size);
            }

            /// \effects Deallocates a \concept{concept_node,node} by putting it back onto the appropriate free list.
            /// \requires \c ptr must be a result from a previous call to \ref allocate_node() with the same size on the same free list,
            /// i.e. either this allocator object or a new object created by moving this to it.
            void deallocate_node(void* ptr, std::size_t node_size) FOONATHAN_NOEXCEPT
            {
                pools_.get(node_size).deallocate(ptr);
            }

            /// \effects Deallocates a \concept{concept_node,node} similar to \ref deallocate_node().
            /// But it checks if it can deallocate this memory.
            /// \returns `true` if the node could be deallocated,
            /// `false` otherwise.
            bool try_deallocate_node(void* ptr, std::size_t node_size) FOONATHAN_NOEXCEPT
            {
                if (node_size > max_node_size() || !arena_.owns(ptr))
                    return false;
                pools_.get(node_size).deallocate(ptr);
                return true;
            }

            /// \effects Deallocates an \concept{concept_array,array} by putting it back onto the free list.
            /// \requires \c ptr must be a result from a previous call to \ref allocate_array() with the same sizes on the same free list,
            /// i.e. either this allocator object or a new object created by moving this to it.
            void deallocate_array(void* ptr, std::size_t count,
                                  std::size_t node_size) FOONATHAN_NOEXCEPT
            {
                pools_.get(node_size).deallocate(ptr, count * node_size);
            }

            /// \effects Deallocates a \concept{concept_array,array} similar to \ref deallocate_array().
            /// But it checks if it can deallocate this memory.
            /// \returns `true` if the array could be deallocated,
            /// `false` otherwise.
            bool try_deallocate_array(void* ptr, std::size_t count,
                                      std::size_t node_size) FOONATHAN_NOEXCEPT
            {
                if (!pool_type::value || node_size > max_node_size() || !arena_.owns(ptr))
                    return false;
                pools_.get(node_size).deallocate(ptr, count * node_size);
                return true;
            }

            /// \effects Inserts more memory on the free list for nodes of given size.
            /// It will try to put \c capacity_left bytes from the arena onto the free list defined over the \c BucketDistribution,
            /// if the arena is empty, a new memory block is requested from the \concept{concept_blockallocator,BlockAllocator}
            /// and it will be used.
            /// \throws Anything thrown by the \concept{concept_blockallocator,BlockAllocator} if a growth is needed.
            /// \requires \c node_size must be valid \concept{concept_node,node size} less than or equal to \ref max_node_size(),
            /// \c capacity_left must be less than \ref next_capacity().
            void reserve(std::size_t node_size, std::size_t capacity)
            {
                FOONATHAN_MEMORY_ASSERT_MSG(node_size <= max_node_size(), "node_size too big");
                auto& pool = pools_.get(node_size);
                reserve_memory(pool, capacity);
            }

            /// \returns The maximum node size for which is a free list.
            /// This is the value passed to it in the constructor.
            std::size_t max_node_size() const FOONATHAN_NOEXCEPT
            {
                return pools_.max_node_size();
            }

            /// \returns The amount of nodes available in the free list for nodes of given size
            /// as defined over the \c BucketDistribution.
            /// This is the number of nodes that can be allocated without the free list requesting more memory from the arena.
            /// \note Array allocations may lead to a growth even if the capacity_left is big enough.
            std::size_t pool_capacity_left(std::size_t node_size) const FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_ASSERT_MSG(node_size <= max_node_size(), "node_size too big");
                return pools_.get(node_size).capacity();
            }

            /// \returns The amount of memory available in the arena not inside the free lists.
            /// This is the number of bytes that can be inserted into the free lists
            /// without requesting more memory from the \concept{concept_blockallocator,BlockAllocator}.
            /// \note Array allocations may lead to a growth even if the capacity is big enough.
            std::size_t capacity_left() const FOONATHAN_NOEXCEPT
            {
                return std::size_t(block_end() - stack_.top());
            }

            /// \returns The size of the next memory block after \ref capacity_left() arena grows.
            /// This is the amount of memory that can be distributed in the pools.
            /// \note If the `PoolType` is \ref small_node_pool, the exact usable memory is lower than that.
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
                return {FOONATHAN_MEMORY_LOG_PREFIX "::memory_pool_collection", this};
            }

            std::size_t def_capacity() const FOONATHAN_NOEXCEPT
            {
                return arena_.next_block_size() / pools_.size();
            }

            detail::fixed_memory_stack allocate_block()
            {
                return detail::fixed_memory_stack(arena_.allocate_block().memory);
            }

            const char* block_end() const FOONATHAN_NOEXCEPT
            {
                auto block = arena_.current_block();
                return static_cast<const char*>(block.memory) + block.size;
            }

            bool insert_rest(typename pool_type::type& pool) FOONATHAN_NOEXCEPT
            {
                if (auto remaining = std::size_t(block_end() - stack_.top()))
                {
                    auto offset = detail::align_offset(stack_.top(), detail::max_alignment);
                    if (offset < remaining)
                    {
                        detail::debug_fill(stack_.top(), offset, debug_magic::alignment_memory);
                        pool.insert(stack_.top() + offset, remaining - offset);
                        return true;
                    }
                }

                return false;
            }

            void try_reserve_memory(typename pool_type::type& pool,
                                    std::size_t               capacity) FOONATHAN_NOEXCEPT
            {
                auto mem = stack_.allocate(block_end(), capacity, detail::max_alignment);
                if (!mem)
                    insert_rest(pool);
                else
                    pool.insert(mem, capacity);
            }

            memory_block reserve_memory(typename pool_type::type& pool, std::size_t capacity)
            {
                auto mem = stack_.allocate(block_end(), capacity, detail::max_alignment);
                if (!mem)
                {
                    insert_rest(pool);
                    // get new block
                    stack_ = allocate_block();

                    // allocate ensuring alignment
                    mem = stack_.allocate(block_end(), capacity, detail::max_alignment);
                    FOONATHAN_MEMORY_ASSERT(mem);
                }
                return {mem, capacity};
            }

            memory_arena<allocator_type, false> arena_;
            detail::fixed_memory_stack stack_;
            free_list_array            pools_;

            friend allocator_traits<memory_pool_collection>;
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class memory_pool_collection<node_pool, identity_buckets>;
        extern template class memory_pool_collection<array_pool, identity_buckets>;
        extern template class memory_pool_collection<small_node_pool, identity_buckets>;

        extern template class memory_pool_collection<node_pool, log2_buckets>;
        extern template class memory_pool_collection<array_pool, log2_buckets>;
        extern template class memory_pool_collection<small_node_pool, log2_buckets>;
#endif

        /// An alias for \ref memory_pool_collection using the \ref identity_buckets policy
        /// and a \c PoolType defaulting to \ref node_pool.
        /// \ingroup memory allocator
        template <class PoolType = node_pool, class ImplAllocator = default_allocator>
        FOONATHAN_ALIAS_TEMPLATE(bucket_allocator,
                                 memory_pool_collection<PoolType, identity_buckets, ImplAllocator>);

        template <class Allocator>
        class allocator_traits;

        /// Specialization of the \ref allocator_traits for \ref memory_pool_collection classes.
        /// \note It is not allowed to mix calls through the specialization and through the member functions,
        /// i.e. \ref memory_pool_collection::allocate_node() and this \c allocate_node().
        /// \ingroup memory allocator
        template <class Pool, class BucketDist, class RawAllocator>
        class allocator_traits<memory_pool_collection<Pool, BucketDist, RawAllocator>>
        {
        public:
            using allocator_type = memory_pool_collection<Pool, BucketDist, RawAllocator>;
            using is_stateful    = std::true_type;

            /// \returns The result of \ref memory_pool_collection::allocate_node().
            /// \throws Anything thrown by the pool allocation function
            /// or a \ref bad_allocation_size exception if \c size / \c alignment exceeds \ref max_node_size() / the suitable alignment value,
            /// i.e. the node is over-aligned.
            static void* allocate_node(allocator_type& state, std::size_t size,
                                       std::size_t alignment)
            {
                // node already checked
                detail::check_allocation_size<bad_alignment>(alignment,
                                                             [&] {
                                                                 return detail::alignment_for(size);
                                                             },
                                                             state.info());
                auto mem = state.allocate_node(size);
                state.on_allocate(size);
                return mem;
            }

            /// \returns The result of \ref memory_pool_collection::allocate_array().
            /// \throws Anything thrown by the pool allocation function or a \ref bad_allocation_size exception.
            /// \requires The \ref memory_pool_collection has to support array allocations.
            static void* allocate_array(allocator_type& state, std::size_t count, std::size_t size,
                                        std::size_t alignment)
            {
                // node and array already checked
                detail::check_allocation_size<bad_alignment>(alignment,
                                                             [&] {
                                                                 return detail::alignment_for(size);
                                                             },
                                                             state.info());
                auto mem = state.allocate_array(count, size);
                state.on_allocate(count * size);
                return mem;
            }

            /// \effects Calls \ref memory_pool_collection::deallocate_node().
            static void deallocate_node(allocator_type& state, void* node, std::size_t size,
                                        std::size_t) FOONATHAN_NOEXCEPT
            {
                state.deallocate_node(node, size);
                state.on_deallocate(size);
            }

            /// \effects Calls \ref memory_pool_collection::deallocate_array().
            /// \requires The \ref memory_pool_collection has to support array allocations.
            static void deallocate_array(allocator_type& state, void* array, std::size_t count,
                                         std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
            {
                state.deallocate_array(array, count, size);
                state.on_deallocate(count * size);
            }

            /// \returns The maximum size of each node which is \ref memory_pool_collection::max_node_size().
            static std::size_t max_node_size(const allocator_type& state) FOONATHAN_NOEXCEPT
            {
                return state.max_node_size();
            }

            /// \returns An upper bound on the maximum array size which is \ref memory_pool::next_capacity().
            static std::size_t max_array_size(const allocator_type& state) FOONATHAN_NOEXCEPT
            {
                return state.next_capacity();
            }

            /// \returns Just \c alignof(std::max_align_t) since the actual maximum alignment depends on the node size,
            /// the nodes must not be over-aligned.
            static std::size_t max_alignment(const allocator_type&) FOONATHAN_NOEXCEPT
            {
                return detail::max_alignment;
            }
        };

        /// Specialization of the \ref composable_allocator_traits for \ref memory_pool_collection classes.
        /// \ingroup memory allocator
        template <class Pool, class BucketDist, class RawAllocator>
        class composable_allocator_traits<memory_pool_collection<Pool, BucketDist, RawAllocator>>
        {
            using traits = allocator_traits<memory_pool_collection<Pool, BucketDist, RawAllocator>>;

        public:
            using allocator_type = memory_pool_collection<Pool, BucketDist, RawAllocator>;

            /// \returns The result of \ref memory_pool_collection::try_allocate_node()
            /// or `nullptr` if the allocation size was too big.
            static void* try_allocate_node(allocator_type& state, std::size_t size,
                                           std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (alignment > traits::max_alignment(state))
                    return nullptr;
                return state.try_allocate_node(size);
            }

            /// \returns The result of \ref memory_pool_collection::try_allocate_array()
            /// or `nullptr` if the allocation size was too big.
            static void* try_allocate_array(allocator_type& state, std::size_t count,
                                            std::size_t size,
                                            std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (count * size > traits::max_array_size(state)
                    || alignment > traits::max_alignment(state))
                    return nullptr;
                return state.try_allocate_array(count, size);
            }

            /// \effects Just forwards to \ref memory_pool_collection::try_deallocate_node().
            /// \returns Whether the deallocation was successful.
            static bool try_deallocate_node(allocator_type& state, void* node, std::size_t size,
                                            std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (alignment > traits::max_alignment(state))
                    return false;
                return state.try_deallocate_node(node, size);
            }

            /// \effects Forwards to \ref memory_pool_collection::deallocate_array().
            /// \returns Whether the deallocation was successful.
            static bool try_deallocate_array(allocator_type& state, void* array, std::size_t count,
                                             std::size_t size,
                                             std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (count * size > traits::max_array_size(state)
                    || alignment > traits::max_alignment(state))
                    return false;
                return state.try_deallocate_array(array, count, size);
            }
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class allocator_traits<memory_pool_collection<node_pool, identity_buckets>>;
        extern template class allocator_traits<memory_pool_collection<array_pool,
                                                                      identity_buckets>>;
        extern template class allocator_traits<memory_pool_collection<small_node_pool,
                                                                      identity_buckets>>;

        extern template class allocator_traits<memory_pool_collection<node_pool, log2_buckets>>;
        extern template class allocator_traits<memory_pool_collection<array_pool, log2_buckets>>;
        extern template class allocator_traits<memory_pool_collection<small_node_pool,
                                                                      log2_buckets>>;

        extern template class composable_allocator_traits<memory_pool_collection<node_pool,
                                                                                 identity_buckets>>;
        extern template class composable_allocator_traits<memory_pool_collection<array_pool,
                                                                                 identity_buckets>>;
        extern template class composable_allocator_traits<memory_pool_collection<small_node_pool,
                                                                                 identity_buckets>>;

        extern template class composable_allocator_traits<memory_pool_collection<node_pool,
                                                                                 log2_buckets>>;
        extern template class composable_allocator_traits<memory_pool_collection<array_pool,
                                                                                 log2_buckets>>;
        extern template class composable_allocator_traits<memory_pool_collection<small_node_pool,
                                                                                 log2_buckets>>;
#endif
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_POOL_COLLECTION_HPP_INCLUDED
