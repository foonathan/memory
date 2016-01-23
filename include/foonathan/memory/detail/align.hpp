// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include FOONATHAN_ALIGNAS_HEADER
#include FOONATHAN_ALIGNOF_HEADER
#include FOONATHAN_MAX_ALIGN_T_HEADER

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

#define FOONATHAN_DETAIL_LOG2TABLE_16(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
#define FOONATHAN_DETAIL_LOG2TABLE_32(n) FOONATHAN_DETAIL_LOG2TABLE_16(n), FOONATHAN_DETAIL_LOG2TABLE_16(n)
#define FOONATHAN_DETAIL_LOG2TABLE_64(n) FOONATHAN_DETAIL_LOG2TABLE_32(n), FOONATHAN_DETAIL_LOG2TABLE_32(n)
#define FOONATHAN_DETAIL_LOG2TABLE_128(n) FOONATHAN_DETAIL_LOG2TABLE_64(n), FOONATHAN_DETAIL_LOG2TABLE_64(n)

        namespace
        {
            FOONATHAN_CONSTEXPR std::int16_t log2table[] = { -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                                                             FOONATHAN_DETAIL_LOG2TABLE_16(4),
                                                             FOONATHAN_DETAIL_LOG2TABLE_32(5),
                                                             FOONATHAN_DETAIL_LOG2TABLE_64(6),
                                                             FOONATHAN_DETAIL_LOG2TABLE_128(7) };

            // All this static_if stuff is just to avoid UB by checking std::size_t width
			// before shifting.
            using size_t32 = std::true_type;
            using size_t64 = std::false_type;
            using size_tcond = std::integral_constant<bool, (sizeof(std::size_t) <= 4)>;

            template<typename Cond>
            struct log2_static_if
            {
                static FOONATHAN_CONSTEXPR_FNC std::int16_t apply(std::uint32_t value)
                {
                    return (value >> 24) ?
                            24 + log2table[value >> 24]
                    : ((value >> 16) ?
                            16 + log2table[value >> 16]
                    : ((value >> 8) ?
                            8 + log2table[value >> 8]
                    : log2table[value]
                    ));
                }
            };

            template<>
            struct log2_static_if<size_t64>
            {
                static FOONATHAN_CONSTEXPR_FNC std::int16_t apply(std::uint64_t value)
                {
                    return (value >> 56) ?
                        56 + log2table[value >> 56]
                    : ((value >> 48) ?
                        48 + log2table[value >> 48]
                    : ((value >> 40) ?
                        40 + log2table[value >> 40]
                    : ((value >> 32) ?
                        32 + log2table[value >> 32]
                    : log2_static_if<size_t32>::apply(static_cast<std::uint32_t>(value))
                    )));
                }
            };

            FOONATHAN_CONSTEXPR_FNC std::size_t log2(std::size_t value)
            {
                return log2_static_if<size_tcond>::apply(value);
            }
        }

        FOONATHAN_CONSTEXPR_FNC std::size_t lastest_pow2(std::size_t value)
        {
            return 1 << (log2(value));
        }

        // maximum alignment value
        FOONATHAN_CONSTEXPR std::size_t max_alignment = FOONATHAN_ALIGNOF(foonathan_comp::max_align_t);
#if FOONATHAN_HAS_CONSTEXPR
        static_assert(is_valid_alignment(max_alignment), "ehm..?");
#endif

        // returns the minimum alignment required for a node of given size
        inline std::size_t alignment_for(std::size_t size) FOONATHAN_NOEXCEPT
        {
            if (size >= max_alignment)
                return max_alignment;

            return lastest_pow2(size);
        }
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
