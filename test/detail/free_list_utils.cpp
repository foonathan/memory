// Copyright (C) 2015-2026 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "detail/free_list_utils.hpp"

#include <doctest/doctest.h>

using namespace foonathan::memory;

TEST_CASE("detail::pointer comparisons")
{
    char storage[3]{};
    for (unsigned i = 0; i != 3; ++i)
        for (unsigned j = 0; j != 3; ++j)
        {
            void* a = &storage[i];
            void* b = &storage[j];
            CHECK(detail::less(a, b) == (i < j));
            CHECK(detail::less_equal(a, b) == (i <= j));
            CHECK(detail::greater(a, b) == (i > j));
            CHECK(detail::greater_equal(a, b) == (i >= j));
        }
}
