// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "detail/align.hpp"

#include <doctest/doctest.h>

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::round_up_to_multiple_of_alignment")
{
    REQUIRE(round_up_to_multiple_of_alignment(0, 1) == 0);
    REQUIRE(round_up_to_multiple_of_alignment(1, 1) == 1);
    REQUIRE(round_up_to_multiple_of_alignment(2, 1) == 2);
    REQUIRE(round_up_to_multiple_of_alignment(3, 1) == 3);
    REQUIRE(round_up_to_multiple_of_alignment(4, 1) == 4);

    REQUIRE(round_up_to_multiple_of_alignment(0, 2) == 0);
    REQUIRE(round_up_to_multiple_of_alignment(1, 2) == 2);
    REQUIRE(round_up_to_multiple_of_alignment(2, 2) == 2);
    REQUIRE(round_up_to_multiple_of_alignment(3, 2) == 4);
    REQUIRE(round_up_to_multiple_of_alignment(4, 2) == 4);
}

TEST_CASE("detail::align_offset")
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

TEST_CASE("detail::is_aligned")
{
    auto ptr = reinterpret_cast<void*>(0);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(is_aligned(ptr, 8));
    REQUIRE(is_aligned(ptr, 16));
    ptr = reinterpret_cast<void*>(1);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(!is_aligned(ptr, 16));
    ptr = reinterpret_cast<void*>(8);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(is_aligned(ptr, 4));
    REQUIRE(is_aligned(ptr, 8));
    REQUIRE(!is_aligned(ptr, 16));
    ptr = reinterpret_cast<void*>(16);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(is_aligned(ptr, 8));
    REQUIRE(is_aligned(ptr, 16));
    ptr = reinterpret_cast<void*>(1025);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(!is_aligned(ptr, 16));
}

TEST_CASE("detail::alignment_for")
{
    static_assert(max_alignment >= 8, "test case not working");
    REQUIRE(alignment_for(1) == 1);
    REQUIRE(alignment_for(2) == 2);
    REQUIRE(alignment_for(3) == 1);
    REQUIRE(alignment_for(4) == 4);
    REQUIRE(alignment_for(5) == 1);
    REQUIRE(alignment_for(6) == 2);
    REQUIRE(alignment_for(7) == 1);
    REQUIRE(alignment_for(8) == 8);
    REQUIRE(alignment_for(9) == 1);
    REQUIRE(alignment_for(10) == 2);
    REQUIRE(alignment_for(12) == 4);
    REQUIRE(alignment_for(13) == 1);
    REQUIRE(alignment_for(14) == 2);
    REQUIRE(alignment_for(15) == 1);
    REQUIRE(alignment_for(16) == max_alignment);
    REQUIRE(alignment_for(1024) == max_alignment);
}
