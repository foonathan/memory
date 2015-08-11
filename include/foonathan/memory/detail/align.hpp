// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED

#include <cstddef>
#include <cstdint>

#include "../config.hpp"
#include "../error.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // whether or not an alignment is valid, i.e. a power of two not zero
        FOONATHAN_CONSTEXPR_FNC bool is_valid_alignment(std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            return alignment && (alignment & (alignment - 1)) == 0u;
        }

        // returns the offset needed to align ptr for given alignment
        // alignment must be valid
        inline std::size_t align_offset(void *ptr, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(is_valid_alignment(alignment));
            auto address = reinterpret_cast<std::uintptr_t>(ptr);
            auto misaligned = address & (alignment - 1);
            return misaligned != 0 ? (alignment - misaligned) : 0;
        }

        // whether or not the pointer is aligned for given alignment
        // alignment must be valid
        inline bool is_aligned(void *ptr, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(is_valid_alignment(alignment));
            auto address = reinterpret_cast<std::uintptr_t>(ptr);
            return address % alignment == 0u;
        }

        // maximum alignment value
        FOONATHAN_CONSTEXPR std::size_t max_alignment = FOONATHAN_ALIGNOF(foonathan_memory_comp::max_align_t);
        static_assert(is_valid_alignment(max_alignment), "ehm..?");

        // returns the minimum alignment required for a node of given size
        inline std::size_t alignment_for(std::size_t size) FOONATHAN_NOEXCEPT
        {
            if (size >= max_alignment)
                return max_alignment;
            // otherwise use the next power of two
            // I'm lazy, assume max_alignment won't be bigger than 16
            static_assert(detail::max_alignment <= 16u, "I am sorry, lookup table doesn't got that far :(");
            // maps size to next bigger power of two
            static FOONATHAN_CONSTEXPR std::size_t next_power_of_two[] = {0, 1, 2, 2, 4, 4, 4, 4,
                                                                          8, 8, 8, 8, 8, 8, 8, 8};
            return next_power_of_two[size];
        }
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
