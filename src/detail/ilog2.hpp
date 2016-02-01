// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_SRC_DETAIL_ILOG2_HPP_INCLUDED
#define FOONATHAN_MEMORY_SRC_DETAIL_ILOG2_HPP_INCLUDED

#include <cstdint>

#include "config.hpp"

#include FOONATHAN_CLZ_HEADER

namespace foonathan { namespace memory
{
    namespace detail
    {
        // undefined for 0
        template <typename UInt>
        FOONATHAN_CONSTEXPR_FNC bool is_power_of_two(UInt x)
        {
            return (x & (x - 1)) == 0;
        }

        // ilog2() implementation, cuts part after the comma
        // e.g. 1 -> 0, 2 -> 1, 3 -> 1, 4 -> 2, 5 -> 2
        template <typename UInt>
        FOONATHAN_CONSTEXPR_FNC UInt ilog2(UInt x)
        {
            return sizeof(x) * CHAR_BIT - foonathan_comp::clz(x) - 1;
        }

        // ceiling ilog2() implementation, adds one if part after comma
        // e.g. 1 -> 0, 2 -> 1, 3 -> 2, 4 -> 2, 5 -> 3
        template <typename UInt>
        FOONATHAN_CONSTEXPR_FNC UInt ilog2_ceil(UInt x)
        {
            // only subtract one if power of two
            return sizeof(x) * CHAR_BIT - foonathan_comp::clz(x) - UInt(is_power_of_two(x));
        }
    }
}} // namespace foonathan::memory

#endif
