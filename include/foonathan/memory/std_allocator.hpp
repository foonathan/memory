// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_STD_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_STD_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::std_allocator and related classes and functions.

#include <new>
#include <type_traits>

#include "detail/utility.hpp"
#include "config.hpp"
#include "allocator_storage.hpp"
#include "threading.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace traits_detail
        {
            template <class RawAllocator>
            auto propagate_on_container_swap(std_concept) ->
                typename RawAllocator::propagate_on_container_swap;

            template <class RawAllocator>
            auto propagate_on_container_swap(min_concept) -> std::true_type;

            template <class RawAllocator>
            auto propagate_on_container_move_assignment(std_concept) ->
                typename RawAllocator::propagate_on_container_move_assignment;

            template <class RawAllocator>
            auto propagate_on_container_move_assignment(min_concept) -> std::true_type;

            template <class RawAllocator>
            auto propagate_on_container_copy_assignment(std_concept) ->
                typename RawAllocator::propagate_on_container_copy_assignment;

            template <class RawAllocator>
            auto propagate_on_container_copy_assignment(min_concept) -> std::true_type;
        } // namespace traits_detail

        /// Controls the propagation of a \ref std_allocator for a certain \concept{concept_rawallocator,RawAllocator}.
        /// \ingroup memory adapter
        template <class RawAllocator>
        struct propagation_traits
        {
            using propagate_on_container_swap =
                decltype(traits_detail::propagate_on_container_swap<RawAllocator>(
                    traits_detail::full_concept{}));

            using propagate_on_container_move_assignment =
                decltype(traits_detail::propagate_on_container_move_assignment<RawAllocator>(
                    traits_detail::full_concept{}));

            using propagate_on_container_copy_assignment =
                decltype(traits_detail::propagate_on_container_copy_assignment<RawAllocator>(
                    traits_detail::full_concept{}));

            template <class AllocReference>
            static AllocReference select_on_container_copy_construction(const AllocReference& alloc)
            {
                return alloc;
            }
        };

        /// Wraps a \concept{concept_rawallocator,RawAllocator} and makes it a "normal" \c Allocator.
        /// It allows using a \c RawAllocator anywhere a \c Allocator is required.
        /// It stores the allocator as \ref allocator_reference to allow required copying of a \c Allocator,
        /// synchronization is done via a \c Mutex which defaults to \ref default_mutex.
        /// \ingroup memory adapter
        template <typename T, class RawAllocator, class Mutex = default_mutex>
        class std_allocator : FOONATHAN_EBO(allocator_reference<RawAllocator, Mutex>)
        {
            using alloc_reference = allocator_reference<RawAllocator, Mutex>;
            // if it is any_allocator_reference an optimized implementation can be used
            using is_any = std::is_same<alloc_reference, any_allocator_reference<Mutex>>;

            using prop_traits = propagation_traits<RawAllocator>;

        public:
            //=== typedefs ===//
            using value_type      = T;
            using pointer         = T*;
            using const_pointer   = const T*;
            using reference       = T&;
            using const_reference = const T&;
            using size_type       = std::size_t;
            using difference_type = std::ptrdiff_t;

            using propagate_on_container_swap = typename prop_traits::propagate_on_container_swap;
            using propagate_on_container_move_assignment =
                typename prop_traits::propagate_on_container_move_assignment;
            using propagate_on_container_copy_assignment =
                typename prop_traits::propagate_on_container_copy_assignment;

            template <typename U>
            struct rebind
            {
                using other = std_allocator<U, RawAllocator, Mutex>;
            };

            using allocator_type = typename alloc_reference::allocator_type;
            using mutex          = Mutex;

            //=== constructor ===//
            /// \effects Default constructs it by storing a default constructed, stateless \c RawAllocator inside the reference.
            /// \requires The \c RawAllocator type is stateless, otherwise the body of this function will not compile.
            std_allocator() FOONATHAN_NOEXCEPT : alloc_reference(allocator_type{})
            {
            }

            /// \effects Creates it from a reference to a \c RawAllocator.
            /// It will store an \ref allocator_reference to it.
            /// \requires The expression <tt>allocator_reference<RawAllocator, Mutex>(alloc)</tt> is well-formed,
            /// that is either \c RawAlloc is the same as \c RawAllocator or \c RawAllocator is the tag type \ref any_allocator.
            /// If the requirement is not fulfilled this function does not participate in overload resolution.
            /// \note The caller has to ensure that the lifetime of the \c RawAllocator is at least as long as the lifetime
            /// of this \ref std_allocator object.
            template <class RawAlloc,
                      // MSVC seems to ignore access rights in decltype SFINAE below
                      // use this to prevent this constructor being chosen instead of move/copy for types inheriting from it
                      FOONATHAN_REQUIRES((!std::is_base_of<std_allocator, RawAlloc>::value))>
            std_allocator(RawAlloc& alloc,
                          FOONATHAN_SFINAE(alloc_reference(alloc))) FOONATHAN_NOEXCEPT
                : alloc_reference(alloc)
            {
            }

            /// \effects Creates it from a stateless, temporary \c RawAllocator object.
            /// It will not store a reference but create it on the fly.
            /// \requires The \c RawAllocator is stateless
            /// and the expression <tt>allocator_reference<RawAllocator, Mutex>(alloc)</tt> is well-formed as above,
            /// otherwise this function does not participate in overload resolution.
            template <class RawAlloc,
                      // MSVC seems to ignore access rights in decltype SFINAE below
                      // use this to prevent this constructor being chosen instead of move/copy for types inheriting from it
                      FOONATHAN_REQUIRES((!std::is_base_of<std_allocator, RawAlloc>::value))>
            std_allocator(const RawAlloc& alloc,
                          FOONATHAN_SFINAE(alloc_reference(alloc))) FOONATHAN_NOEXCEPT
                : alloc_reference(alloc)
            {
            }

            /// \effects Creates it from another \ref allocator_reference using the same allocator type and mutex.
            std_allocator(const alloc_reference& alloc) FOONATHAN_NOEXCEPT : alloc_reference(alloc)
            {
            }

            /// \details Implicit conversion from any other \ref allocator_storage is forbidden
            /// to prevent accidentally wrapping another \ref allocator_storage inside a \ref allocator_reference.
            template <class StoragePolicy, class OtherMut>
            std_allocator(const allocator_storage<StoragePolicy, OtherMut>&) = delete;

            /// @{
            /// \effects Creates it from another \ref std_allocator allocating a different type.
            /// This is required by the \c Allcoator concept and simply takes the same \ref allocator_reference.
            template <typename U>
            std_allocator(const std_allocator<U, RawAllocator, Mutex>& alloc) FOONATHAN_NOEXCEPT
                : alloc_reference(alloc.get_allocator())
            {
            }

            template <typename U>
            std_allocator(std_allocator<U, RawAllocator, Mutex>& alloc) FOONATHAN_NOEXCEPT
                : alloc_reference(alloc.get_allocator())
            {
            }
            /// @}

            /// \returns A copy of the allocator.
            /// This is required by the \c Allocator concept and forwards to the \ref propagation_traits.
            std_allocator<T, RawAllocator, Mutex> select_on_container_copy_construction() const
            {
                return prop_traits::select_on_container_copy_construction(*this);
            }

            //=== allocation/deallocation ===//
            /// \effects Allocates memory using the underlying \concept{concept_rawallocator,RawAllocator}.
            /// If \c n is \c 1, it will call <tt>allocate_node(sizeof(T), alignof(T))</tt>,
            /// otherwise <tt>allocate_array(n, sizeof(T), alignof(T))</tt>.
            /// \returns A pointer to a memory block suitable for \c n objects of type \c T.
            /// \throws Anything thrown by the \c RawAllocator.
            pointer allocate(size_type n, void* = nullptr)
            {
                return static_cast<pointer>(allocate_impl(is_any{}, n));
            }

            /// \effects Deallcoates memory using the underlying \concept{concept_rawallocator,RawAllocator}.
            /// It will forward to the deallocation function in the same way as in \ref allocate().
            /// \requires The pointer must come from a previous call to \ref allocate() with the same \c n on this object or any copy of it.
            void deallocate(pointer p, size_type n) FOONATHAN_NOEXCEPT
            {
                deallocate_impl(is_any{}, p, n);
            }

            //=== construction/destruction ===//
            /// \effects Creates an object of type \c U at given address using the passed arguments.
            template <typename U, typename... Args>
            void construct(U* p, Args&&... args)
            {
                void* mem = p;
                ::new (mem) U(detail::forward<Args>(args)...);
            }

            /// \effects Calls the destructor for an object of type \c U at given address.
            template <typename U>
            void destroy(U* p) FOONATHAN_NOEXCEPT
            {
                p->~U();
            }

            //=== getter ===//
            /// \returns The maximum size for an allocation which is <tt>max_array_size() / sizeof(value_type)</tt>.
            /// This is only an upper bound, not the exact maximum.
            size_type max_size() const FOONATHAN_NOEXCEPT
            {
                return this->max_array_size() / sizeof(value_type);
            }

            /// @{
            /// \effects Returns a reference to the referenced allocator.
            /// \returns For stateful allocators: A (\c const) reference to the stored allocator.
            /// For stateless allocators: A temporary constructed allocator.
            /// \note This does not lock the \c Mutex.
            auto get_allocator() FOONATHAN_NOEXCEPT
                -> decltype(std::declval<alloc_reference>().get_allocator())
            {
                return alloc_reference::get_allocator();
            }

            auto get_allocator() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<const alloc_reference>().get_allocator())
            {
                return alloc_reference::get_allocator();
            }
            /// @}

            /// @{
            /// \returns A proxy object that acts like a pointer to the stored allocator.
            /// It cannot be reassigned to point to another allocator object and only moving is supported, which is destructive.
            /// As long as the proxy object lives and is not moved from, the \c Mutex will be kept locked.
            /// \requires The result of \ref get_allocator() must not be a temporary, otherwise the body of this function will not compile.
            auto lock() FOONATHAN_NOEXCEPT -> decltype(std::declval<alloc_reference>().lock())
            {
                return alloc_reference::lock();
            }

            auto lock() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<const alloc_reference>().lock())
            {
                return alloc_reference::lock();
            }
            /// @}.

        private:
            // any_allocator_reference: use virtual function which already does a dispatch on node/array
            void* allocate_impl(std::true_type, size_type n)
            {
                return lock()->allocate_impl(n, sizeof(T), FOONATHAN_ALIGNOF(T));
            }

            void deallocate_impl(std::true_type, void* ptr, size_type n)
            {
                lock()->deallocate_impl(ptr, n, sizeof(T), FOONATHAN_ALIGNOF(T));
            }

            // alloc_reference: decide between node/array
            void* allocate_impl(std::false_type, size_type n)
            {
                if (n == 1)
                    return this->allocate_node(sizeof(T), FOONATHAN_ALIGNOF(T));
                else
                    return this->allocate_array(n, sizeof(T), FOONATHAN_ALIGNOF(T));
            }

            void deallocate_impl(std::false_type, void* ptr, size_type n)
            {
                if (n == 1)
                    this->deallocate_node(ptr, sizeof(T), FOONATHAN_ALIGNOF(T));
                else
                    this->deallocate_array(ptr, n, sizeof(T), FOONATHAN_ALIGNOF(T));
            }

            template <typename U> // stateful
            bool equal_to_impl(std::true_type, const std_allocator<U, RawAllocator, mutex>& other)
                const FOONATHAN_NOEXCEPT
            {
                return &get_allocator() == &other.get_allocator();
            }

            template <typename U> // non-stateful
            bool equal_to_impl(std::false_type, const std_allocator<U, RawAllocator, mutex>&) const
                FOONATHAN_NOEXCEPT
            {
                return true;
            }

            template <typename U> // shared
            bool equal_to(std::true_type, const std_allocator<U, RawAllocator, mutex>& other) const
                FOONATHAN_NOEXCEPT
            {
                return get_allocator() == other.get_allocator();
            }

            template <typename U> // not shared
            bool equal_to(std::false_type, const std_allocator<U, RawAllocator, mutex>& other) const
                FOONATHAN_NOEXCEPT
            {
                return equal_to_impl(typename allocator_traits<RawAllocator>::is_stateful{}, other);
            }

            template <typename T1, typename T2, class Impl, class Mut>
            friend bool operator==(const std_allocator<T1, Impl, Mut>& lhs,
                                   const std_allocator<T2, Impl, Mut>& rhs) FOONATHAN_NOEXCEPT;
        };

        /// \effects Compares two \ref std_allocator object, they are equal if either stateless or reference the same allocator.
        /// \returns The result of the comparision for equality.
        /// \relates std_allocator
        template <typename T, typename U, class Impl, class Mut>
        bool operator==(const std_allocator<T, Impl, Mut>& lhs,
                        const std_allocator<U, Impl, Mut>& rhs) FOONATHAN_NOEXCEPT
        {
            return lhs.equal_to(is_shared_allocator<Impl>{}, rhs);
        }

        /// \effects Compares two \ref std_allocator object, they are equal if either stateless or reference the same allocator.
        /// \returns The result of the comparision for inequality.
        /// \relates std_allocator
        template <typename T, typename U, class Impl, class Mut>
        bool operator!=(const std_allocator<T, Impl, Mut>& lhs,
                        const std_allocator<U, Impl, Mut>& rhs) FOONATHAN_NOEXCEPT
        {
            return !(lhs == rhs);
        }

        /// \returns A new \ref std_allocator for a given type using a certain allocator object.
        /// \relates std_allocator
        template <typename T, class RawAllocator>
        auto make_std_allocator(RawAllocator&& allocator) FOONATHAN_NOEXCEPT
            -> std_allocator<T, typename std::decay<RawAllocator>::type>
        {
            return {detail::forward<RawAllocator>(allocator)};
        }

        /// \returns A new \ref std_allocator for a given type and \c Mutex using a certain allocator object.
        /// \relates std_allocator
        template <typename T, class Mutex, class RawAllocator>
        auto make_std_allocator(RawAllocator&& allocator) FOONATHAN_NOEXCEPT
            -> std_allocator<T, typename std::decay<RawAllocator>::type, Mutex>
        {
            return {detail::forward<RawAllocator>(allocator)};
        }

        /// An alias template for \ref std_allocator using a type-erased \concept{concept_rawallocator,RawAllocator}.
        /// This is the same as using a \ref std_allocator with the tag type \ref any_allocator.
        /// The implementation is optimized to call fewer virtual functions.
        /// \ingroup memory adapter
        template <typename T, class Mutex = default_mutex>
        FOONATHAN_ALIAS_TEMPLATE(any_std_allocator, std_allocator<T, any_allocator, Mutex>);

        /// \returns A new \ref any_std_allocator for a given type using a certain allocator object.
        /// \relates any_std_allocator
        template <typename T, class RawAllocator>
        any_std_allocator<T> make_any_std_allocator(RawAllocator&& allocator) FOONATHAN_NOEXCEPT
        {
            return {detail::forward<RawAllocator>(allocator)};
        }

        /// \returns A new \ref any_std_allocator for a given type and \c Mutex using a certain allocator object.
        /// \relates any_std_allocator
        template <typename T, class Mutex, class RawAllocator>
        any_std_allocator<T, Mutex> make_any_std_allocator(RawAllocator&& allocator)
            FOONATHAN_NOEXCEPT
        {
            return {detail::forward<RawAllocator>(allocator)};
        }
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_STD_ALLOCATOR_HPP_INCLUDED
