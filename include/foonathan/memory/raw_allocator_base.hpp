// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_RAW_ALLOCATOR_BASE
#define FOONATHAN_MEMORY_RAW_ALLOCATOR_BASE

/// \file
/// \brief Utility base class generating functions.

#include <cassert>
#include <cstddef>
#include <limits>

#include "detail/align.hpp"

namespace foonathan { namespace memory
{
    /// \brief Base class that generates default implementations for the more repetitive functions.
    template <class Derived>
    class raw_allocator_base
    {
    public:
        raw_allocator_base() FOONATHAN_NOEXCEPT {}
        raw_allocator_base(raw_allocator_base&&) FOONATHAN_NOEXCEPT {}
        raw_allocator_base& operator=(raw_allocator_base &&) FOONATHAN_NOEXCEPT {return *this;}

        /// @{
        /// \brief Array allocation forwards to node allocation.
        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            assert(count * size <= max_array_size() && "invalid array length");
            return static_cast<Derived*>(this)->allocate_node(count * size, alignment);
        }

        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            static_cast<Derived*>(this)->deallocate_node(ptr, count * size, alignment);
        }
        /// @}

        /// @{
        /// \brief Returns maximum value.
        std::size_t max_node_size() const FOONATHAN_NOEXCEPT
        {
            return std::numeric_limits<std::size_t>::max();
        }

        std::size_t max_array_size() const FOONATHAN_NOEXCEPT
        {
            return std::numeric_limits<std::size_t>::max();
        }
        /// @}

        /// \brief Returns \c alignof(std::max_align_t).
        std::size_t max_alignment() const FOONATHAN_NOEXCEPT
        {
            return detail::max_alignment;
        }

    protected:
        ~raw_allocator_base() FOONATHAN_NOEXCEPT = default;
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_RAW_ALLOCATOR_BASE
