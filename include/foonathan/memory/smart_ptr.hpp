// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED
#define FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED

/// \file
/// \c std::make_unique() / \c std::make_shared() replacement allocating memory through a \concept{concept_rawallocator,RawAllocator}.
/// \note Only available on a hosted implementation.

#include "config.hpp"
#if !FOONATHAN_HOSTED_IMPLEMENTATION
#error "This header is only available for a hosted implementation."
#endif

#include <memory>
#include <type_traits>

#include "detail/utility.hpp"
#include "deleter.hpp"
#include "std_allocator.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            template <typename T, class RawAllocator, typename... Args>
            auto allocate_unique(allocator_reference<RawAllocator> alloc, Args&&... args)
                -> std::unique_ptr<T, allocator_deleter<T, RawAllocator>>
            {
                using raw_ptr = std::unique_ptr<T, allocator_deallocator<T, RawAllocator>>;

                auto memory = alloc.allocate_node(sizeof(T), FOONATHAN_ALIGNOF(T));
                // raw_ptr deallocates memory in case of constructor exception
                raw_ptr result(static_cast<T*>(memory), {alloc});
                // call constructor
                ::new (memory) T(detail::forward<Args>(args)...);
                // pass ownership to return value using a deleter that calls destructor
                return {result.release(), {alloc}};
            }

            template <typename T, typename... Args>
            void construct(std::true_type, T* cur, T* end, Args&&... args)
            {
                for (; cur != end; ++cur)
                    ::new (static_cast<void*>(cur)) T(detail::forward<Args>(args)...);
            }

            template <typename T, typename... Args>
            void construct(std::false_type, T* begin, T* end, Args&&... args)
            {
#if FOONATHAN_HAS_EXCEPTION_SUPPORT
                auto cur = begin;
                try
                {
                    for (; cur != end; ++cur)
                        ::new (static_cast<void*>(cur)) T(detail::forward<Args>(args)...);
                }
                catch (...)
                {
                    for (auto el = begin; el != cur; ++el)
                        el->~T();
                    throw;
                }
#else
                construct(std::true_type{}, begin, end, detail::forward<Args>(args)...);
#endif
            }

            template <typename T, class RawAllocator>
            auto allocate_array_unique(std::size_t size, allocator_reference<RawAllocator> alloc)
                -> std::unique_ptr<T[], allocator_deleter<T[], RawAllocator>>
            {
                using raw_ptr = std::unique_ptr<T[], allocator_deallocator<T[], RawAllocator>>;

                auto memory = alloc.allocate_array(size, sizeof(T), FOONATHAN_ALIGNOF(T));
                // raw_ptr deallocates memory in case of constructor exception
                raw_ptr result(static_cast<T*>(memory), {alloc, size});
                construct(std::integral_constant<bool, FOONATHAN_NOEXCEPT_OP(T())>{}, result.get(),
                          result.get() + size);
                // pass ownership to return value using a deleter that calls destructor
                return {result.release(), {alloc, size}};
            }
        } // namespace detail

        /// A \c std::unique_ptr that deletes using a \concept{concept_rawallocator,RawAllocator}.
        ///
        /// It is an alias template using \ref allocator_deleter as \c Deleter class.
        /// \ingroup memory adapter
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(unique_ptr,
                                 std::unique_ptr<T, allocator_deleter<T, RawAllocator>>);

        /// A \c std::unique_ptr that deletes using a \concept{concept_rawallocator,RawAllocator} and allows polymorphic types.
        ///
        /// It can only be created by converting a regular unique pointer to a pointer to a derived class,
        /// and is meant to be used inside containers.
        /// It is an alias template using \ref allocator_polymorphic_deleter as \c Deleter class.
        /// \note It has a relatively high overhead, so only use it if you have to.
        /// \ingroup memory adapter
        template <class BaseType, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            unique_base_ptr,
            std::unique_ptr<BaseType, allocator_polymorphic_deleter<BaseType, RawAllocator>>);

        /// Creates a \c std::unique_ptr using a \concept{concept_rawallocator,RawAllocator} for the allocation.
        /// \effects Allocates memory for the given type using the allocator
        /// and creates a new object inside it passing the given arguments to its constructor.
        /// \returns A \c std::unique_ptr owning that memory.
        /// \note If the allocator is stateful a reference to the \c RawAllocator will be stored inside the deleter,
        /// the caller has to ensure that the object lives as long as the smart pointer.
        /// \ingroup memory adapter
        template <typename T, class RawAllocator, typename... Args>
        auto allocate_unique(RawAllocator&& alloc, Args&&... args) -> FOONATHAN_REQUIRES_RET(
            !std::is_array<T>::value,
            std::unique_ptr<T, allocator_deleter<T, typename std::decay<RawAllocator>::type>>)
        {
            return detail::allocate_unique<T>(make_allocator_reference(
                                                  detail::forward<RawAllocator>(alloc)),
                                              detail::forward<Args>(args)...);
        }

        /// Creates a \c std::unique_ptr using a type-erased \concept{concept_rawallocator,RawAllocator} for the allocation.
        /// It is the same as the other overload but stores the reference to the allocator type-erased inside the \c std::unique_ptr.
        /// \effects Allocates memory for the given type using the allocator
        /// and creates a new object inside it passing the given arguments to its constructor.
        /// \returns A \c std::unique_ptr with a type-erased allocator reference owning that memory.
        /// \note If the allocator is stateful a reference to the \c RawAllocator will be stored inside the deleter,
        /// the caller has to ensure that the object lives as long as the smart pointer.
        /// \ingroup memory adapter
        template <typename T, class RawAllocator, typename... Args>
        auto allocate_unique(any_allocator, RawAllocator&& alloc, Args&&... args)
            -> FOONATHAN_REQUIRES_RET(!std::is_array<T>::value,
                                      std::unique_ptr<T, allocator_deleter<T, any_allocator>>)
        {
            return detail::allocate_unique<T, any_allocator>(make_allocator_reference(
                                                                 detail::forward<RawAllocator>(
                                                                     alloc)),
                                                             detail::forward<Args>(args)...);
        }

        /// Creates a \c std::unique_ptr owning an array using a \concept{concept_rawallocator,RawAllocator} for the allocation.
        /// \effects Allocates memory for an array of given size and value initializes each element inside of it.
        /// \returns A \c std::unique_ptr owning that array.
        /// \note If the allocator is stateful a reference to the \c RawAllocator will be stored inside the deleter,
        /// the caller has to ensure that the object lives as long as the smart pointer.
        /// \ingroup memory adapter
        template <typename T, class RawAllocator>
        auto allocate_unique(RawAllocator&& alloc, std::size_t size) -> FOONATHAN_REQUIRES_RET(
            std::is_array<T>::value,
            std::unique_ptr<T, allocator_deleter<T, typename std::decay<RawAllocator>::type>>)
        {
            return detail::allocate_array_unique<
                typename std::remove_extent<T>::type>(size,
                                                      make_allocator_reference(
                                                          detail::forward<RawAllocator>(alloc)));
        }

        /// Creates a \c std::unique_ptr owning an array using a type-erased \concept{concept_rawallocator,RawAllocator} for the allocation.
        /// It is the same as the other overload but stores the reference to the allocator type-erased inside the \c std::unique_ptr.
        /// \effects Allocates memory for an array of given size and value initializes each element inside of it.
        /// \returns A \c std::unique_ptr with a type-erased allocator reference owning that array.
        /// \note If the allocator is stateful a reference to the \c RawAllocator will be stored inside the deleter,
        /// the caller has to ensure that the object lives as long as the smart pointer.
        /// \ingroup memory adapter
        template <typename T, class RawAllocator>
        auto allocate_unique(any_allocator, RawAllocator&& alloc, std::size_t size)
            -> FOONATHAN_REQUIRES_RET(std::is_array<T>::value,
                                      std::unique_ptr<T, allocator_deleter<T, any_allocator>>)
        {
            return detail::allocate_array_unique<typename std::remove_extent<T>::type,
                                                 any_allocator>(size,
                                                                make_allocator_reference(
                                                                    detail::forward<RawAllocator>(
                                                                        alloc)));
        }

        /// Creates a \c std::shared_ptr using a \concept{concept_rawallocator,RawAllocator} for the allocation.
        /// It is similar to \c std::allocate_shared but uses a \c RawAllocator (and thus also supports any \c Allocator).
        /// \effects Calls \ref std_allocator::make_std_allocator to wrap the allocator and forwards to \c std::allocate_shared.
        /// \returns A \c std::shared_ptr created using \c std::allocate_shared.
        /// \note If the allocator is stateful a reference to the \c RawAllocator will be stored inside the shared pointer,
        /// the caller has to ensure that the object lives as long as the smart pointer.
        /// \ingroup memory adapter
        template <typename T, class RawAllocator, typename... Args>
        std::shared_ptr<T> allocate_shared(RawAllocator&& alloc, Args&&... args)
        {
            return std::allocate_shared<T>(make_std_allocator<T>(
                                               detail::forward<RawAllocator>(alloc)),
                                           detail::forward<Args>(args)...);
        }

#if !defined(DOXYGEN)
#include "detail/container_node_sizes.hpp"
#else
        /// Contains the node size needed for a `std::shared_ptr`.
        /// These classes are auto-generated and only available if the tools are build and without cross-compiling.
        /// \ingroup memory adapter
        template <typename T>
        struct shared_ptr_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };
#endif
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED
