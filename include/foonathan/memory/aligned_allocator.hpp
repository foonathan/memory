// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_ALIGNED_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALIGNED_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::aligned_allocator and related functions.

#include <type_traits>

#include "detail/assert.hpp"
#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "config.hpp"

namespace foonathan
{
    namespace memory
    {
        /// A \concept{concept_rawallocator,RawAllocator} adapter that ensures a minimum alignment.
        /// It adjusts the alignment value so that it is always larger than the minimum and forwards to the specified allocator.
        /// \ingroup adapter
        template <class RawAllocator>
        class aligned_allocator : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
        {
            using traits            = allocator_traits<RawAllocator>;
            using composable_traits = composable_allocator_traits<RawAllocator>;
            using composable        = is_composable_allocator<typename traits::allocator_type>;

        public:
            using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;
            using is_stateful    = std::true_type;

            /// \effects Creates it passing it the minimum alignment value and the allocator object.
            /// \requires \c min_alignment must be less than \c this->max_alignment().
            explicit aligned_allocator(std::size_t min_alignment, allocator_type&& alloc = {})
            : allocator_type(detail::move(alloc)), min_alignment_(min_alignment)
            {
                FOONATHAN_MEMORY_ASSERT(min_alignment_ <= max_alignment());
            }

            /// @{
            /// \effects Moves the \c aligned_allocator object.
            /// It simply moves the underlying allocator.
            aligned_allocator(aligned_allocator&& other) noexcept
            : allocator_type(detail::move(other)), min_alignment_(other.min_alignment_)
            {
            }

            aligned_allocator& operator=(aligned_allocator&& other) noexcept
            {
                allocator_type::operator=(detail::move(other));
                min_alignment_          = other.min_alignment_;
                return *this;
            }
            /// @}

            /// @{
            /// \effects Forwards to the underlying allocator through the \ref allocator_traits.
            /// If the \c alignment is less than the \c min_alignment(), it is set to the minimum alignment.
            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                if (min_alignment_ > alignment)
                    alignment = min_alignment_;
                return traits::allocate_node(get_allocator(), size, alignment);
            }

            void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
            {
                if (min_alignment_ > alignment)
                    alignment = min_alignment_;
                return traits::allocate_array(get_allocator(), count, size, alignment);
            }

            void deallocate_node(void* ptr, std::size_t size, std::size_t alignment) noexcept
            {
                if (min_alignment_ > alignment)
                    alignment = min_alignment_;
                traits::deallocate_node(get_allocator(), ptr, size, alignment);
            }

            void deallocate_array(void* ptr, std::size_t count, std::size_t size,
                                  std::size_t alignment) noexcept
            {
                if (min_alignment_ > alignment)
                    alignment = min_alignment_;
                traits::deallocate_array(get_allocator(), ptr, count, size, alignment);
            }
            /// @}

            /// @{
            /// \effects Forwards to the underlying allocator through the \ref composable_allocator_traits.
            /// If the \c alignment is less than the \c min_alignment(), it is set to the minimum alignment.
            /// \requires The underyling allocator must be composable.
            FOONATHAN_ENABLE_IF(composable::value)
            void* try_allocate_node(std::size_t size, std::size_t alignment) noexcept
            {
                if (min_alignment_ > alignment)
                    alignment = min_alignment_;
                return composable_traits::try_allocate_node(get_allocator(), size, alignment);
            }

            FOONATHAN_ENABLE_IF(composable::value)
            void* try_allocate_array(std::size_t count, std::size_t size,
                                     std::size_t alignment) noexcept
            {
                if (min_alignment_ > alignment)
                    alignment = min_alignment_;
                return composable_traits::try_allocate_array(get_allocator(), count, size,
                                                             alignment);
            }

            FOONATHAN_ENABLE_IF(composable::value)
            bool try_deallocate_node(void* ptr, std::size_t size, std::size_t alignment) noexcept
            {
                if (min_alignment_ > alignment)
                    alignment = min_alignment_;
                return composable_traits::try_deallocate_node(get_allocator(), ptr, size,
                                                              alignment);
            }

            FOONATHAN_ENABLE_IF(composable::value)
            bool try_deallocate_array(void* ptr, std::size_t count, std::size_t size,
                                      std::size_t alignment) noexcept
            {
                if (min_alignment_ > alignment)
                    alignment = min_alignment_;
                return composable_traits::try_deallocate_array(get_allocator(), ptr, count, size,
                                                               alignment);
            }
            /// @}

            /// @{
            /// \returns The value returned by the \ref allocator_traits for the underlying allocator.
            std::size_t max_node_size() const
            {
                return traits::max_node_size(get_allocator());
            }

            std::size_t max_array_size() const
            {
                return traits::max_array_size(get_allocator());
            }

            std::size_t max_alignment() const
            {
                return traits::max_alignment(get_allocator());
            }
            /// @}

            /// @{
            /// \returns A reference to the underlying allocator.
            allocator_type& get_allocator() noexcept
            {
                return *this;
            }

            const allocator_type& get_allocator() const noexcept
            {
                return *this;
            }
            /// @}

            /// \returns The minimum alignment.
            std::size_t min_alignment() const noexcept
            {
                return min_alignment_;
            }

            /// \effects Sets the minimum alignment to a new value.
            /// \requires \c min_alignment must be less than \c this->max_alignment().
            void set_min_alignment(std::size_t min_alignment)
            {
                FOONATHAN_MEMORY_ASSERT(min_alignment <= max_alignment());
                min_alignment_ = min_alignment;
            }

        private:
            std::size_t min_alignment_;
        };

        /// \returns A new \ref aligned_allocator created by forwarding the parameters to the constructor.
        /// \relates aligned_allocator
        template <class RawAllocator>
        auto make_aligned_allocator(std::size_t min_alignment, RawAllocator&& allocator) noexcept
            -> aligned_allocator<typename std::decay<RawAllocator>::type>
        {
            return aligned_allocator<
                typename std::decay<RawAllocator>::type>{min_alignment,
                                                         detail::forward<RawAllocator>(allocator)};
        }
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_ALIGNED_ALLOCATOR_HPP_INCLUDED
