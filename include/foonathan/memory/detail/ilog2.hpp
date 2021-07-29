// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_ILOG2_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_ILOG2_HPP_INCLUDED

#include <climits>
#include <cstdint>

#include "config.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            // undefined for 0
            template <typename UInt>
            constexpr bool is_power_of_two(UInt x)
            {
                return (x & (x - 1)) == 0;
            }

            inline std::size_t ilog2_base(std::uint64_t x)
            {
#if defined(__GNUC__)
                unsigned long long value = x;
                return sizeof(value) * CHAR_BIT - static_cast<unsigned>(__builtin_clzll(value));
#else
                // Adapted from https://stackoverflow.com/a/40943402
                std::size_t clz = 64;
                std::size_t c   = 32;
                do
                {
                    auto tmp = x >> c;
                    if (tmp != 0)
                    {
                        clz -= c;
                        x = tmp;
                    }
                    c = c >> 1;
                } while (c != 0);
                clz -= x ? 1 : 0;

                return 64 - clz;
#endif
            }

            // ilog2() implementation, cuts part after the comma
            // e.g. 1 -> 0, 2 -> 1, 3 -> 1, 4 -> 2, 5 -> 2
            inline std::size_t ilog2(std::uint64_t x)
            {
                return ilog2_base(x) - 1;
            }

            // ceiling ilog2() implementation, adds one if part after comma
            // e.g. 1 -> 0, 2 -> 1, 3 -> 2, 4 -> 2, 5 -> 3
            inline std::size_t ilog2_ceil(std::uint64_t x)
            {
                // only subtract one if power of two
                return ilog2_base(x) - std::size_t(is_power_of_two(x));
            }
        } // namespace detail
    }     // namespace memory
} // namespace foonathan

#endif
