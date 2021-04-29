// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/ilog2.hpp"

#include <doctest/doctest.h>

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::ilog2()")
{
    // Check everything up to 2^16.
    for (std::size_t i = 0; i != 16; ++i)
    {
        auto power      = 1u << i;
        auto next_power = 2 * power;
        for (auto x = power; x != next_power; ++x)
            CHECK(ilog2(x) == i);
    }

    // Check some higher values.
    CHECK(ilog2(std::uint64_t(1) << 32) == 32);
    CHECK(ilog2((std::uint64_t(1) << 32) + 44) == 32);
    CHECK(ilog2((std::uint64_t(1) << 32) + 2048) == 32);

    CHECK(ilog2(std::uint64_t(1) << 48) == 48);
    CHECK(ilog2((std::uint64_t(1) << 48) + 44) == 48);
    CHECK(ilog2((std::uint64_t(1) << 48) + 2048) == 48);

    CHECK(ilog2(std::uint64_t(1) << 63) == 63);
    CHECK(ilog2((std::uint64_t(1) << 63) + 44) == 63);
    CHECK(ilog2((std::uint64_t(1) << 63) + 2063) == 63);
}

TEST_CASE("detail::ilog2_ceil()")
{
    // Check everything up to 2^16.
    for (std::size_t i = 0; i != 16; ++i)
    {
        auto power = 1u << i;
        CHECK(ilog2_ceil(power) == i);

        auto next_power = 2 * power;
        for (auto x = power + 1; x != next_power; ++x)
            CHECK(ilog2_ceil(x) == i + 1);
    }

    // Check some higher values.
    CHECK(ilog2_ceil(std::uint64_t(1) << 32) == 32);
    CHECK(ilog2_ceil((std::uint64_t(1) << 32) + 44) == 33);
    CHECK(ilog2_ceil((std::uint64_t(1) << 32) + 2048) == 33);

    CHECK(ilog2_ceil(std::uint64_t(1) << 48) == 48);
    CHECK(ilog2_ceil((std::uint64_t(1) << 48) + 44) == 49);
    CHECK(ilog2_ceil((std::uint64_t(1) << 48) + 2048) == 49);

    CHECK(ilog2_ceil(std::uint64_t(1) << 63) == 63);
    CHECK(ilog2_ceil((std::uint64_t(1) << 63) + 44) == 64);
    CHECK(ilog2_ceil((std::uint64_t(1) << 63) + 2063) == 64);
}

