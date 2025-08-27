// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

// tests all possible default allocator classes

#include "virtual_memory.hpp"

#include <doctest/doctest.h>

namespace
{

    using namespace foonathan::memory;

    static void* pages = virtual_memory_reserve(10);

    TEST_CASE("can use virtual_memory during static initialization")
    {
        REQUIRE(pages != nullptr);

        virtual_memory_release(pages, 10);
        pages = nullptr;

#if defined(FOONATHAN_MEMORY_ALLOW_VIRTUAL_MEMORY_PAGE_SIZE)
        CHECK(virtual_memory_page_size == get_virtual_memory_page_size());
#endif
    }

} // namespace
