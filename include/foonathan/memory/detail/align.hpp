// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED

#include <cstdint>

#include "../config.hpp"
#include "assert.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            // whether or not an alignment is valid, i.e. a power of two not zero
            constexpr bool is_valid_alignment(std::size_t alignment) noexcept
            {
                return alignment && (alignment & (alignment - 1)) == 0u;
            }

            // returns the offset needed to align ptr for given alignment
            // alignment must be valid
            inline std::size_t align_offset(std::uintptr_t address, std::size_t alignment) noexcept
            {
                FOONATHAN_MEMORY_ASSERT(is_valid_alignment(alignment));
                auto misaligned = address & (alignment - 1);
                return misaligned != 0 ? (alignment - misaligned) : 0;
            }
            inline std::size_t align_offset(void* ptr, std::size_t alignment) noexcept
            {
                return align_offset(reinterpret_cast<std::uintptr_t>(ptr), alignment);
            }

            // whether or not the pointer is aligned for given alignment
            // alignment must be valid
            bool is_aligned(void* ptr, std::size_t alignment) noexcept;

            // maximum alignment value
            constexpr std::size_t max_alignment = alignof(std::max_align_t);
            static_assert(is_valid_alignment(max_alignment), "ehm..?");

            // returns the minimum alignment required for a node of given size
            std::size_t alignment_for(std::size_t size) noexcept;
        } // namespace detail
    }     // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
