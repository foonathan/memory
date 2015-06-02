// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/align.hpp"

#include <catch.hpp>

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::align_offset", "[detail][core]")
{
    auto ptr = reinterpret_cast<void*>(0);
    REQUIRE(align_offset(ptr, 1) == 0u);
    REQUIRE(align_offset(ptr, 16) == 0u);
    ptr = reinterpret_cast<void*>(1);
    REQUIRE(align_offset(ptr, 1) == 0u);
    REQUIRE(align_offset(ptr, 16) == 15u);
    ptr = reinterpret_cast<void*>(8);
    REQUIRE(align_offset(ptr, 4) == 0u);
    REQUIRE(align_offset(ptr, 8) == 0u);
    REQUIRE(align_offset(ptr, 16) == 8u);
    ptr = reinterpret_cast<void*>(16);
    REQUIRE(align_offset(ptr, 16) == 0u);
    ptr = reinterpret_cast<void*>(1025);
    REQUIRE(align_offset(ptr, 16) == 15u);
}
