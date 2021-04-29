// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "segregator.hpp"

#include <doctest/doctest.h>

#include "test_allocator.hpp"

using namespace foonathan::memory;

TEST_CASE("threshold_segregatable")
{
    using segregatable = threshold_segregatable<test_allocator>;
    segregatable s(8u);

    REQUIRE(s.use_allocate_node(1u, 1u));
    REQUIRE(s.use_allocate_node(8u, 1u));
    REQUIRE(s.use_allocate_node(8u, 100u));
    REQUIRE(!s.use_allocate_node(9u, 1u));
    REQUIRE(!s.use_allocate_node(9u, 100u));

    REQUIRE(s.use_allocate_array(1u, 1u, 1u));
    REQUIRE(s.use_allocate_array(1u, 8u, 1u));
    REQUIRE(s.use_allocate_array(2u, 4u, 1u));
    REQUIRE(!s.use_allocate_array(2u, 8u, 1u));
    REQUIRE(!s.use_allocate_array(1u, 9u, 1u));
}

TEST_CASE("binary_segregator")
{
    using segregatable = threshold_segregatable<test_allocator>;
    using segregator   = binary_segregator<segregatable, test_allocator>;

    segregator s(threshold(8u, test_allocator{}));
    REQUIRE(s.get_segregatable_allocator().no_allocated() == 0u);
    REQUIRE(s.get_fallback_allocator().no_allocated() == 0u);
    REQUIRE(s.get_segregatable_allocator().no_deallocated() == 0u);
    REQUIRE(s.get_fallback_allocator().no_deallocated() == 0u);

    auto ptr = s.allocate_node(1u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_allocated() == 1u);
    REQUIRE(s.get_fallback_allocator().no_allocated() == 0u);
    s.deallocate_node(ptr, 1u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_deallocated() == 1u);
    REQUIRE(s.get_fallback_allocator().no_deallocated() == 0u);

    ptr = s.allocate_node(8u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_allocated() == 1u);
    REQUIRE(s.get_fallback_allocator().no_allocated() == 0u);
    s.deallocate_node(ptr, 8u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_deallocated() == 2u);
    REQUIRE(s.get_fallback_allocator().no_deallocated() == 0u);

    ptr = s.allocate_node(8u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_allocated() == 1u);
    REQUIRE(s.get_fallback_allocator().no_allocated() == 0u);
    s.deallocate_node(ptr, 8u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_deallocated() == 3u);
    REQUIRE(s.get_fallback_allocator().no_deallocated() == 0u);

    ptr = s.allocate_node(9u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_allocated() == 0u);
    REQUIRE(s.get_fallback_allocator().no_allocated() == 1u);
    s.deallocate_node(ptr, 9u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_deallocated() == 3u);
    REQUIRE(s.get_fallback_allocator().no_deallocated() == 1u);

    ptr = s.allocate_array(1u, 8u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_allocated() == 1u);
    REQUIRE(s.get_fallback_allocator().no_allocated() == 0u);
    s.deallocate_array(ptr, 1u, 8u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_deallocated() == 4u);
    REQUIRE(s.get_fallback_allocator().no_deallocated() == 1u);

    ptr = s.allocate_array(2u, 8u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_allocated() == 0u);
    REQUIRE(s.get_fallback_allocator().no_allocated() == 1u);
    s.deallocate_array(ptr, 2u, 8u, 1u);
    REQUIRE(s.get_segregatable_allocator().no_deallocated() == 4u);
    REQUIRE(s.get_fallback_allocator().no_deallocated() == 2u);
}

TEST_CASE("segregator")
{
    using segregatable = threshold_segregatable<test_allocator>;
    using segregator_0 = segregator<segregatable>;
    using segregator_1 = segregator<segregatable, test_allocator>;
    using segregator_2 = segregator<segregatable, segregatable, test_allocator>;
    using segregator_3 = segregator<segregatable, segregatable, segregatable, test_allocator>;

    static_assert(std::is_same<segregator_0,
                               binary_segregator<segregatable, null_allocator>>::value,
                  "");
    static_assert(std::is_same<segregator_1,
                               binary_segregator<segregatable, test_allocator>>::value,
                  "");
    static_assert(std::is_same<segregator_2, binary_segregator<segregatable, segregator_1>>::value,
                  "");
    static_assert(std::is_same<segregator_3, binary_segregator<segregatable, segregator_2>>::value,
                  "");

    static_assert(segregator_size<segregator_0>::value == 1, "");
    static_assert(segregator_size<segregator_1>::value == 1, "");
    static_assert(segregator_size<segregator_2>::value == 2, "");
    static_assert(segregator_size<segregator_3>::value == 3, "");

    static_assert(std::is_same<segregatable_allocator_type<0, segregator_3>, test_allocator>::value,
                  "");
    static_assert(std::is_same<segregatable_allocator_type<1, segregator_3>, test_allocator>::value,
                  "");
    static_assert(std::is_same<segregatable_allocator_type<2, segregator_3>, test_allocator>::value,
                  "");

    static_assert(std::is_same<fallback_allocator_type<segregator_3>, test_allocator>::value, "");

    auto s = make_segregator(threshold(4, test_allocator{}), threshold(8, test_allocator{}),
                             threshold(16, test_allocator{}), test_allocator{});
    REQUIRE(get_segregatable_allocator<0>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<1>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<2>(s).no_allocated() == 0u);
    REQUIRE(get_fallback_allocator(s).no_allocated() == 0u);

    auto ptr = s.allocate_node(2, 1);
    REQUIRE(get_segregatable_allocator<0>(s).no_allocated() == 1u);
    REQUIRE(get_segregatable_allocator<1>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<2>(s).no_allocated() == 0u);
    REQUIRE(get_fallback_allocator(s).no_allocated() == 0u);
    s.deallocate_node(ptr, 2, 1);

    ptr = s.allocate_node(4, 1);
    REQUIRE(get_segregatable_allocator<0>(s).no_allocated() == 1u);
    REQUIRE(get_segregatable_allocator<1>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<2>(s).no_allocated() == 0u);
    REQUIRE(get_fallback_allocator(s).no_allocated() == 0u);
    s.deallocate_node(ptr, 4, 1);

    ptr = s.allocate_node(5, 1);
    REQUIRE(get_segregatable_allocator<0>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<1>(s).no_allocated() == 1u);
    REQUIRE(get_segregatable_allocator<2>(s).no_allocated() == 0u);
    REQUIRE(get_fallback_allocator(s).no_allocated() == 0u);
    s.deallocate_node(ptr, 5, 1);

    ptr = s.allocate_node(8, 1);
    REQUIRE(get_segregatable_allocator<0>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<1>(s).no_allocated() == 1u);
    REQUIRE(get_segregatable_allocator<2>(s).no_allocated() == 0u);
    REQUIRE(get_fallback_allocator(s).no_allocated() == 0u);
    s.deallocate_node(ptr, 8, 1);

    ptr = s.allocate_node(9, 1);
    REQUIRE(get_segregatable_allocator<0>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<1>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<2>(s).no_allocated() == 1u);
    REQUIRE(get_fallback_allocator(s).no_allocated() == 0u);
    s.deallocate_node(ptr, 9, 1);

    ptr = s.allocate_node(16, 1);
    REQUIRE(get_segregatable_allocator<0>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<1>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<2>(s).no_allocated() == 1u);
    REQUIRE(get_fallback_allocator(s).no_allocated() == 0u);
    s.deallocate_node(ptr, 16, 1);

    ptr = s.allocate_node(17, 1);
    REQUIRE(get_segregatable_allocator<0>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<1>(s).no_allocated() == 0u);
    REQUIRE(get_segregatable_allocator<2>(s).no_allocated() == 0u);
    REQUIRE(get_fallback_allocator(s).no_allocated() == 1u);
    s.deallocate_node(ptr, 17, 1);
}
