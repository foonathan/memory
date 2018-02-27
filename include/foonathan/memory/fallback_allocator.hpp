// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_FALLBACK_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_FALLBACK_ALLOCATOR_HPP_INCLUDED

/// \file
//// Class template \ref foonathan::memory::fallback_allocator.

#include "detail/ebo_storage.hpp"
#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "config.hpp"

namespace foonathan
{
    namespace memory
    {
        /// A \concept{raw_allocator,RawAllocator} with a fallback.
        /// Allocation first tries `Default`, if it fails,
        /// it uses `Fallback`.
        /// \requires `Default` must be a composable \concept{concept_rawallocator,RawAllocator},
        /// `Fallback` must be a \concept{concept_rawallocator,RawAllocator}.
        /// \ingroup memory adapter
        template <class Default, class Fallback>
        class fallback_allocator
        : FOONATHAN_EBO(detail::ebo_storage<0, typename allocator_traits<Default>::allocator_type>),
          FOONATHAN_EBO(detail::ebo_storage<1, typename allocator_traits<Fallback>::allocator_type>)
        {
            using default_traits             = allocator_traits<Default>;
            using default_composable_traits  = composable_allocator_traits<Default>;
            using fallback_traits            = allocator_traits<Fallback>;
            using fallback_composable_traits = composable_allocator_traits<Fallback>;
            using fallback_composable =
                is_composable_allocator<typename fallback_traits::allocator_type>;

        public:
            using default_allocator_type  = typename allocator_traits<Default>::allocator_type;
            using fallback_allocator_type = typename allocator_traits<Fallback>::allocator_type;

            using is_stateful =
                std::integral_constant<bool, default_traits::is_stateful::value
                                                 || fallback_traits::is_stateful::value>;

            /// \effects Default constructs both allocators.
            /// \notes This function only participates in overload resolution, if both allocators are not stateful.
            FOONATHAN_ENABLE_IF(!is_stateful::value)
            fallback_allocator()
            : detail::ebo_storage<0, default_allocator_type>({}),
              detail::ebo_storage<1, fallback_allocator_type>({})
            {
            }

            /// \effects Constructs the allocator by passing in the two allocators it has.
            explicit fallback_allocator(default_allocator_type&&  default_alloc,
                                        fallback_allocator_type&& fallback_alloc = {})
            : detail::ebo_storage<0, default_allocator_type>(detail::move(default_alloc)),
              detail::ebo_storage<1, fallback_allocator_type>(detail::move(fallback_alloc))
            {
            }

            /// @{
            /// \effects First calls the compositioning (de)allocation function on the `default_allocator_type`.
            /// If that fails, uses the non-compositioning function of the `fallback_allocator_type`.
            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                auto ptr = default_composable_traits::try_allocate_node(get_default_allocator(),
                                                                        size, alignment);
                if (!ptr)
                    ptr = fallback_traits::allocate_node(get_fallback_allocator(), size, alignment);
                return ptr;
            }

            void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
            {
                auto ptr = default_composable_traits::try_allocate_array(get_default_allocator(),
                                                                         count, size, alignment);
                if (!ptr)
                    ptr = fallback_traits::allocate_array(get_fallback_allocator(), count, size,
                                                          alignment);
                return ptr;
            }

            void deallocate_node(void* ptr, std::size_t size,
                                 std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                auto res = default_composable_traits::try_deallocate_node(get_default_allocator(),
                                                                          ptr, size, alignment);
                if (!res)
                    fallback_traits::deallocate_node(get_fallback_allocator(), ptr, size,
                                                     alignment);
            }

            void deallocate_array(void* ptr, std::size_t count, std::size_t size,
                                  std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                auto res =
                    default_composable_traits::try_deallocate_array(get_default_allocator(), ptr,
                                                                    count, size, alignment);
                if (!res)
                    fallback_traits::deallocate_array(get_fallback_allocator(), ptr, count, size,
                                                      alignment);
            }
            /// @}

            /// @{
            /// \effects First calls the compositioning (de)allocation function on the `default_allocator_type`.
            /// If that fails, uses the compositioning function of the `fallback_allocator_type`.
            /// \requires The `fallback_allocator_type` msut be composable.
            FOONATHAN_ENABLE_IF(fallback_composable::value)
            void* try_allocate_node(std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                auto ptr = default_composable_traits::try_allocate_node(get_default_allocator(),
                                                                        size, alignment);
                if (!ptr)
                    ptr = fallback_composable_traits::try_allocate_node(get_fallback_allocator(),
                                                                        size, alignment);
                return ptr;
            }

            FOONATHAN_ENABLE_IF(fallback_composable::value)
            void* allocate_array(std::size_t count, std::size_t size,
                                 std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                auto ptr = default_composable_traits::try_allocate_array(get_default_allocator(),
                                                                         count, size, alignment);
                if (!ptr)
                    ptr = fallback_composable_traits::try_allocate_array(get_fallback_allocator(),
                                                                         count, size, alignment);
                return ptr;
            }

            FOONATHAN_ENABLE_IF(fallback_composable::value)
            bool try_deallocate_node(void* ptr, std::size_t size,
                                     std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                auto res = default_composable_traits::try_deallocate_node(get_default_allocator(),
                                                                          ptr, size, alignment);
                if (!res)
                    res = fallback_composable_traits::try_deallocate_node(get_fallback_allocator(),
                                                                          ptr, size, alignment);
                return res;
            }

            FOONATHAN_ENABLE_IF(fallback_composable::value)
            bool try_deallocate_array(void* ptr, std::size_t count, std::size_t size,
                                      std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                auto res =
                    default_composable_traits::try_deallocate_array(get_default_allocator(), ptr,
                                                                    count, size, alignment);
                if (!res)
                    res = fallback_composable_traits::try_deallocate_array(get_fallback_allocator(),
                                                                           ptr, count, size,
                                                                           alignment);
                return res;
            }
            /// @}

            /// @{
            /// \returns The maximum of the two values from both allocators.
            std::size_t max_node_size() const
            {
                auto def      = default_traits::max_node_size(get_default_allocator());
                auto fallback = fallback_traits::max_node_size(get_fallback_allocator());
                return fallback > def ? fallback : def;
            }

            std::size_t max_array_size() const
            {
                auto def      = default_traits::max_array_size(get_default_allocator());
                auto fallback = fallback_traits::max_array_size(get_fallback_allocator());
                return fallback > def ? fallback : def;
            }

            std::size_t max_alignment() const
            {
                auto def      = default_traits::max_alignment(get_default_allocator());
                auto fallback = fallback_traits::max_alignment(get_fallback_allocator());
                return fallback > def ? fallback : def;
            }
            /// @}

            /// @{
            /// \returns A (`const`) reference to the default allocator.
            default_allocator_type& get_default_allocator() FOONATHAN_NOEXCEPT
            {
                return detail::ebo_storage<0, default_allocator_type>::get();
            }

            const default_allocator_type& get_default_allocator() const FOONATHAN_NOEXCEPT
            {
                return detail::ebo_storage<0, default_allocator_type>::get();
            }
            /// @}

            /// @{
            /// \returns A (`const`) reference to the fallback allocator.
            fallback_allocator_type& get_fallback_allocator() FOONATHAN_NOEXCEPT
            {
                return detail::ebo_storage<1, fallback_allocator_type>::get();
            }

            const fallback_allocator_type& get_fallback_allocator() const FOONATHAN_NOEXCEPT
            {
                return detail::ebo_storage<1, fallback_allocator_type>::get();
            }
            /// @}
        };
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_FALLBACK_ALLOCATOR_HPP_INCLUDED
