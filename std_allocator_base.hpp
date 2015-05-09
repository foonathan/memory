// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_STD_ALLOCATOR_BASE_HPP_INCLUDED
#define FOONATHAN_MEMORY_STD_ALLOCATOR_BASE_HPP_INCLUDED

/// \file
/// \brief A utility base class for \c Allocator classes.

#include <limits>
#include <memory>

namespace foonathan { namespace memory
{
    /// \brief Generates the default stuff for \c Allocator classes.
    ///
    /// Inherit from it to gain the typedefs, construction/destruction, etc.
    /// \ingroup memory
    template <class Derived, typename T>
    class std_allocator_base
    {
    public:
        //=== typedefs ===//
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        //=== misc ===//
        template <typename U, typename ... Args>
        void construct(U *p, Args&&... args)
        {
            void* mem = p;
            ::new(mem) U(std::forward<Args>(args)...);
        }

        template <typename U>
        void destroy(U *p) FOONATHAN_NOEXCEPT
        {
            p->~U();
        }

        size_type max_size() const FOONATHAN_NOEXCEPT
        {
            return std::numeric_limits<size_type>::max();
        }

        friend bool operator!=(const Derived &lhs, const Derived &rhs)
        {
            return !(lhs == rhs);
        }

    protected:
        ~std_allocator_base() FOONATHAN_NOEXCEPT = default;
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_STD_ALLOCATOR_BASE_HPP_INCLUDED
