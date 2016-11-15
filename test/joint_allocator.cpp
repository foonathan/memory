// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "joint_allocator.hpp"

#include <catch.hpp>

#include "container.hpp"
#include "test_allocator.hpp"

using namespace foonathan::memory;

template <typename T, class RawAllocator, class Mutex>
void verify(const joint_ptr<T, RawAllocator, Mutex>& ptr, const RawAllocator& alloc, int value)
{
    REQUIRE(ptr);
    REQUIRE(ptr.get());
    REQUIRE(ptr.get() == ptr.operator->());
    REQUIRE(ptr.get() == &*ptr);
    REQUIRE(&ptr.get_allocator() == &alloc);
    REQUIRE(ptr->value == value);
}

template <typename T, class RawAllocator, class Mutex>
void verify_null(const joint_ptr<T, RawAllocator, Mutex>& ptr, const RawAllocator& alloc)
{
    REQUIRE(!ptr);
    REQUIRE(ptr.get() == nullptr);
    REQUIRE(&ptr.get_allocator() == &alloc);
}

TEST_CASE("joint_ptr", "[allocator]")
{
    struct joint_test : joint_type<joint_test>
    {
        int value;

        joint_test(joint tag, int v) : joint_type(tag), value(v)
        {
        }

        joint_test(joint tag, const joint_test& other) : joint_type(tag), value(other.value)
        {
        }
    };

    test_allocator alloc;

    SECTION("allocator constructor")
    {
        joint_ptr<joint_test, test_allocator> ptr(alloc);
        verify_null(ptr, alloc);

        REQUIRE(alloc.no_allocated() == 0u);
        REQUIRE(alloc.no_deallocated() == 0u);
    }
    SECTION("creation constructor")
    {
        joint_ptr<joint_test, test_allocator> ptr(alloc, joint_size(10u), 5);
        verify(ptr, alloc, 5);

        REQUIRE(alloc.no_allocated() == 1u);
        REQUIRE(alloc.last_allocated().size == sizeof(joint_test) + 10u);
        REQUIRE(alloc.last_allocated().alignment == FOONATHAN_ALIGNOF(joint_test));
        REQUIRE(alloc.no_deallocated() == 0u);
    }
    SECTION("move constructor")
    {
        auto ptr1 = allocate_joint<joint_test>(alloc, joint_size(10u), 5);
        verify(ptr1, alloc, 5);

        auto ptr2 = std::move(ptr1);
        verify_null(ptr1, alloc);
        verify(ptr2, alloc, 5);

        auto ptr3 = std::move(ptr1);
        verify_null(ptr1, alloc);

        REQUIRE(alloc.no_allocated() == 1u);
    }
    SECTION("move assignment")
    {
        joint_ptr<joint_test, test_allocator> ptr1(alloc);
        verify_null(ptr1, alloc);

        auto ptr2 = allocate_joint<joint_test>(alloc, joint_size(10u), 5);
        verify(ptr2, alloc, 5);
        REQUIRE(alloc.no_allocated() == 1u);

        ptr1 = std::move(ptr2);
        verify_null(ptr2, alloc);
        verify(ptr1, alloc, 5);
        REQUIRE(alloc.no_allocated() == 1u);

        ptr1 = std::move(ptr2);
        verify_null(ptr1, alloc);
        verify_null(ptr2, alloc);
        REQUIRE(alloc.no_allocated() == 0u);
    }
    SECTION("swap")
    {
        joint_ptr<joint_test, test_allocator> ptr1(alloc);
        verify_null(ptr1, alloc);

        auto ptr2 = allocate_joint<joint_test>(alloc, joint_size(10u), 5);
        verify(ptr2, alloc, 5);

        swap(ptr1, ptr2);
        verify(ptr1, alloc, 5);
        verify_null(ptr2, alloc);

        REQUIRE(alloc.no_allocated() == 1u);
    }
    SECTION("reset")
    {
        joint_ptr<joint_test, test_allocator> ptr1(alloc);
        verify_null(ptr1, alloc);
        ptr1.reset();
        verify_null(ptr1, alloc);

        auto ptr2 = allocate_joint<joint_test>(alloc, joint_size(10u), 5);
        verify(ptr2, alloc, 5);
        REQUIRE(alloc.no_allocated() == 1u);

        ptr2.reset();
        verify_null(ptr2, alloc);
        REQUIRE(alloc.no_allocated() == 0u);
    }
    SECTION("compare")
    {
        joint_ptr<joint_test, test_allocator> ptr1(alloc);
        verify_null(ptr1, alloc);

        auto ptr2 = allocate_joint<joint_test>(alloc, joint_size(10u), 5);
        verify(ptr2, alloc, 5);

        REQUIRE(ptr1 == nullptr);
        REQUIRE(nullptr == ptr1);
        REQUIRE_FALSE(ptr1 != nullptr);
        REQUIRE_FALSE(nullptr != ptr1);

        REQUIRE_FALSE(ptr1 == ptr2.get());
        REQUIRE_FALSE(ptr2.get() == ptr1);
        REQUIRE(ptr1 != ptr2.get());
        REQUIRE(ptr2.get() != ptr1);

        REQUIRE_FALSE(ptr2 == nullptr);
        REQUIRE_FALSE(nullptr == ptr2);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(nullptr != ptr2);

        REQUIRE(ptr2 == ptr2.get());
        REQUIRE(ptr2.get() == ptr2);
        REQUIRE_FALSE(ptr2 != ptr2.get());
        REQUIRE_FALSE(ptr2.get() != ptr2);
    }
    SECTION("clone")
    {
        auto ptr1 = allocate_joint<joint_test>(alloc, joint_size(10u), 5);
        verify(ptr1, alloc, 5);

        REQUIRE(alloc.no_allocated() == 1u);
        REQUIRE(alloc.last_allocated().size == sizeof(joint_test) + 10u);
        REQUIRE(alloc.last_allocated().alignment == FOONATHAN_ALIGNOF(joint_test));
        REQUIRE(alloc.no_deallocated() == 0u);

        auto ptr2 = clone_joint(alloc, *ptr1);
        verify(ptr2, alloc, 5);

        REQUIRE(alloc.no_allocated() == 2u);
        REQUIRE(alloc.last_allocated().size == sizeof(joint_test));
        REQUIRE(alloc.last_allocated().alignment == FOONATHAN_ALIGNOF(joint_test));
        REQUIRE(alloc.no_deallocated() == 0u);
    }

    REQUIRE(alloc.no_allocated() == 0u);
}

TEST_CASE("joint_allocator", "[allocator]")
{
    struct joint_test : joint_type<joint_test>
    {
        vector<int, joint_allocator> vec;
        int value;

        joint_test(joint tag, int value, std::size_t size)
        : joint_type(tag), vec(*this), value(value)
        {
            vec.reserve(size);
            vec.push_back(42);
            vec.push_back(-1);
        }
    };

    test_allocator alloc;

    auto ptr = allocate_joint<joint_test>(alloc, joint_size(10 * sizeof(int)), 5, 3);
    verify(ptr, alloc, 5);
    REQUIRE(ptr->vec[0] == 42);
    REQUIRE(ptr->vec[1] == -1);

    ptr->vec.push_back(5);
    REQUIRE(ptr->vec.back() == 5);

    REQUIRE(alloc.no_allocated() == 1u);
    REQUIRE(alloc.last_allocated().size == sizeof(joint_test) + 10 * sizeof(int));
    REQUIRE(alloc.last_allocated().alignment == FOONATHAN_ALIGNOF(joint_test));
}

template <typename T>
void verify(const joint_array<T>& array, std::size_t size)
{
    REQUIRE(array.data() == array.begin());
    REQUIRE(array.size() == size);
    REQUIRE(!array.empty());

    auto iter = array.begin();
    for (auto i = 0u; i != array.size(); ++i)
    {
        REQUIRE(&array[i] == array.data() + i);
        REQUIRE(&array[i] == iter);
        REQUIRE(iter - array.begin() == i);
        ++iter;
    }
}

TEST_CASE("joint_array", "[allocator]")
{
    struct joint_test : joint_type<joint_test>
    {
        int value;

        joint_test(joint tag, int v) : joint_type(tag), value(v)
        {
        }
    };

    test_allocator alloc;
    auto           ptr = allocate_joint<joint_test>(alloc, joint_size(20 * sizeof(int)), 5);
    verify(ptr, alloc, 5);

    SECTION("size constructor")
    {
        joint_array<int> arr(5, *ptr);
        verify(arr, 5);
        for (auto el : arr)
            REQUIRE(el == 0);
    }
    SECTION("size value constructor")
    {
        joint_array<int> arr(5, 1, *ptr);
        verify(arr, 5);
        for (auto el : arr)
            REQUIRE(el == 1);
    }
    SECTION("ilist constructor")
    {
        joint_array<int> arr({1, 2, 3}, *ptr);
        verify(arr, 3);
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 2);
        REQUIRE(arr[2] == 3);
    }
    SECTION("iterator constructor")
    {
        int              input[] = {1, 2, 3};
        joint_array<int> arr(std::begin(input), std::end(input), *ptr);
        verify(arr, 3);
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 2);
        REQUIRE(arr[2] == 3);
    }
    SECTION("copy/move constructor")
    {
        joint_array<int> arr1({1, 2, 3}, *ptr);
        verify(arr1, 3);

        joint_array<int> arr2(arr1, *ptr);
        verify(arr2, 3);
        REQUIRE(arr2[0] == 1);
        REQUIRE(arr2[1] == 2);
        REQUIRE(arr2[2] == 3);
    }
}
