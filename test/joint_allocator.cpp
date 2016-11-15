// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "joint_allocator.hpp"

#include <catch.hpp>

#include "test_allocator.hpp"

using namespace foonathan::memory;

TEST_CASE("joint_ptr", "[allocator]")
{
    struct joint_test : joint_type<joint_test>
    {
        int value;

        joint_test(int v) : value(v)
        {
        }
    };

    test_allocator alloc;

    SECTION("allocator constructor")
    {
        joint_ptr<joint_test, test_allocator> ptr(alloc);
        REQUIRE(!ptr);
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(&ptr.get_allocator() == &alloc);

        REQUIRE(alloc.no_allocated() == 0u);
        REQUIRE(alloc.no_deallocated() == 0u);
    }
    SECTION("creation constructor")
    {
        joint_ptr<joint_test, test_allocator> ptr(alloc, 10u, )
    }
}
