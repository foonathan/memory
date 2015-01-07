#ifndef FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A heap allocator.

#include <type_traits>

namespace foonathan { namespace memory
{
    /// \brief A \ref concept::RawAllocator that allocates memory from the heap.
    ///
    /// It uses system calls directly when possible and thus performs no caching at all. <br>
    /// It is no singleton but stateless; each instance is the same.
    /// \ingroup memory
    class heap_allocator
    {
    public:
        using stateful = std::false_type;
        
        heap_allocator() = default;
        heap_allocator(const heap_allocator&) = delete;
        heap_allocator(heap_allocator&&) = default;

        //=== allocation/deallocation ===//
        /// \brief Allocates raw memory from the heap.
        ///
        /// If it fails, it behaves exactly as \c ::operator \c new
        /// (calling new handler, throwing exception...).
        void* allocate(std::size_t size, std::size_t alignment);

        /// \brief Deallocates raw memory.
        void deallocate(void *ptr, std::size_t size, std::size_t alignment) noexcept;
    };
}} // namespace foonathan::memory

#endif // PORTAL_MEMORY_HEAP_ALLOCATOR_HPP_INCLUDED
