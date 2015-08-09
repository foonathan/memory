// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DELETER_HPP_INCLUDED
#define FOONATHAN_MEMORY_DELETER_HPP_INCLUDED

/// \file
/// \brief RawAllocator deleter classes to be used in smart pointers, std::function,...

#include "allocator_adapter.hpp"

namespace foonathan { namespace memory
{
    /// \brief A deleter class that calls the appropriate deallocate function.
    /// \details It stores an \ref allocator_reference. It does not call any destrucotrs.
    /// \ingroup memory
    template <typename Type, class RawAllocator>
    class raw_allocator_deallocator
    : FOONATHAN_EBO(allocator_reference<RawAllocator>)
    {
    public:
        using raw_allocator = RawAllocator;
        using value_type = Type;

        /// \brief Creates it giving it the allocator used for deallocation.
        raw_allocator_deallocator(allocator_reference<raw_allocator> alloc) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(std::move(alloc)) {}

        /// \brief Deallocates the memory via the stored allocator.
        /// \details It calls \ref allocator_traits::deallocate_node, but no destructors.
        void operator()(value_type *pointer) FOONATHAN_NOEXCEPT
        {
            this->deallocate_node(pointer, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }

        /// \brief Returns a reference to the stored allocator.
        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<raw_allocator>>().get_allocator())
        {
            return this->allocator_reference<raw_allocator>::get_allocator();
        }
    };

    /// \brief Specialization of \ref raw_allocator_deallocator for arrays.
    /// \ingroup memory
    template <typename Type, class RawAllocator>
    class raw_allocator_deallocator<Type[], RawAllocator>
    : FOONATHAN_EBO(allocator_reference<RawAllocator>)
    {
    public:
        using raw_allocator = RawAllocator;
        using value_type = Type;

        /// \brief Creates it giving it the allocator used for deallocation and the array size.
        raw_allocator_deallocator(allocator_reference<raw_allocator> alloc,
                                  std::size_t size) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(std::move(alloc)),
          size_(size) {}

        /// \brief Deallocates the memory via the stored allocator.
        /// \details It calls \ref allocator_traits::deallocate_array, but no destructors.
        void operator()(value_type *pointer) FOONATHAN_NOEXCEPT
        {
            this->deallocate_array(pointer, size_, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }

        /// \brief Returns a reference to the stored allocator.
        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<raw_allocator>>().get_allocator())
        {
            return this->allocator_reference<raw_allocator>::get_allocator();
        }

        /// \brief Returns the array size.
        std::size_t array_size() const FOONATHAN_NOEXCEPT
        {
            return size_;
        }

    private:
        std::size_t size_;
    };

    /// \brief A deleter class that calls the appropriate destructors and deallocate function.
    /// \details It stores an \ref allocator_reference. It calls destructors.
    /// \ingroup memory
    template <typename Type, class RawAllocator>
    class raw_allocator_deleter
    : FOONATHAN_EBO(allocator_reference<RawAllocator>)
    {
    public:
        using raw_allocator = RawAllocator;
        using value_type = Type;

        /// \brief Creates it giving it the allocator used for deallocation.
        raw_allocator_deleter(allocator_reference<raw_allocator> alloc) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(std::move(alloc)) {}

        /// \brief Deallocates the memory via the stored allocator.
        /// \details It calls the destructor and \ref allocator_traits::deallocate_node.
        void operator()(value_type *pointer) FOONATHAN_NOEXCEPT
        {
            pointer->~value_type();
            this->deallocate_node(pointer, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }

        /// \brief Returns a reference to the stored allocator.
        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<raw_allocator>>().get_allocator())
        {
            return this->allocator_reference<raw_allocator>::get_allocator();
        }
    };

    /// \brief Specialization of \ref raw_allocator_deleter for arrays.
    /// \ingroup memory
    template <typename Type, class RawAllocator>
    class raw_allocator_deleter<Type[], RawAllocator>
    : FOONATHAN_EBO(allocator_reference<RawAllocator>)
    {
    public:
        using raw_allocator = RawAllocator;
        using value_type = Type;

        /// \brief Creates it giving it the allocator used for deallocation and the array size.
        raw_allocator_deleter(allocator_reference<raw_allocator> alloc,
                              std::size_t size) FOONATHAN_NOEXCEPT
         : allocator_reference<RawAllocator>(std::move(alloc)),
           size_(size) {}

        /// \brief Deallocates the memory via the stored allocator.
        /// \details It calls the destructors and \ref allocator_traits::deallocate_array.
        void operator()(value_type *pointer) FOONATHAN_NOEXCEPT
        {
            for (auto cur = pointer; cur != pointer + size_; ++cur)
                cur->~value_type();
            this->deallocate_array(pointer, size_, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }

        /// \brief Returns a reference to the stored allocator.
        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<raw_allocator>>().get_allocator())
        {
            return this->allocator_reference<raw_allocator>::get_allocator();
        }

        /// \brief Returns the array size.
        std::size_t array_size() const FOONATHAN_NOEXCEPT
        {
            return size_;
        }

    private:
        std::size_t size_;
    };
}} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_DELETER_HPP_INCLUDED
