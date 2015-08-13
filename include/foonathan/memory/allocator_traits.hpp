// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED

/// \file
/// \brief Allocator traits class template.

#include <cstddef>
#include <type_traits>

#include "detail/align.hpp"
#include "detail/utility.hpp"
#include "config.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <memory>
#endif

namespace foonathan { namespace memory
{
    namespace traits_detail // use seperate namespace to avoid name clashes
    {
        // full_concept has the best conversion rank, error the lowest
        // used to give priority to the functions
        struct error {};
        struct std_concept : error {};
        struct min_concept : std_concept {};
        struct full_concept : min_concept {};

        // used to delay assert in handle_error() until instantiation
        template <typename T>
        struct invalid_allocator_concept
        {
            static const bool error = false;
        };

        // used in lowest priority overload for a better error message
        template <typename T>
        void handle_error()
        {
            static_assert(invalid_allocator_concept<T>::error,
                "type does not model requirement for default allocator_traits specialization");
        }

        //=== allocator_type ===//
        // if Allocator has a type ::value_type, assume std_allocator and rebind to char
        // everything else returns the type as-is
    #if FOONATHAN_HOSTED_IMPLEMENTATION
        template <class Allocator>
        auto allocator_type(std_concept, FOONATHAN_SFINAE(std::declval<typename Allocator::value_type>()))
        -> typename std::allocator_traits<Allocator>::template rebind_alloc<char>;
    #endif

        template <class Allocator>
        Allocator allocator_type(error);

        //=== is_stateful ===//
        // first try to access Allocator::is_stateful,
        // then use whether or not the type is empty
        template <class Allocator>
        auto is_stateful(full_concept)
        -> decltype(typename Allocator::is_stateful{});

        template <class Allocator>
        auto is_stateful(min_concept)
        ->  FOONATHAN_REQUIRES_RET(std::is_empty<Allocator>::value, std::false_type);

        template <class Allocator>
        auto is_stateful(min_concept)
        -> FOONATHAN_REQUIRES_RET(!std::is_empty<Allocator>::value, std::true_type);

        //=== allocate_node() ===//
        // first try Allocator::allocate_node
        // then assume std_allocator and call Allocator::allocate
        // then error
        template <class Allocator>
        auto allocate_node(full_concept, Allocator &alloc,
                           std::size_t size, std::size_t alignment)
        -> FOONATHAN_AUTO_RETURN_TYPE(alloc.allocate_node(size, alignment), void*)

        template <class Allocator>
        auto allocate_node(std_concept, Allocator &alloc,
                           std::size_t size, std::size_t)
        -> FOONATHAN_AUTO_RETURN(static_cast<void*>(alloc.allocate(size)))

        template <class Allocator>
        void* allocate_node(error, Allocator &,
                           std::size_t, std::size_t)
        {
            return handle_error<Allocator>(), nullptr;
        }

        //=== deallocate_node() ===//
        // first try Allocator::deallocate_node
        // then assume std_allocator and call Allocator::deallocate
        // then error
        template <class Allocator>
        auto deallocate_node(full_concept, Allocator &alloc,
                            void* ptr, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        -> FOONATHAN_AUTO_RETURN_TYPE(alloc.deallocate_node(ptr, size, alignment), void)

        template <class Allocator>
        auto deallocate_node(std_concept, Allocator &alloc,
                             void *ptr, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        -> FOONATHAN_AUTO_RETURN_TYPE(alloc.deallocate(static_cast<char*>(ptr), size), void)

        template <class Allocator>
        void deallocate_node(error, Allocator&, void*, std::size_t, std::size_t)
        {
            handle_error<Allocator>();
        }

        //=== allocate_array() ===//
        // first try Allocator::allocate_array
        // then forward to allocate_node()
        template <class Allocator>
        auto allocate_array(full_concept, Allocator &alloc, std::size_t count,
                            std::size_t size, std::size_t alignment)
        -> FOONATHAN_AUTO_RETURN_TYPE(alloc.allocate_array(count, size, alignment), void*)

        template <class Allocator>
        void* allocate_array(min_concept, Allocator &alloc, std::size_t count,
                            std::size_t size, std::size_t alignment)
        {
            return allocate_node(full_concept{}, alloc, count * size, alignment);
        }

        //=== deallocate_array() ===//
        // first try Allocator::deallocate_array
        // then forward to deallocate_node()
        template <class Allocator>
        auto deallocate_array(full_concept, Allocator &alloc, void *ptr, std::size_t count,
                            std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        -> FOONATHAN_AUTO_RETURN_TYPE(alloc.deallocate_array(ptr, count, size, alignment), void)

        template <class Allocator>
        void deallocate_array(min_concept, Allocator &alloc, void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            deallocate_node(full_concept{}, alloc, ptr, count * size, alignment);
        }

        //=== max_node_size() ===//
        // first try Allocator::max_node_size()
        // then return maximum value
        template <class Allocator>
        auto max_node_size(full_concept, const Allocator &alloc)
        -> FOONATHAN_AUTO_RETURN_TYPE(alloc.max_node_size(), std::size_t)

        template <class Allocator>
        std::size_t max_node_size(min_concept, const Allocator &) FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }

        //=== max_node_size() ===//
        // first try Allocator::max_array_size()
        // then forward to max_node_size()
        template <class Allocator>
        auto max_array_size(full_concept, const Allocator &alloc)
        -> FOONATHAN_AUTO_RETURN_TYPE(alloc.max_array_size(), std::size_t)

        template <class Allocator>
        std::size_t max_array_size(min_concept, const Allocator &alloc)
        {
            return max_node_size(full_concept{}, alloc);
        }

        //=== max_alignment() ===//
        // first try Allocator::max_alignment()
        // then return detail::max_alignment
        template <class Allocator>
        auto max_alignment(full_concept, const Allocator &alloc)
        -> FOONATHAN_AUTO_RETURN_TYPE(alloc.max_alignment(), std::size_t)

        template <class Allocator>
        std::size_t max_alignment(min_concept, const Allocator &)
        {
            return detail::max_alignment;
        }
    } // namespace traits_detail

    /// \brief Default traits for \ref concept::RawAllocator classes.
    /// \details It provides the unified interface for all allocators.<br>
    /// Specialize it for own classes.
    /// \note Do not mix memory allocated through this interface and directly over the pool,
    /// since their allocation function might be implemented in a different way,
    /// e.g. this interfaces provides leak checking, the other one don't.
    /// \ref alignment "Valid alignment value"
    /// \ingroup memory
    template <class Allocator>
    class allocator_traits
    {
    public:
        /// \brief Must be the same as the template parameter.
        using allocator_type = decltype(traits_detail::allocator_type<Allocator>(traits_detail::full_concept{}));
        using is_stateful = decltype(traits_detail::is_stateful<Allocator>(traits_detail::full_concept{}));

        static void* allocate_node(allocator_type& state,
                                std::size_t size, std::size_t alignment)
        {
            return traits_detail::allocate_node(traits_detail::full_concept{},
                                                state, size, alignment);
        }

        static void* allocate_array(allocator_type& state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            return traits_detail::allocate_array(traits_detail::full_concept{},
                                                 state, count, size, alignment);
        }

        static void deallocate_node(allocator_type& state,
                    void *node, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            traits_detail::deallocate_node(traits_detail::full_concept{},
                                           state, node, size, alignment);
        }

        static void deallocate_array(allocator_type& state, void *array, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            traits_detail::deallocate_array(traits_detail::full_concept{},
                                            state, array, count, size, alignment);
        }

        static std::size_t max_node_size(const allocator_type &state)
        {
            return traits_detail::max_node_size(traits_detail::full_concept{}, state);
        }

        static std::size_t max_array_size(const allocator_type &state)
        {
            return traits_detail::max_array_size(traits_detail::full_concept{}, state);
        }

        static std::size_t max_alignment(const allocator_type &state)
        {
            return traits_detail::max_alignment(traits_detail::full_concept{}, state);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED
