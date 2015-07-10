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
#include "config.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <memory>
#endif

namespace foonathan { namespace memory
{
    namespace traits_detail // use seperate namespace to avoid name clashes
    {
        template <class Allocator>
        auto allocate_array(int, Allocator &alloc, std::size_t count,
                            std::size_t size, std::size_t alignment)
        -> decltype(alloc.allocate_array(count, size, alignment))
        {
            return alloc.allocate_array(count, size, alignment);
        }

        template <class Allocator>
        void* allocate_array(short, Allocator &alloc, std::size_t count,
                            std::size_t size, std::size_t alignment)
        {
            return alloc.allocate_node(count * size, alignment);
        }

        template <class Allocator>
        auto deallocate_array(int, Allocator &alloc, void *ptr, std::size_t count,
                            std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        -> decltype(alloc.deallocate_array(ptr, count, size, alignment))
        {
            alloc.deallocate_array(ptr, count, size, alignment);
        }

        template <class Allocator>
        void deallocate_array(short, Allocator &alloc, void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            alloc.deallocate_node(ptr, count * size, alignment);
        }

        template <class Allocator>
        auto max_array_size(int, const Allocator &alloc)
        -> decltype(alloc.max_array_size())
        {
            return alloc.max_array_size();
        }

        template <class Allocator>
        std::size_t max_array_size(short, const Allocator &alloc)
        {
            return alloc.max_node_size();
        }

        template <class Allocator>
        auto max_alignment(int, const Allocator &alloc)
        -> decltype(alloc.max_alignment())
        {
            return alloc.max_alignment();
        }

        template <class Allocator>
        std::size_t max_alignment(short, const Allocator &)
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
    /// \ingroup memory
    template <class Allocator>
    class allocator_traits
    {
    public:
        /// \brief Must be the same as the template parameter.
        using allocator_type = Allocator;
        using is_stateful = typename Allocator::is_stateful;

        static void* allocate_node(allocator_type& state,
                                std::size_t size, std::size_t alignment)
        {
            return state.allocate_node(size, alignment);
        }

        static void* allocate_array(allocator_type& state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            return traits_detail::allocate_array(0, state, count, size, alignment);
        }

        static void deallocate_node(allocator_type& state,
                    void *node, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            state.deallocate_node(node, size, alignment);
        }

        static void deallocate_array(allocator_type& state, void *array, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            traits_detail::deallocate_array(0, state, array, count, size, alignment);
        }

        static std::size_t max_node_size(const allocator_type &state)
        {
            return state.max_node_size();
        }

        static std::size_t max_array_size(const allocator_type &state)
        {
            return traits_detail::max_array_size(0, state);
        }

        static std::size_t max_alignment(const allocator_type &state)
        {
            return traits_detail::max_alignment(0, state);
        }
    };

    /// \brief Provides all traits functions for \c std::allocator types.
    /// \details Inherit from it when specializing the \ref allocator_traits for such allocators.<br>
    /// It uses the std::allocator_traits to call the functions.
    /// \note It is only available on a hosted implementation.
    /// \ingroup memory
#if FOONATHAN_HOSTED_IMPLEMENTATION
    template <class StdAllocator>
    class allocator_traits_std_allocator
    {
    public:
        /// \brief The state type is the Allocator rebind for \c char.
        using allocator_type = typename std::allocator_traits<StdAllocator>::
                                    template rebind_alloc<char>;

        /// \brief Assume it is not stateful when std::is_empty.
        using is_stateful = std::integral_constant<bool, !std::is_empty<allocator_type>::value>;

    private:
        using std_traits = std::allocator_traits<allocator_type>;

    public:
        /// @{
        /// \brief Allocation functions forward to \c allocate().
        /// \details They request a char-array of sufficient length.<br>
        /// Alignment is ignored.
        static void* allocate_node(allocator_type& state,
                                std::size_t size, std::size_t)
        {
            return std_traits::allocate(state, size);
        }

        static void* allocate_array(allocator_type& state, std::size_t count,
                             std::size_t size, std::size_t)
        {
            return std_traits::allocate(state, count * size);
        }
        /// @}

        /// @{
        /// \brief Deallocation functions forward to \c deallocate().
        static void deallocate_node(allocator_type& state,
                    void *node, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            std_traits::deallocate(state, static_cast<typename std_traits::pointer>(node), size);
        }

        static void deallocate_array(allocator_type& state, void *array, std::size_t count,
                              std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
        {
            std_traits::deallocate(state, static_cast<typename std_traits::pointer>(array), count * size);
        }
        /// @}

        /// @{
        /// \brief The maximum size forwards to \c max_size().
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return std_traits::max_size(state);
        }

        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return std_traits::max_size(state);
        }
        /// @}

        /// \brief Maximum alignment is \c alignof(std::max_align_t).
        static std::size_t max_alignment(const allocator_type &) FOONATHAN_NOEXCEPT
        {
            return  detail::max_alignment;
        }
    };
#endif

    /// \brief Specialization of \ref allocator_traits for \c std::allocator.
    /// \note It is only available on a hosted implementation.
    /// \ingroup memory
#if FOONATHAN_HOSTED_IMPLEMENTATION
    template <typename ... ImplArguments>
    class allocator_traits<std::allocator<ImplArguments...>>
    // implementation note: std::allocator can have any number of implementation defined, defaulted arguments
    : public allocator_traits_std_allocator<std::allocator<ImplArguments...>>
    {};
#endif
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED
