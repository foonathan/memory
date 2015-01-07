#ifndef FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A new allocator.

#include <type_traits>

#include "tracking.hpp"

namespace foonathan { namespace memory
{
    /// \brief A \ref concept::RawAllocator that allocates memory using \c new.
    ///
    /// It is no singleton but stateless; each instance is the same.
    /// \ingroup memory
    class new_allocator : non_copyable // for consistency
    {
    public:
        using stateful = std::false_type;

        new_allocator(const new_allocator &) = delete;
        new_allocator(new_allocator&) = default;

        void* allocate(std::size_t size, std::size_t)
        {
            auto mem =  ::operator new(size);
            // no new override - need to call handler
            detail::on_heap_alloc(true, mem, size);
            return mem;
        }

        void deallocate(void *ptr, std::size_t size, std::size_t) noexcept
        {
            // no new override - need to call handler
            detail::on_heap_alloc(false, ptr, size);
            ::operator delete(ptr);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED
