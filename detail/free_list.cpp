// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "free_list.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <utility>

#include "../debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

namespace
{
    // xor for pointer
    char* xor_ptr(char *prev, char *next) FOONATHAN_NOEXCEPT
    {
        auto a = reinterpret_cast<std::uintptr_t>(prev);
        auto b = reinterpret_cast<std::uintptr_t>(next);
        auto val = a ^ b;
        return reinterpret_cast<char*>(val);
    }

    // returns the stored pointer value
    char* get_ptr_impl(void* ptr) FOONATHAN_NOEXCEPT
    {
        assert(ptr);
        char* result = nullptr;
        std::memcpy(&result, ptr, sizeof(char*));
        return result;
    }

    // returns the next pointer
    char* get_next(void *ptr, char *prev) FOONATHAN_NOEXCEPT
    {
        return xor_ptr(get_ptr_impl(ptr), prev);
    }

    // returns the prev pointer
    char* get_prev(void *ptr, char *next) FOONATHAN_NOEXCEPT
    {
        return xor_ptr(get_ptr_impl(ptr), next);
    }

    // sets the next and previous pointer
    void set_next_prev(void *ptr, char *prev, char *next) FOONATHAN_NOEXCEPT
    {
        assert(ptr);
        auto val = xor_ptr(prev, next);
        std::memcpy(ptr, &val, sizeof(char*));
    }

    // changes next pointer given the old next pointer
    void change_next(void *ptr, char *old_next, char *new_next) FOONATHAN_NOEXCEPT
    {
        assert(ptr);
        // use old_next to get previous pointer
        auto prev = get_prev(ptr, old_next);
        // don't change previous pointer
        set_next_prev(ptr, prev, new_next);
    }

    // same for prev
    void change_prev(void *ptr, char *old_prev, char *new_prev) FOONATHAN_NOEXCEPT
    {
        assert(ptr);
        auto next = get_next(ptr, old_prev);
        set_next_prev(ptr, new_prev, next);
    }

    // builds the list in memory block mem that is followed by no_blocks * node_size of memory
    // doesn't set next of last node
    // prev is the previous node of the first node in memory
    // after is the next node of the last node in memory
    // returns the last node
    char* build_list(char *prev, char *after, std::size_t node_size,
                                void *mem, std::size_t no_blocks) FOONATHAN_NOEXCEPT
    {
        assert(no_blocks != 0u);
        auto ptr = static_cast<char*>(mem);
        for (std::size_t i = 0u; i != no_blocks - 1; ++i)
        {
            set_next_prev(ptr, prev, ptr + node_size);
            prev = ptr;
            ptr += node_size;
        }
        set_next_prev(ptr, prev, after);
        return ptr;
    }

    // finds the position in the list to insert mem, so that the total list is ordered
    // goes in both directions starting at forward and backward
    // debug: checks that mem isn't already in the list
    std::pair<char*, char*> find_position(void *mem,
                                char *forward, char *forward_prev,
                                char *backward, char *backward_next) FOONATHAN_NOEXCEPT
    {
        auto greater = std::greater<char*>();
        auto less = std::less<char*>();

        auto memory = static_cast<char*>(mem);
        for (;;)
        {
            if (forward)
            {
                if (greater(forward, memory))
                    return std::make_pair(forward_prev, forward);
                FOONATHAN_MEMORY_IMPL_POINTER_CHECK(forward != memory,
                    "foonathan::memory::detail::free_memory_list", list, mem);
                auto next = get_next(forward, forward_prev);
                forward_prev = forward;
                forward = next;
            }
            if (backward)
            {
                if (less(backward, memory))
                    return std::make_pair(backward, backward_next);
                FOONATHAN_MEMORY_IMPL_POINTER_CHECK(backward != memory,
                    "foonathan::memory::detail::free_memory_list", list, mem);
                auto prev = get_prev(backward, backward_next);
                backward_next = backward;
                backward = prev;
            }
        }
        return {};
    }

    // checks if a free memory block no_bytes bug starts at cur
    // pre: cur
    bool check_bytes(char* &prev, char* &cur, std::size_t no_bytes, std::size_t el_size) FOONATHAN_NOEXCEPT
    {
        auto available = el_size; // we already have one (cur)
        if (available >= no_bytes)
            return true;
        for (; cur; )
        {
            if (get_next(cur, prev) == cur + el_size)
            {
                prev = cur;
                cur += el_size;
                available += el_size;
                if (available >= no_bytes)
                    break;
            }
            else
                return false;
        }
        return true;
    }
}

FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_alignment;

free_memory_list::free_memory_list(std::size_t el_size) FOONATHAN_NOEXCEPT
: first_(nullptr), last_(nullptr), last_next_(nullptr),
  node_size_(std::max(min_element_size, el_size) + 2 * debug_fence_size),
  capacity_(0u)
{}

free_memory_list::free_memory_list(std::size_t el_size,
                                   void *mem, std::size_t size) FOONATHAN_NOEXCEPT
: free_memory_list(el_size)
{
    insert(mem, size);
}

free_memory_list::free_memory_list(free_memory_list &&other) FOONATHAN_NOEXCEPT
: first_(other.first_), last_(other.last_), last_next_(other.last_next_),
  node_size_(other.node_size_), capacity_(other.capacity_)
{
    other.first_ = other.last_ = other.last_next_ = nullptr;
    other.capacity_ = 0;
}

free_memory_list& free_memory_list::operator=(free_memory_list &&other) FOONATHAN_NOEXCEPT
{
    first_ = other.first_;
    last_ = other.last_;
    last_next_ = other.last_next_;
    node_size_ = other.node_size_;
    capacity_ = other.capacity_;
    other.first_ = other.last_ = other.last_next_ = nullptr;
    other.capacity_ = 0u;
    return *this;
}

void free_memory_list::insert(void *mem, std::size_t size) FOONATHAN_NOEXCEPT
{
#if FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
    if (!empty())
        return insert_ordered(mem, size);
#endif
    auto no_blocks = size / node_size_;
    capacity_ += no_blocks;
    insert_between(nullptr, first_, mem, no_blocks);
}

void free_memory_list::insert_ordered(void *mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    if (empty())
        return insert(mem, size);
    auto no_blocks = size / node_size_;
    capacity_ += no_blocks;
    auto pos = find_position(mem, first_, nullptr, last_, last_next_);
    insert_between(pos.first, pos.second, mem, no_blocks);
}

void free_memory_list::insert_between(char *pre, char *after,
                                      void *mem, std::size_t no_blocks) FOONATHAN_NOEXCEPT
{
    // list is build with pre being the node before and after the node after
    auto last = build_list(pre, after, node_size_, mem, no_blocks);

    if (pre)
        // change pre's next from after to memory
        change_next(pre, after, static_cast<char*>(mem));
    else
        // insert at beginning
        first_ = static_cast<char*>(mem);

    if (after)
        // change after's prev from prev to last node
        change_prev(after, pre, last);

    last_ = first_;
    last_next_ = get_next(first_, nullptr);
}

void* free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    --capacity_;
    auto block = first_;

    // first_ has no previous node
    auto new_first = get_next(first_, nullptr);
    last_ = new_first;
    if (new_first)
    {
        // change new_first's node previous node from first_ to nullptr
        change_prev(new_first, first_, nullptr);
        last_next_ = get_next(last_, nullptr);
    }
    else
        last_next_  = nullptr;
    first_ = new_first;

    debug_fill(block, debug_fence_size, debug_magic::fence_memory);
    block += debug_fence_size;
    debug_fill(block, node_size(), debug_magic::new_memory);
    debug_fill(block + node_size(), debug_fence_size, debug_magic::fence_memory);

    return block;
}

void* free_memory_list::allocate(std::size_t n, std::size_t other_node_size) FOONATHAN_NOEXCEPT
{
    auto n_bytes = n * other_node_size;
    auto needed = n_bytes + 2 * debug_fence_size;
    auto prev = static_cast<char*>(nullptr);
    for(auto cur = first_; cur; )
    {
        auto begin = cur;
        auto begin_prev = prev;;
        if (check_bytes(prev, cur, needed, node_size_))
        {
            // found continuos nodes big enough for need memory
            // cur is the last element inclusive, next(cur) is the next free node
            // prev is the second last element

            assert((cur - start) % node_size_ == 0u);
            auto no_nodes = (cur - begin) / node_size_ + 1;
            capacity_ -= no_nodes;

            auto end = get_next(cur, prev);
            if (begin_prev)
                // change begin_prev's next from begin to end
                change_next(begin_prev, begin, end);
            else
                // begin was the first node
                first_ = end;

            last_ = end;
            if (end)
            {
                // change end's previous node from cur to begin_prev
                change_prev(end, cur, begin_prev);
                last_next_ = get_next(last_, end);
            }
            else
                last_next_ = nullptr;


            debug_fill(begin, debug_fence_size, debug_magic::fence_memory);
            begin += debug_fence_size;
            debug_fill(begin, n_bytes, debug_magic::new_memory);
            debug_fill(begin + n_bytes, debug_fence_size, debug_magic::fence_memory);

            return begin;
        }
        auto next = get_next(cur, prev);
        prev = cur;
        cur = next;
    }
    return nullptr;
}

void free_memory_list::deallocate(void *ptr) FOONATHAN_NOEXCEPT
{
#if FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
    // use ordered version since checks for double free needed
    // when searching in the last anyway, it can also be optimized
    deallocate_ordered(ptr);
#else
    debug_fill(ptr, node_size(), debug_magic::freed_memory);

    auto node_ptr = static_cast<char*>(ptr) - debug_fence_size;
    insert_between(nullptr, first_, node_ptr, 1);
    ++capacity_;
#endif
}

void free_memory_list::deallocate(void *ptr, std::size_t n,
                                std::size_t other_node_size) FOONATHAN_NOEXCEPT
{
#if FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
    deallocate_ordered(ptr, n, other_node_size);
#else
    auto n_bytes = n * other_node_size;
    debug_fill(ptr, n_bytes, debug_magic::freed_memory);

    auto node_ptr = static_cast<char*>(ptr) - debug_fence_size;
    auto bytes = n_bytes + 2 * debug_fence_size;
    auto no_blocks = bytes / node_size_ + (bytes % node_size_ != 0);

    insert_between(nullptr, first_, node_ptr, no_blocks);
    capacity_ += no_blocks;
#endif
}

void free_memory_list::deallocate_ordered(void *ptr) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_IMPL_POINTER_CHECK(ptr,
        "foonathan::memory::detail::free_memory_list", first_, ptr);
    debug_fill(ptr, node_size(), debug_magic::freed_memory);

    auto node_ptr = static_cast<char*>(ptr) - debug_fence_size;

    auto pos = find_position(node_ptr, first_, nullptr, last_, last_next_);
    insert_between(pos.first, pos.second, node_ptr, 1);
    ++capacity_;
}

void free_memory_list::deallocate_ordered(void *ptr, std::size_t n,
                                        std::size_t other_node_size) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_IMPL_POINTER_CHECK(ptr,
        "foonathan::memory::detail::free_memory_list", first_, ptr);
    auto n_bytes = n * other_node_size;
    debug_fill(ptr, n_bytes, debug_magic::freed_memory);

    auto node_ptr = static_cast<char*>(ptr) - debug_fence_size;
    auto bytes = n_bytes + 2 * debug_fence_size;
    auto no_blocks = bytes / node_size_ + (bytes % node_size_ != 0);

    auto pos = find_position(node_ptr, first_, nullptr, last_, last_next_);
    insert_between(pos.first, pos.second, node_ptr, no_blocks);
    capacity_ += no_blocks;
}

std::size_t free_memory_list::node_size() const FOONATHAN_NOEXCEPT
{
    // note: node_size_ contains debug fence size, result not
    return node_size_ - 2 * debug_fence_size;
}
