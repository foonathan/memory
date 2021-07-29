// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_arena and related functionality regarding \concept{concept_blockallocator,BlockAllocators}.

#include <type_traits>

#include "detail/debug_helpers.hpp"
#include "detail/assert.hpp"
#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "config.hpp"
#include "default_allocator.hpp"
#include "error.hpp"

namespace foonathan
{
    namespace memory
    {
        /// A memory block.
        /// It is defined by its starting address and size.
        /// \ingroup core
        struct memory_block
        {
            void*       memory; ///< The address of the memory block (might be \c nullptr).
            std::size_t size;   ///< The size of the memory block (might be \c 0).

            /// \effects Creates an invalid memory block with starting address \c nullptr and size \c 0.
            memory_block() noexcept : memory_block(nullptr, std::size_t(0)) {}

            /// \effects Creates a memory block from a given starting address and size.
            memory_block(void* mem, std::size_t s) noexcept : memory(mem), size(s) {}

            /// \effects Creates a memory block from a [begin,end) range.
            memory_block(void* begin, void* end) noexcept
            : memory_block(begin, static_cast<std::size_t>(static_cast<char*>(end)
                                                           - static_cast<char*>(begin)))
            {
            }

            /// \returns Whether or not a pointer is inside the memory.
            bool contains(const void* address) const noexcept
            {
                auto mem  = static_cast<const char*>(memory);
                auto addr = static_cast<const char*>(address);
                return addr >= mem && addr < mem + size;
            }
        };

        namespace detail
        {
            template <class BlockAllocator>
            std::true_type is_block_allocator_impl(
                int,
                FOONATHAN_SFINAE(std::declval<memory_block&>() =
                                     std::declval<BlockAllocator&>().allocate_block()),
                FOONATHAN_SFINAE(std::declval<std::size_t&>() =
                                     std::declval<BlockAllocator&>().next_block_size()),
                FOONATHAN_SFINAE(std::declval<BlockAllocator>().deallocate_block(memory_block{})));

            template <typename T>
            std::false_type is_block_allocator_impl(short);
        } // namespace detail

        /// Traits that check whether a type models concept \concept{concept_blockallocator,BlockAllocator}.
        /// \ingroup core
        template <typename T>
        struct is_block_allocator : decltype(detail::is_block_allocator_impl<T>(0))
        {
        };

#if !defined(DOXYGEN)
        template <class BlockAllocator, bool Cached = true>
        class memory_arena;
#endif

        /// @{
        /// Controls the caching of \ref memory_arena.
        /// By default, deallocated blocks are put onto a cache, so they can be reused later;
        /// this tag value enable/disable it..<br>
        /// This can be useful, e.g. if there will never be blocks available for deallocation.
        /// The (tiny) overhead for the cache can then be disabled.
        /// An example is \ref memory_pool.
        /// \ingroup core
        constexpr bool cached_arena   = true;
        constexpr bool uncached_arena = false;
        /// @}

        namespace detail
        {
            // stores memory block in an intrusive linked list and allows LIFO access
            class memory_block_stack
            {
            public:
                memory_block_stack() noexcept : head_(nullptr) {}

                ~memory_block_stack() noexcept {}

                memory_block_stack(memory_block_stack&& other) noexcept : head_(other.head_)
                {
                    other.head_ = nullptr;
                }

                memory_block_stack& operator=(memory_block_stack&& other) noexcept
                {
                    memory_block_stack tmp(detail::move(other));
                    swap(*this, tmp);
                    return *this;
                }

                friend void swap(memory_block_stack& a, memory_block_stack& b) noexcept
                {
                    detail::adl_swap(a.head_, b.head_);
                }

                // the raw allocated block returned from an allocator
                using allocated_mb = memory_block;

                // the inserted block slightly smaller to allow for the fixup value
                using inserted_mb = memory_block;

                // how much an inserted block is smaller
                static constexpr std::size_t implementation_offset() noexcept
                {
                    // node size rounded up to the next multiple of max_alignment.
                    return (sizeof(node) / max_alignment + (sizeof(node) % max_alignment != 0))
                           * max_alignment;
                }

                // pushes a memory block
                void push(allocated_mb block) noexcept;

                // pops a memory block and returns the original block
                allocated_mb pop() noexcept;

                // steals the top block from another stack
                void steal_top(memory_block_stack& other) noexcept;

                // returns the last pushed() inserted memory block
                inserted_mb top() const noexcept
                {
                    FOONATHAN_MEMORY_ASSERT(head_);
                    auto mem = static_cast<void*>(head_);
                    return {static_cast<char*>(mem) + implementation_offset(), head_->usable_size};
                }

                bool empty() const noexcept
                {
                    return head_ == nullptr;
                }

                bool owns(const void* ptr) const noexcept;

                // O(n) size
                std::size_t size() const noexcept;

            private:
                struct node
                {
                    node*       prev;
                    std::size_t usable_size;

                    node(node* p, std::size_t size) noexcept : prev(p), usable_size(size) {}
                };

                node* head_;
            };

            template <bool Cached>
            class memory_arena_cache;

            template <>
            class memory_arena_cache<cached_arena>
            {
            protected:
                bool cache_empty() const noexcept
                {
                    return cached_.empty();
                }

                std::size_t cache_size() const noexcept
                {
                    return cached_.size();
                }

                std::size_t cached_block_size() const noexcept
                {
                    return cached_.top().size;
                }

                bool take_from_cache(detail::memory_block_stack& used) noexcept
                {
                    if (cached_.empty())
                        return false;
                    used.steal_top(cached_);
                    return true;
                }

                template <class BlockAllocator>
                void do_deallocate_block(BlockAllocator&, detail::memory_block_stack& used) noexcept
                {
                    cached_.steal_top(used);
                }

                template <class BlockAllocator>
                void do_shrink_to_fit(BlockAllocator& alloc) noexcept
                {
                    detail::memory_block_stack to_dealloc;
                    // pop from cache and push to temporary stack
                    // this revers order
                    while (!cached_.empty())
                        to_dealloc.steal_top(cached_);
                    // now dealloc everything
                    while (!to_dealloc.empty())
                        alloc.deallocate_block(to_dealloc.pop());
                }

            private:
                detail::memory_block_stack cached_;
            };

            template <>
            class memory_arena_cache<uncached_arena>
            {
            protected:
                bool cache_empty() const noexcept
                {
                    return true;
                }

                std::size_t cache_size() const noexcept
                {
                    return 0u;
                }

                std::size_t cached_block_size() const noexcept
                {
                    return 0u;
                }

                bool take_from_cache(detail::memory_block_stack&) noexcept
                {
                    return false;
                }

                template <class BlockAllocator>
                void do_deallocate_block(BlockAllocator&             alloc,
                                         detail::memory_block_stack& used) noexcept
                {
                    alloc.deallocate_block(used.pop());
                }

                template <class BlockAllocator>
                void do_shrink_to_fit(BlockAllocator&) noexcept
                {
                }
            };
        } // namespace detail

        /// A memory arena that manages huge memory blocks for a higher-level allocator.
        /// Some allocators like \ref memory_stack work on huge memory blocks,
        /// this class manages them fro those allocators.
        /// It uses a \concept{concept_blockallocator,BlockAllocator} for the allocation of those blocks.
        /// The memory blocks in use are put onto a stack like structure, deallocation will pop from the top,
        /// so it is only possible to deallocate the last allocated block of the arena.
        /// By default, blocks are not really deallocated but stored in a cache.
        /// This can be disabled with the second template parameter,
        /// passing it \ref uncached_arena (or \c false) disables it,
        /// \ref cached_arena (or \c true) enables it explicitly.
        /// \ingroup core
        template <class BlockAllocator, bool Cached /* = true */>
        class memory_arena : FOONATHAN_EBO(BlockAllocator),
                             FOONATHAN_EBO(detail::memory_arena_cache<Cached>)
        {
            static_assert(is_block_allocator<BlockAllocator>::value,
                          "BlockAllocator is not a BlockAllocator!");
            using cache = detail::memory_arena_cache<Cached>;

        public:
            using allocator_type = BlockAllocator;
            using is_cached      = std::integral_constant<bool, Cached>;

            /// \returns The minimum block size required for an arena containing the given amount of memory.
            /// If an arena is created with the result of `min_block_size(n)`, the resulting capacity will be exactly `n`.
            /// \requires `byte_size` must be a positive number.
            static constexpr std::size_t min_block_size(std::size_t byte_size) noexcept
            {
                return detail::memory_block_stack::implementation_offset() + byte_size;
            }

            /// \effects Creates it by giving it the size and other arguments for the \concept{concept_blockallocator,BlockAllocator}.
            /// It forwards these arguments to its constructor.
            /// \requires \c block_size must be greater than \c min_block_size(0) and other requirements depending on the \concept{concept_blockallocator,BlockAllocator}.
            /// \throws Anything thrown by the constructor of the \c BlockAllocator.
            template <typename... Args>
            explicit memory_arena(std::size_t block_size, Args&&... args)
            : allocator_type(block_size, detail::forward<Args>(args)...)
            {
                FOONATHAN_MEMORY_ASSERT(block_size > min_block_size(0));
            }

            /// \effects Deallocates all memory blocks that where requested back to the \concept{concept_blockallocator,BlockAllocator}.
            ~memory_arena() noexcept
            {
                // clear cache
                shrink_to_fit();
                // now deallocate everything
                while (!used_.empty())
                    allocator_type::deallocate_block(used_.pop());
            }

            /// @{
            /// \effects Moves the arena.
            /// The new arena takes ownership over all the memory blocks from the other arena object,
            /// which is empty after that.
            /// This does not invalidate any memory blocks.
            memory_arena(memory_arena&& other) noexcept
            : allocator_type(detail::move(other)),
              cache(detail::move(other)),
              used_(detail::move(other.used_))
            {
            }

            memory_arena& operator=(memory_arena&& other) noexcept
            {
                memory_arena tmp(detail::move(other));
                swap(*this, tmp);
                return *this;
            }
            /// @}

            /// \effects Swaps to memory arena objects.
            /// This does not invalidate any memory blocks.
            friend void swap(memory_arena& a, memory_arena& b) noexcept
            {
                detail::adl_swap(static_cast<allocator_type&>(a), static_cast<allocator_type&>(b));
                detail::adl_swap(static_cast<cache&>(a), static_cast<cache&>(b));
                detail::adl_swap(a.used_, b.used_);
            }

            /// \effects Allocates a new memory block.
            /// It first uses a cache of previously deallocated blocks, if caching is enabled,
            /// if it is empty, allocates a new one.
            /// \returns The new \ref memory_block.
            /// \throws Anything thrown by the \concept{concept_blockallocator,BlockAllocator} allocation function.
            memory_block allocate_block()
            {
                if (!this->take_from_cache(used_))
                    used_.push(allocator_type::allocate_block());

                auto block = used_.top();
                detail::debug_fill_internal(block.memory, block.size, false);
                return block;
            }

            /// \returns The current memory block.
            /// This is the memory block that will be deallocated by the next call to \ref deallocate_block().
            memory_block current_block() const noexcept
            {
                return used_.top();
            }

            /// \effects Deallocates the current memory block.
            /// The current memory block is the block on top of the stack of blocks.
            /// If caching is enabled, it does not really deallocate it but puts it onto a cache for later use,
            /// use \ref shrink_to_fit() to purge that cache.
            void deallocate_block() noexcept
            {
                auto block = used_.top();
                detail::debug_fill_internal(block.memory, block.size, true);
                this->do_deallocate_block(get_allocator(), used_);
            }

            /// \returns If `ptr` is in memory owned by the arena.
            bool owns(const void* ptr) const noexcept
            {
                return used_.owns(ptr);
            }

            /// \effects Purges the cache of unused memory blocks by returning them.
            /// The memory blocks will be deallocated in reversed order of allocation.
            /// Does nothing if caching is disabled.
            void shrink_to_fit() noexcept
            {
                this->do_shrink_to_fit(get_allocator());
            }

            /// \returns The capacity of the arena, i.e. how many blocks are used and cached.
            std::size_t capacity() const noexcept
            {
                return size() + cache_size();
            }

            /// \returns The size of the cache, i.e. how many blocks can be allocated without allocation.
            std::size_t cache_size() const noexcept
            {
                return cache::cache_size();
            }

            /// \returns The size of the arena, i.e. how many blocks are in use.
            /// It is always smaller or equal to the \ref capacity().
            std::size_t size() const noexcept
            {
                return used_.size();
            }

            /// \returns The size of the next memory block,
            /// i.e. of the next call to \ref allocate_block().
            /// If there are blocks in the cache, returns size of the next one.
            /// Otherwise forwards to the \concept{concept_blockallocator,BlockAllocator} and subtracts an implementation offset.
            std::size_t next_block_size() const noexcept
            {
                return this->cache_empty() ?
                           allocator_type::next_block_size()
                               - detail::memory_block_stack::implementation_offset() :
                           this->cached_block_size();
            }

            /// \returns A reference of the \concept{concept_blockallocator,BlockAllocator} object.
            /// \requires It is undefined behavior to move this allocator out into another object.
            allocator_type& get_allocator() noexcept
            {
                return *this;
            }

        private:
            detail::memory_block_stack used_;
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class memory_arena<static_block_allocator, true>;
        extern template class memory_arena<static_block_allocator, false>;
        extern template class memory_arena<virtual_block_allocator, true>;
        extern template class memory_arena<virtual_block_allocator, false>;
#endif

        /// A \concept{concept_blockallocator,BlockAllocator} that uses a given \concept{concept_rawallocator,RawAllocator} for allocating the blocks.
        /// It calls the \c allocate_array() function with a node of size \c 1 and maximum alignment on the used allocator for the block allocation.
        /// The size of the next memory block will grow by a given factor after each allocation,
        /// allowing an amortized constant allocation time in the higher level allocator.
        /// The factor can be given as rational in the template parameter, default is \c 2.
        /// \ingroup adapter
        template <class RawAllocator = default_allocator, unsigned Num = 2, unsigned Den = 1>
        class growing_block_allocator
        : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
        {
            static_assert(float(Num) / Den >= 1.0, "invalid growth factor");

            using traits = allocator_traits<RawAllocator>;

        public:
            using allocator_type = typename traits::allocator_type;

            /// \effects Creates it by giving it the initial block size, the allocator object and the growth factor.
            /// By default, it uses a default-constructed allocator object and a growth factor of \c 2.
            /// \requires \c block_size must be greater than 0.
            explicit growing_block_allocator(std::size_t    block_size,
                                             allocator_type alloc = allocator_type()) noexcept
            : allocator_type(detail::move(alloc)), block_size_(block_size)
            {
            }

            /// \effects Allocates a new memory block and increases the block size for the next allocation.
            /// \returns The new \ref memory_block.
            /// \throws Anything thrown by the \c allocate_array() function of the \concept{concept_rawallocator,RawAllocator}.
            memory_block allocate_block()
            {
                auto memory =
                    traits::allocate_array(get_allocator(), block_size_, 1, detail::max_alignment);
                memory_block block(memory, block_size_);
                block_size_ = grow_block_size(block_size_);
                return block;
            }

            /// \effects Deallocates a previously allocated memory block.
            /// This does not decrease the block size.
            /// \requires \c block must be previously returned by a call to \ref allocate_block().
            void deallocate_block(memory_block block) noexcept
            {
                traits::deallocate_array(get_allocator(), block.memory, block.size, 1,
                                         detail::max_alignment);
            }

            /// \returns The size of the memory block returned by the next call to \ref allocate_block().
            std::size_t next_block_size() const noexcept
            {
                return block_size_;
            }

            /// \returns A reference to the used \concept{concept_rawallocator,RawAllocator} object.
            allocator_type& get_allocator() noexcept
            {
                return *this;
            }

            /// \returns The growth factor.
            static float growth_factor() noexcept
            {
                static constexpr auto factor = float(Num) / Den;
                return factor;
            }

            static std::size_t grow_block_size(std::size_t block_size) noexcept
            {
                return block_size * Num / Den;
            }

        private:
            std::size_t block_size_;
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class growing_block_allocator<>;
        extern template class memory_arena<growing_block_allocator<>, true>;
        extern template class memory_arena<growing_block_allocator<>, false>;
#endif

        /// A \concept{concept_blockallocator,BlockAllocator} that allows only one block allocation.
        /// It can be used to prevent higher-level allocators from expanding.
        /// The one block allocation is performed through the \c allocate_array() function of the given \concept{concept_rawallocator,RawAllocator}.
        /// \ingroup adapter
        template <class RawAllocator = default_allocator>
        class fixed_block_allocator : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
        {
            using traits = allocator_traits<RawAllocator>;

        public:
            using allocator_type = typename traits::allocator_type;

            /// \effects Creates it by passing it the size of the block and the allocator object.
            /// \requires \c block_size must be greater than 0,
            explicit fixed_block_allocator(std::size_t    block_size,
                                           allocator_type alloc = allocator_type()) noexcept
            : allocator_type(detail::move(alloc)), block_size_(block_size)
            {
            }

            /// \effects Allocates a new memory block or throws an exception if there was already one allocation.
            /// \returns The new \ref memory_block.
            /// \throws Anything thrown by the \c allocate_array() function of the \concept{concept_rawallocator,RawAllocator} or \ref out_of_memory if this is not the first call.
            memory_block allocate_block()
            {
                if (block_size_)
                {
                    auto         mem = traits::allocate_array(get_allocator(), block_size_, 1,
                                                      detail::max_alignment);
                    memory_block block(mem, block_size_);
                    block_size_ = 0u;
                    return block;
                }
                FOONATHAN_THROW(out_of_fixed_memory(info(), block_size_));
            }

            /// \effects Deallocates the previously allocated memory block.
            /// It also resets and allows a new call again.
            void deallocate_block(memory_block block) noexcept
            {
                detail::debug_check_pointer([&] { return block_size_ == 0u; }, info(),
                                            block.memory);
                traits::deallocate_array(get_allocator(), block.memory, block.size, 1,
                                         detail::max_alignment);
                block_size_ = block.size;
            }

            /// \returns The size of the next block which is either the initial size or \c 0.
            std::size_t next_block_size() const noexcept
            {
                return block_size_;
            }

            /// \returns A reference to the used \concept{concept_rawallocator,RawAllocator} object.
            allocator_type& get_allocator() noexcept
            {
                return *this;
            }

        private:
            allocator_info info() noexcept
            {
                return {FOONATHAN_MEMORY_LOG_PREFIX "::fixed_block_allocator", this};
            }

            std::size_t block_size_;
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class fixed_block_allocator<>;
        extern template class memory_arena<fixed_block_allocator<>, true>;
        extern template class memory_arena<fixed_block_allocator<>, false>;
#endif

        namespace detail
        {
            template <class RawAlloc>
            using default_block_wrapper = growing_block_allocator<RawAlloc>;

            template <template <class...> class Wrapper, class BlockAllocator, typename... Args>
            BlockAllocator make_block_allocator(std::true_type, std::size_t block_size,
                                                Args&&... args)
            {
                return BlockAllocator(block_size, detail::forward<Args>(args)...);
            }

            template <template <class...> class Wrapper, class RawAlloc>
            auto make_block_allocator(std::false_type, std::size_t block_size,
                                      RawAlloc alloc = RawAlloc()) -> Wrapper<RawAlloc>
            {
                return Wrapper<RawAlloc>(block_size, detail::move(alloc));
            }
        } // namespace detail

        /// Takes either a \concept{concept_blockallocator,BlockAllocator} or a \concept{concept_rawallocator,RawAllocator}.
        /// In the first case simply aliases the type unchanged, in the second to \ref growing_block_allocator (or the template in `BlockAllocator`) with the \concept{concept_rawallocator,RawAllocator}.
        /// Using this allows passing normal \concept{concept_rawallocator,RawAllocators} as \concept{concept_blockallocator,BlockAllocators}.
        /// \ingroup core
        template <class BlockOrRawAllocator,
                  template <typename...> class BlockAllocator = detail::default_block_wrapper>
        using make_block_allocator_t = FOONATHAN_IMPL_DEFINED(
            typename std::conditional<is_block_allocator<BlockOrRawAllocator>::value,
                                      BlockOrRawAllocator,
                                      BlockAllocator<BlockOrRawAllocator>>::type);

        /// @{
        /// Helper function make a \concept{concept_blockallocator,BlockAllocator}.
        /// \returns A \concept{concept_blockallocator,BlockAllocator} of the given type created with the given arguments.
        /// \requires Same requirements as the constructor.
        /// \ingroup core
        template <class BlockOrRawAllocator, typename... Args>
        make_block_allocator_t<BlockOrRawAllocator> make_block_allocator(std::size_t block_size,
                                                                         Args&&... args)
        {
            return detail::make_block_allocator<
                detail::default_block_wrapper,
                BlockOrRawAllocator>(is_block_allocator<BlockOrRawAllocator>{}, block_size,
                                     detail::forward<Args>(args)...);
        }

        template <template <class...> class BlockAllocator, class BlockOrRawAllocator,
                  typename... Args>
        make_block_allocator_t<BlockOrRawAllocator, BlockAllocator> make_block_allocator(
            std::size_t block_size, Args&&... args)
        {
            return detail::make_block_allocator<
                BlockAllocator, BlockOrRawAllocator>(is_block_allocator<BlockOrRawAllocator>{},
                                                     block_size, detail::forward<Args>(args)...);
        }
        /// @}

        namespace literals
        {
            /// Syntax sugar to express sizes with unit prefixes.
            /// \returns The number of bytes `value` is in the given unit.
            /// \ingroup core
            /// @{
            constexpr std::size_t operator"" _KiB(unsigned long long value) noexcept
            {
                return std::size_t(value * 1024);
            }

            constexpr std::size_t operator"" _KB(unsigned long long value) noexcept
            {
                return std::size_t(value * 1000);
            }

            constexpr std::size_t operator"" _MiB(unsigned long long value) noexcept
            {
                return std::size_t(value * 1024 * 1024);
            }

            constexpr std::size_t operator"" _MB(unsigned long long value) noexcept
            {
                return std::size_t(value * 1000 * 1000);
            }

            constexpr std::size_t operator"" _GiB(unsigned long long value) noexcept
            {
                return std::size_t(value * 1024 * 1024 * 1024);
            }

            constexpr std::size_t operator"" _GB(unsigned long long value) noexcept
            {
                return std::size_t(value * 1000 * 1000 * 1000);
            }
        } // namespace literals
    }     // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
