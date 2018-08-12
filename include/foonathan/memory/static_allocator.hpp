// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_STATIC_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_STATIC_ALLOCATOR_HPP_INCLUDED

/// \file
/// Allocators using a static, fixed-sized storage.

#include <type_traits>

#include "detail/align.hpp"
#include "detail/assert.hpp"
#include "detail/memory_stack.hpp"
#include "detail/utility.hpp"
#include "config.hpp"

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
#include "allocator_traits.hpp"
#endif

namespace foonathan
{
    namespace memory
    {
        /// Storage for a \ref static_allocator.
        /// Its constructor will take a reference to it and use it for its allocation.
        /// The storage type is simply a \c char array aligned for maximum alignment.
        /// \note It is not allowed to access the memory of the storage.
        /// \ingroup memory allocator
        template <std::size_t Size>
        struct static_allocator_storage
        {
#ifdef _MSC_VER
            // note: MSVC ignores alignment of std::aligned_storage
            FOONATHAN_ALIGNAS(8) char storage[Size];
#else
            typename std::aligned_storage<Size, detail::max_alignment>::type storage;
#endif
        };

        static_assert(sizeof(static_allocator_storage<1024>) == 1024, "");
        static_assert(FOONATHAN_ALIGNOF(static_allocator_storage<1024>) == detail::max_alignment,
                      "");

        struct allocator_info;

        /// A stateful \concept{concept_rawallocator,RawAllocator} that uses a fixed sized storage for the allocations.
        /// It works on a \ref static_allocator_storage and uses its memory for all allocations.
        /// Deallocations are not supported, memory cannot be marked as freed.<br>
        /// \note It is not allowed to share an \ref static_allocator_storage between multiple \ref static_allocator objects.
        /// \ingroup memory allocator
        class static_allocator
        {
        public:
            using is_stateful = std::true_type;

            /// \effects Creates it by passing it a \ref static_allocator_storage by reference.
            /// It will take the address of the storage and use its memory for the allocation.
            /// \requires The storage object must live as long as the allocator object.
            /// It must not be shared between multiple allocators,
            /// i.e. the object must not have been passed to a constructor before.
            template <std::size_t Size>
            static_allocator(static_allocator_storage<Size>& storage) FOONATHAN_NOEXCEPT
            : stack_(&storage),
              end_(stack_.top() + Size)
            {
            }

            /// \effects A \concept{concept_rawallocator,RawAllocator} allocation function.
            /// It uses the specified \ref static_allocator_storage.
            /// \returns A pointer to a \concept{concept_node,node}, it will never be \c nullptr.
            /// \throws An exception of type \ref out_of_memory or whatever is thrown by its handler if the storage is exhausted.
            void* allocate_node(std::size_t size, std::size_t alignment);

            /// \effects A \concept{concept_rawallocator,RawAllocator} deallocation function.
            /// It does nothing, deallocation is not supported by this allocator.
            void deallocate_node(void*, std::size_t, std::size_t) FOONATHAN_NOEXCEPT {}

            /// \returns The maximum node size which is the capacity remaining inside the \ref static_allocator_storage.
            std::size_t max_node_size() const FOONATHAN_NOEXCEPT
            {
                return static_cast<std::size_t>(end_ - stack_.top());
            }

            /// \returns The maximum possible value since there is no alignment restriction
            /// (except indirectly through the size of the \ref static_allocator_storage).
            std::size_t max_alignment() const FOONATHAN_NOEXCEPT
            {
                return std::size_t(-1);
            }

        private:
            allocator_info info() const FOONATHAN_NOEXCEPT;

            detail::fixed_memory_stack stack_;
            const char*                end_;
        };

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
        extern template class allocator_traits<static_allocator>;
#endif

        struct memory_block;

        /// A \concept{concept_blockallocator,BlockAllocator} that allocates the blocks from a fixed size storage.
        /// It works on a \ref static_allocator_storage and uses it for all allocations,
        /// deallocations are only allowed in reversed order which is guaranteed by \ref memory_arena.
        /// \note It is not allowed to share an \ref static_allocator_storage between multiple \ref static_allocator objects.
        /// \ingroup memory allocator
        class static_block_allocator
        {
        public:
            /// \effects Creates it by passing it the block size and a \ref static_allocator_storage by reference.
            /// It will take the address of the storage and use it to allocate \c block_size'd blocks.
            /// \requires The storage object must live as long as the allocator object.
            /// It must not be shared between multiple allocators,
            /// i.e. the object must not have been passed to a constructor before.
            /// The size of the \ref static_allocator_storage must be a multiple of the (non-null) block size.
            template <std::size_t Size>
            static_block_allocator(std::size_t                     block_size,
                                   static_allocator_storage<Size>& storage) FOONATHAN_NOEXCEPT
            : cur_(static_cast<char*>(static_cast<void*>(&storage))),
              end_(cur_ + Size),
              block_size_(block_size)
            {
                FOONATHAN_MEMORY_ASSERT(block_size <= Size);
                FOONATHAN_MEMORY_ASSERT(Size % block_size == 0u);
            }

            ~static_block_allocator() FOONATHAN_NOEXCEPT = default;

            /// @{
            /// \effects Moves the block allocator, it transfers ownership over the \ref static_allocator_storage.
            /// This does not invalidate any memory blocks.
            static_block_allocator(static_block_allocator&& other) FOONATHAN_NOEXCEPT
            : cur_(other.cur_),
              end_(other.end_),
              block_size_(other.block_size_)
            {
                other.cur_ = other.end_ = nullptr;
                other.block_size_       = 0;
            }

            static_block_allocator& operator=(static_block_allocator&& other) FOONATHAN_NOEXCEPT
            {
                static_block_allocator tmp(detail::move(other));
                swap(*this, tmp);
                return *this;
            }
            /// @}

            /// \effects Swaps the ownership over the \ref static_allocator_storage.
            /// This does not invalidate any memory blocks.
            friend void swap(static_block_allocator& a,
                             static_block_allocator& b) FOONATHAN_NOEXCEPT
            {
                detail::adl_swap(a.cur_, b.cur_);
                detail::adl_swap(a.end_, b.end_);
                detail::adl_swap(a.block_size_, b.block_size_);
            }

            /// \effects Allocates a new block by returning the \ref next_block_size() bytes.
            /// \returns The new memory block.
            memory_block allocate_block();

            /// \effects Deallocates the last memory block by marking the block as free again.
            /// This block will be returned again by the next call to \ref allocate_block().
            /// \requires \c block must be the current top block of the memory,
            /// this is guaranteed by \ref memory_arena.
            void deallocate_block(memory_block block) FOONATHAN_NOEXCEPT;

            /// \returns The next block size, this is the size passed to the constructor.
            std::size_t next_block_size() const FOONATHAN_NOEXCEPT
            {
                return block_size_;
            }

        private:
            allocator_info info() const FOONATHAN_NOEXCEPT;

            char *      cur_, *end_;
            std::size_t block_size_;
        };
    } // namespace memory
} // namespace foonathan

#endif //FOONATHAN_MEMORY_STATIC_ALLOCATOR_HPP_INCLUDED
