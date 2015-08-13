// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_ALIGNED_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALIGNED_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief An allocator ensuring a certain alignment.

#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "config.hpp"
#include "error.hpp"

namespace foonathan { namespace memory
{
    /// \brief A \c RawAllocator adapter that ensures a minimum alignment.
    /// \details It changes the alignment requirements passed to the allocation function if necessary
    /// and forwards to the wrapped allocator.
    /// \ingroup memory
    template <class RawAllocator>
    class aligned_allocator
    : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
    {
        using traits = allocator_traits<RawAllocator>;
    public:
        using raw_allocator = typename allocator_traits<RawAllocator>::allocator_type;
        using is_stateful = std::true_type;

        /// \brief Creates it passing it the minimum alignment requirement.
        /// \details It must be less than the maximum supported alignment.
        explicit aligned_allocator(std::size_t min_alignment, raw_allocator &&alloc = {})
        : raw_allocator(detail::move(alloc)), min_alignment_(min_alignment)
        {
            FOONATHAN_MEMORY_ASSERT(min_alignment_ <= max_alignment());
        }

        /// @{
        /// \brief (De-)Allocation functions ensure the given minimum alignemnt.
        /// \details If the alignment requirement is higher, it is unchanged.
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

        void deallocate_node(void *ptr, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            if (min_alignment_ > alignment)
                alignment = min_alignment_;
            traits::deallocate_node(get_allocator(), ptr, size, alignment);
        }

        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            if (min_alignment_ > alignment)
                alignment = min_alignment_;
            traits::deallocate_array(get_allocator(), ptr, count, size, alignment);
        }
        /// @}

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

        /// @{
        /// \brief Returns a reference to the actual allocator.
        raw_allocator& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const raw_allocator& get_allocator() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }
        /// @}

        /// @{
        /// \brief Get/set the minimum alignment.
        std::size_t min_alignment() const FOONATHAN_NOEXCEPT
        {
            return min_alignment_;
        }

        void set_min_alignment(std::size_t min_alignment)
        {
            FOONATHAN_MEMORY_ASSERT(min_alignment <= max_alignment());
            min_alignment_ = min_alignment;
        }
        /// @}

    private:
        std::size_t min_alignment_;
    };

    /// \brief Creates an \ref aligned_allocator.
    /// \relates aligned_allocator
    template <class RawAllocator>
    auto make_aligned_allocator(std::size_t min_alignment, RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> aligned_allocator<typename std::decay<RawAllocator>::type>
    {
        return aligned_allocator<typename std::decay<RawAllocator>::type>
                {min_alignment, detail::forward<RawAllocator>(allocator)};
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALIGNED_ALLOCATOR_HPP_INCLUDED
