// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A new allocator.

#include <type_traits>

#include "raw_allocator_base.hpp"

namespace foonathan { namespace memory
{
    /// \brief A \ref concept::RawAllocator that allocates memory using \c new.
    ///
    /// It is no singleton but stateless; each instance is the same.
    /// \ingroup memory
    class new_allocator : public raw_allocator_base<new_allocator>
    {
    public:
        using is_stateful = std::false_type;
        
        /// \brief Allocates memory using \c ::operator \c new.
        void* allocate_node(std::size_t size, std::size_t alignment);

        /// \brief Deallocates memory using \c ::operator \c delete.
        void deallocate_node(void *node, std::size_t size, std::size_t alignment) noexcept;
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_NEW_ALLOCATOR_HPP_INCLUDED
