// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED

/// \file
/// \brief Allocator traits class template.

#include <cstddef>
#include <memory>
#include <type_traits>

#include "detail/align.hpp"

namespace foonathan { namespace memory
{
    /// \brief Default traits for \ref concept::RawAllocator classes.
    /// \details It provides the unified interface for all allocators.<br>
    /// Specialize it for own classes.
    /// \note Do not mix memory allocated through this interface and directly over the pool,
    /// since their allocation function might be implemented in a different way,\
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
            return state.allocate_array(count, size, alignment);
        }

        static void deallocate_node(allocator_type& state,
                    void *node, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            state.deallocate_node(node, size, alignment);
        }

        static void deallocate_array(allocator_type& state, void *array, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            state.deallocate_array(array, count, size, alignment);
        }

        static std::size_t max_node_size(const allocator_type &state)
        {
            return state.max_node_size();
        }

        static std::size_t max_array_size(const allocator_type &state)
        {
            return state.max_array_size();
        }

        static std::size_t max_alignment(const allocator_type &state)
        {
            return state.max_alignment();
        }
    };

    /// \brief Provides all traits functions for \c std::allocator types.
    /// \details Inherit from it when specializing the \ref allocator_traits for such allocators.<br>
    /// It uses the std::allocator_traits to call the functions.
    /// \ingroup memory
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

    /// \brief Specialization of \ref allocator_traits for \c std::allocator.
    /// \ingroup memory
    // implementation note: std::allocator can have any number of implementation defined, defaulted arguments
    template <typename ... ImplArguments>
    class allocator_traits<std::allocator<ImplArguments...>>
    : public allocator_traits_std_allocator<std::allocator<ImplArguments...>>
    {};
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED
