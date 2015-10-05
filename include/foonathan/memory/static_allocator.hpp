// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_STATIC_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_STATIC_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::static_allocator and \ref foonathan::memory::static_allocator_storage.

#include <type_traits>

#include "detail/align.hpp"
#include "detail/memory_stack.hpp"
#include "config.hpp"

namespace foonathan { namespace memory
{
    /// Storage for a \ref static_allocator.
    /// Its constructor will take a reference to it and use it for its allocation.
    /// The storage type is simply a \c char array aligned for maximum alignment.
    /// \note It is not allowed to access the memory of the storage.
    /// \ingroup memory
    template <std::size_t Size>
    FOONATHAN_ALIAS_TEMPLATE(static_allocator_storage, typename std::aligned_storage<1, detail::max_alignment>::type[Size]);

    /// A stateful \concept{concept_rawallocator,RawAllocator} that uses a fixed sized storage for the allocations.
    /// It works on a \ref static_allocator_storage and uses its memory for all allocations.
    /// Deallocations are not supported, memory cannot be marked as freed.<br>
    /// Its main use is as implementation allocator in memory arenas.
    /// Create a \ref static_allocator_storage of the block size of the arena onto the stack
    /// and pass it to the arena.
    /// This will create an arena of fixed size whose memory is created on the stack and which cannot grow.
    /// It thus allows to have complex containers like \c std::unordered_map located on the program stack.
    /// \note It is not allowed to share an \ref static_allocator_storage between multiple \ref static_allocators.
    /// \ingroup memory
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
        static_allocator(static_allocator_storage<Size> &storage) FOONATHAN_NOEXCEPT
        : stack_(&storage, Size) {}

        /// \effects A \concept{concept_rawallocator,RawAllocator} allocation function.
        /// It uses the specified \ref static_allocator_storage.
        /// \returns A pointer to a \concept{concept_node,node}, it will never be \c nullptr.
        /// \throws An exception of type \ref out_of_memory or whatever is thrown by its handler if the storage is exhausted.
        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            auto mem = stack_.allocate(size, alignment);
            if (!mem)
                FOONATHAN_THROW(out_of_memory(info(), size));
            return mem;
        }

        /// \effects A \concept{concept_rawallocator,RawAllocator} deallocation function.
        /// It does nothing, deallocation is not supported by this allocator.
        void deallocate_node(void *, std::size_t, std::size_t) FOONATHAN_NOEXCEPT {}

        /// \returns The maximum node size which is the capacity remaining inside the \ref static_allocator_storage.
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT
        {
            return stack_.end() - stack_.top();
        }

        /// \returns The maximum possible value since there is no alignment restriction
        /// (except indirectly through the size of the \ref static_allocator_storage).
        std::size_t max_alignment() const FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }

    private:
        allocator_info info() const FOONATHAN_NOEXCEPT
        {
            return {FOONATHAN_MEMORY_LOG_PREFIX "::static_allocator", this};
        }

        detail::fixed_memory_stack stack_;
    };
}} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_STATIC_ALLOCATOR_HPP_INCLUDED
