// Copyright (C) 2015-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/debug_helpers.hpp"

#include <catch.hpp>

#include "debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::debug_fill", "[detail][core]")
{
    debug_magic array[10];
    for (auto& el : array)
        el = debug_magic::freed_memory;

    debug_fill(array, sizeof(array), debug_magic::new_memory);
#if FOONATHAN_MEMORY_DEBUG_FILL
    for (auto el : array)
        REQUIRE(el == debug_magic::new_memory);
#else
    for (auto el : array)
        REQUIRE(el == debug_magic::freed_memory);
#endif
}

TEST_CASE("detail::debug_is_filled", "[detail][core]")
{
    debug_magic array[10];
    for (auto& el : array)
        el = debug_magic::freed_memory;

    REQUIRE(debug_is_filled(array, sizeof(array), debug_magic::freed_memory) == nullptr);

    array[5] = debug_magic::new_memory;
    auto ptr =
        static_cast<debug_magic*>(debug_is_filled(array, sizeof(array), debug_magic::freed_memory));
#if FOONATHAN_MEMORY_DEBUG_FILL
    REQUIRE(ptr == array + 5);
#else
    REQUIRE(ptr == nullptr);
#endif
}

TEST_CASE("detail::debug_fill_new/free", "[detail][core]")
{
    debug_magic array[10];

    auto result          = debug_fill_new(array, 8 * sizeof(debug_magic), sizeof(debug_magic));
    auto offset          = static_cast<debug_magic*>(result) - array;
    auto expected_offset = debug_fence_size ? sizeof(debug_magic) : 0u;
    REQUIRE(offset == expected_offset);

#if FOONATHAN_MEMORY_DEBUG_FILL
#if FOONATHAN_MEMORY_DEBUG_FENCE
    REQUIRE(array[0] == debug_magic::fence_memory);
    REQUIRE(array[9] == debug_magic::fence_memory);
    const auto start = 1;
#else
    const auto start = 0;
#endif
    for (auto i = start; i < start + 8; ++i)
        REQUIRE(array[i] == debug_magic::new_memory);
#endif

    result = debug_fill_free(result, 8 * sizeof(debug_magic), sizeof(debug_magic));
    REQUIRE(static_cast<debug_magic*>(result) == array);

#if FOONATHAN_MEMORY_DEBUG_FILL
#if FOONATHAN_MEMORY_DEBUG_FENCE
    REQUIRE(array[0] == debug_magic::fence_memory);
    REQUIRE(array[9] == debug_magic::fence_memory);
#endif
    for (auto i = start; i < start + 8; ++i)
        REQUIRE(array[i] == debug_magic::freed_memory);
#endif
}
