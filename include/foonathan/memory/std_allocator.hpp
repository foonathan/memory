// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_STD_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_STD_ALLOCATOR_HPP_INCLUDED

#include <limits>
#include <new>

#include "config.hpp"
#include "allocator_storage.hpp"

namespace foonathan { namespace memory
{
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    class std_allocator;

    namespace detail
    {
        // whether or not derived from std_allocator template
        template <class C>
        using is_derived_from_std_allocator = detail::is_base_of_template<std_allocator, C>;
    } // namespace detail

    /// \brief Wraps a \ref concept::RawAllocator to create an \c std::allocator.
    /// \details To allow copying of the allocator, it will not store an object directly
    /// but instead wraps it into an allocator reference.
    /// This wrapping won't occur, if you pass it an \ref allocator_storage providing reference semantics,
    /// then it will stay in the it.<br>
    /// This means, that if you instantiate it with e.g. a \ref any_allocator_reference, it will be kept,
    /// allowing it to store any allocator.
    /// \ingroup memory
    template <typename T, class RawAllocator, class Mutex/* = default_mutex*/>
    class std_allocator
    : FOONATHAN_EBO(allocator_reference<RawAllocator, Mutex>)
    {
        using alloc_reference = allocator_reference<RawAllocator, Mutex>;
        // if it is any_allocator_reference an optimized implementation can be used
        using is_any = std::is_same<alloc_reference, any_allocator_reference<Mutex>>;

        static const std::size_t size;
        static const std::size_t alignment;

    public:
        //=== typedefs ===//
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        /// \brief Whether or not to swap the allocators, if the containers are swapped.
        /// \details It is \c true to ensure that the allocator stays with its memory.
        using propagate_on_container_swap = std::true_type;

        /// \brief Whether or not the allocators are moved, if the containers are move assigned.
        /// \details It is \c true to allow fast move operations.
        using propagate_on_container_move_assignment = std::true_type;

        /// \brief Whether or not to copy the allocators, if the containers are copy assigned.
        /// \details It is \c true for consistency.
        using propagate_on_container_copy_assignment = std::true_type;

        template <typename U>
        struct rebind {using other = std_allocator<U, RawAllocator, Mutex>;};

        /// \brief The underlying raw allocator.
        /// \details Or the polymorphic base class in case or \ref any_allocator_reference.
        using allocator_type = typename alloc_reference::allocator_type;

        /// \brief The mutex used for synchronization.
        using mutex = Mutex;

        //=== constructor ===//
        /// \brief Default constructor is only available for stateless allocators.
        std_allocator() FOONATHAN_NOEXCEPT
        : alloc_reference(allocator_type{}) {}

        /// \brief Creates it from a reference to a raw allocator.
        /// \details If the instantiation allows any allocator, it will accept any allocator,
        /// otherwise only the \ref allocator_type.
        template <class RawAlloc,
                // MSVC seems to ignore access rights in decltype SFINAE below
                // use this to prevent this constructor being chosen instead of move/copy for types inheriting from it
                FOONATHAN_REQUIRES(!detail::is_derived_from_std_allocator<RawAlloc>::value)>
        std_allocator(RawAlloc &alloc,
                      FOONATHAN_SFINAE(alloc_reference(alloc))) FOONATHAN_NOEXCEPT
        : alloc_reference(alloc) {}

        /// \brief Creates it from a temporary raw allocator.
        /// \details Same as above, but only valid for stateless allocators.
        template <class RawAlloc,
                // MSVC seems to ignore access rights in decltype SFINAE below
                // use this to prevent this constructor being chosen instead of move/copy for types inheriting from it
                FOONATHAN_REQUIRES(!detail::is_derived_from_std_allocator<RawAlloc>::value)>
        std_allocator(const RawAlloc &alloc,
                      FOONATHAN_SFINAE(alloc_reference(alloc))) FOONATHAN_NOEXCEPT
        : alloc_reference(alloc) {}

        /// \brief Creates it from another \ref alloc_reference or \ref any_allocator_reference.
        /// \details It must reference the same type and use the same mutex.
        std_allocator(const alloc_reference &alloc) FOONATHAN_NOEXCEPT
        : alloc_reference(alloc) {}

        /// \brief Conversion from any other \ref allocator_storage is forbidden.
        /// \details This prevents unnecessary nested wrapper classes.
        template <class StoragePolicy, class OtherMut>
        std_allocator(const allocator_storage<StoragePolicy, OtherMut>&) = delete;

        /// \brief Creates it from another \ref std_allocator allocating a different type.
        /// \details This is required by the \c Allcoator concept.
        template <typename U>
        std_allocator(const std_allocator<U, RawAllocator, Mutex> &alloc) FOONATHAN_NOEXCEPT
        : alloc_reference(alloc.get_allocator()) {}

        //=== allocation/deallocation ===//
        pointer allocate(size_type n, void * = nullptr)
        {
            return static_cast<pointer>(allocate_impl(is_any{}, n));
        }

        void deallocate(pointer p, size_type n) FOONATHAN_NOEXCEPT
        {
            deallocate_impl(is_any{}, p, n);
        }

        //=== construction/destruction ===//
        template <typename U, typename ... Args>
        void construct(U *p, Args&&... args)
        {
            void* mem = p;
            ::new(mem) U(detail::forward<Args>(args)...);
        }

        template <typename U>
        void destroy(U *p) FOONATHAN_NOEXCEPT
        {
            p->~U();
        }

        //=== getter ===//
        size_type max_size() const FOONATHAN_NOEXCEPT
        {
            return this->max_array_size() / sizeof(value_type);
        }

        /// \brief Returns the referenced allocator.
        /// \details It returns a reference for stateful allcoators and \ref any_allocator_reference,
        /// otherwise a temporary object.<br>
        /// \note This function does not lock the mutex.
        /// \see allocator_storage::get_allocator
        using alloc_reference::get_allocator;

        /// \brief Returns the referenced allocator while keeping the mutex locked.
        /// \details It returns a proxy object acting like a pointer to the allocator.
        /// \see allocator_storage::lock
        using alloc_reference::lock;

    private:
        // any_allocator_reference: use virtual function which already does a dispatch on node/array
        void* allocate_impl(std::true_type, size_type n)
        {
            return lock()->allocate_impl(n, size, alignment);
        }

        void deallocate_impl(std::true_type, void *ptr, size_type n)
        {
            lock()->deallocate_impl(ptr, n, size, alignment);
        }

        // alloc_reference: decide between node/array
        void* allocate_impl(std::false_type, size_type n)
        {
            if (n == 1)
                return this->allocate_node(size, alignment);
            else
                return this->allocate_array(n, size, alignment);
        }

        void deallocate_impl(std::false_type, void* ptr, size_type n)
        {
            if (n == 1)
                this->deallocate_node(ptr, size, alignment);
            else
                this->deallocate_array(ptr, n, size, alignment);
        }

        template <typename U> // stateful
        bool equal_to(std::true_type, const std_allocator<U, RawAllocator, mutex> &other) const FOONATHAN_NOEXCEPT
        {
            return &get_allocator() == &other.get_allocator();
        }

        template <typename U> // non-stateful
        bool equal_to(std::false_type, const std_allocator<U, RawAllocator, mutex> &) const FOONATHAN_NOEXCEPT
        {
            return true;
        }

        template <typename T1, typename T2, class Impl, class Mut>
        friend bool operator==(const std_allocator<T1, Impl, Mut> &lhs,
                               const std_allocator<T2, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT;
    };

    template <typename T, class Alloc, class Mut>
    const std::size_t std_allocator<T, Alloc, Mut>::size = sizeof(T);
    template <typename T, class Alloc, class Mut>
    const std::size_t std_allocator<T, Alloc, Mut>::alignment = FOONATHAN_ALIGNOF(T);

    /// @{
    /// \brief Makes an \ref std_allocator.
    /// \relates std_allocator
    template <typename T, class RawAllocator>
    auto make_std_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> std_allocator<T, typename std::decay<RawAllocator>::type>
    {
        return {detail::forward<RawAllocator>(allocator)};
    }

    template <typename T, class Mutex, class RawAllocator>
    auto make_std_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> std_allocator<T, typename std::decay<RawAllocator>::type, Mutex>
    {
        return {detail::forward<RawAllocator>(allocator)};
    }
    /// @}

    /// @{
    /// \brief Compares to \ref std_allocator.
    /// \details They are equal if either stateless or reference the same allocator.
    /// \relates std_allocator
    template <typename T, typename U, class Impl, class Mut>
    bool operator==(const std_allocator<T, Impl, Mut> &lhs,
                    const std_allocator<U, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT
    {
        using allocator = typename std_allocator<T, Impl, Mut>::allocator_type;
        return lhs.equal_to(typename allocator_traits<allocator>::is_stateful{}, rhs);
    }

    template <typename T, typename U, class Impl, class Mut>
    bool operator!=(const std_allocator<T, Impl, Mut> &lhs,
                    const std_allocator<U, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT
    {
        return !(lhs == rhs);
    }
    /// @}

    /// \brief Handy typedef to create an \ref std_allocator that can take any raw allocator.
    /// \details It uses \ref any_allocator_reference and its type erasure.<br>
    /// It is just an instantiation of \ref std_allocator with \ref any_allocator_reference.
    /// \ingroup memory
    template <typename T, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(any_std_allocator,
                             std_allocator<T, any_allocator, Mutex>);

    /// @{
    /// \brief Makes an \ref any_allocator.
    /// \relates any_std_allocator
    template <typename T, class RawAllocator>
    any_std_allocator<T> make_any_std_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    {
        return {detail::forward<RawAllocator>(allocator)};
    }

    template <typename T, class Mutex, class RawAllocator>
    any_std_allocator<T, Mutex> make_any_std_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    {
        return {detail::forward<RawAllocator>(allocator)};
    }
    /// @}
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_STD_ALLOCATOR_HPP_INCLUDED
