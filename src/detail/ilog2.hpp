// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_SRC_DETAIL_ILOG2_HPP_INCLUDED
#define FOONATHAN_MEMORY_SRC_DETAIL_ILOG2_HPP_INCLUDED

#include "config.hpp"

#include <foonathan/clz.hpp>

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

        FOONATHAN_CONSTEXPR_FNC std::size_t ilog2_base(std::uint64_t x)
        {
            return sizeof(x) * CHAR_BIT - foonathan_comp::clz(x);
        }

        // ilog2() implementation, cuts part after the comma
        // e.g. 1 -> 0, 2 -> 1, 3 -> 1, 4 -> 2, 5 -> 2
        FOONATHAN_CONSTEXPR_FNC std::size_t ilog2(std::size_t x)
        {
            return ilog2_base(x) - 1;
        }

        // ceiling ilog2() implementation, adds one if part after comma
        // e.g. 1 -> 0, 2 -> 1, 3 -> 2, 4 -> 2, 5 -> 3
        FOONATHAN_CONSTEXPR_FNC std::size_t ilog2_ceil(std::size_t x)
        {
            // only subtract one if power of two
            return ilog2_base(x) - std::size_t(is_power_of_two(x));
        }
    }
}} // namespace foonathan::memory

#endif
