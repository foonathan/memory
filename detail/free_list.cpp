// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "free_list.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>

#include "../debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

namespace
{
    // reads a stored pointer value
    char* get_ptr(char *address) FOONATHAN_NOEXCEPT
    {
        assert(address);
        char* result = nullptr;
        std::memcpy(&result, address, sizeof(char*));
        return result;
    }

    // stores a pointer value
    void set_ptr(char *address, char *ptr) FOONATHAN_NOEXCEPT
    {
        assert(address);
        std::memcpy(address, &ptr, sizeof(char*));
    }
}

FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_alignment;

free_memory_list::free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT
: first_(nullptr),
  node_size_(std::max(node_size, min_element_size) + 2 * debug_fence_size),
  capacity_(0u)
{}

free_memory_list::free_memory_list(std::size_t node_size,
                 void *mem, std::size_t size) FOONATHAN_NOEXCEPT
: free_memory_list(node_size)
{
    insert(mem, size);
}

free_memory_list::free_memory_list(free_memory_list &&other) FOONATHAN_NOEXCEPT
: first_(other.first_), node_size_(other.node_size_), capacity_(other.capacity_)
{
    other.first_ = nullptr;
    other.capacity_ = 0u;
}

free_memory_list& free_memory_list::operator=(free_memory_list &&other) FOONATHAN_NOEXCEPT
{
    first_  = other.first_;
    node_size_ = other.node_size_;
    capacity_  = other.capacity_;

    other.first_ = nullptr;
    other.capacity_ = 0u;

    return *this;
}

void free_memory_list::insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    auto no_nodes = size / node_size_;
    assert(no_nodes > 0u);

    auto ptr = static_cast<char*>(mem);
    for (std::size_t i = 0u; i != no_nodes - 1; ++i, ptr += node_size_)
        set_ptr(ptr, ptr + node_size_);
    set_ptr(ptr, first_);
    first_ = static_cast<char*>(mem);

    capacity_ += no_nodes;
}

void* free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    assert(capacity_ > 0u);
    --capacity_;

    auto node = first_;
    first_ = get_ptr(first_);

    return debug_fill_new(node, node_size());
}

void free_memory_list::deallocate(void* ptr) FOONATHAN_NOEXCEPT
{
    auto node = debug_fill_free(ptr, node_size());

    set_ptr(node, first_);
    first_ = node;

    ++capacity_;
}

std::size_t free_memory_list::node_size() const FOONATHAN_NOEXCEPT
{
    return node_size_ - 2 * debug_fence_size;
}

namespace
{
    // xor for pointers
    char* xor_ptr(char *prev, char *next) FOONATHAN_NOEXCEPT
    {
        auto a = reinterpret_cast<std::uintptr_t>(prev);
        auto b = reinterpret_cast<std::uintptr_t>(next);
        auto val = a ^ b;
        return reinterpret_cast<char*>(val);
    }

    // returns the next pointer given the previous pointer
    char* get_next(char *address, char *prev) FOONATHAN_NOEXCEPT
    {
        return xor_ptr(get_ptr(address), prev);
    }

    // returns the prev pointer given the next pointer
    char* get_prev(char *address, char *next) FOONATHAN_NOEXCEPT
    {
        return xor_ptr(get_ptr(address), next);
    }

    // sets the next and previous pointer
    void set_next_prev(char *address, char *prev, char *next) FOONATHAN_NOEXCEPT
    {
        set_ptr(address, xor_ptr(prev, next));
    }

    // changes next pointer given the old next pointer
    void change_next(char *address, char *old_next, char *new_next) FOONATHAN_NOEXCEPT
    {
        assert(address);
        // use old_next to get previous pointer
        auto prev = get_prev(address, old_next);
        // don't change previous pointer
        set_next_prev(address, prev, new_next);
    }

    // same for prev
    void change_prev(char *address, char *old_prev, char *new_prev) FOONATHAN_NOEXCEPT
    {
        assert(address);
        auto next = get_next(address, old_prev);
        set_next_prev(address, new_prev, next);
    }

    // advances a pointer pair forward
    void next(char* &cur, char* &prev) FOONATHAN_NOEXCEPT
    {
        auto next = get_next(cur, prev);
        prev = cur;
        cur = next;
    }

    // advances a pointer pair backward
    void prev(char* &cur, char* &next) FOONATHAN_NOEXCEPT
    {
        auto prev = get_prev(cur, next);
        next = cur;
        cur = prev;
    }
}

FOONATHAN_CONSTEXPR std::size_t ordered_free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t ordered_free_memory_list::min_element_alignment;

void ordered_free_memory_list::list_impl::insert(std::size_t node_size,
                        void *memory, std::size_t no_nodes, bool new_memory) FOONATHAN_NOEXCEPT
{
    assert(no_nodes > 0u);
    auto pos = find_pos(node_size, static_cast<char*>(memory));

    auto cur = static_cast<char*>(memory), prev = pos.prev;
    if (pos.prev)
        // update next pointer of preceding node from pos.after to cur
        change_next(pos.prev, pos.after, cur);
    else
        // update first_ pointer
        first_ = cur;

    for (std::size_t i = 0u; i != no_nodes - 1; ++i)
    {
        // previous node is old position of iterator, next node is node_size further
        set_next_prev(cur, prev, cur + node_size);
        next(cur, prev);
    }

    // from last node: prev is old position, next is calculated position after
    set_next_prev(cur, prev, pos.after);

    if (pos.after)
        // update prev pointer of following node from pos.prev to cur
        change_prev(pos.after, pos.prev, cur);

    // point insert to last inserted node, if not new memory
    if (!new_memory)
    {
        insert_ = cur;
        insert_prev_ = prev;
    }
}

void* ordered_free_memory_list::list_impl::erase(std::size_t) FOONATHAN_NOEXCEPT
{
    assert(!empty());

    auto to_erase = first_;

    // first_ has no previous node
    auto new_first = get_next(first_, nullptr);
    if (new_first)
        // change new_first previous node from first_ to nullptr
        change_prev(new_first, first_, nullptr);

    // update insert pointer if needed
    if (insert_ == first_)
    {
        insert_ = new_first;
        insert_prev_ = nullptr;
    }

    first_ = new_first;
    return to_erase;
}

void* ordered_free_memory_list::list_impl::
    erase(std::size_t node_size, std::size_t bytes_needed,
          std::size_t& no_nodes) FOONATHAN_NOEXCEPT
{
    assert(!empty());
    if (bytes_needed <= node_size)
        return erase(node_size);

    for (char* cur = first_, *prev = nullptr; cur; next(cur, prev))
    {
        // whether or not to update insert because it would be removed
        auto update_insert = cur == insert_;

        auto begin = cur, begin_prev = prev;
        auto available = node_size; // we already have node_size bytes available
        while (get_next(cur, prev) == cur + node_size)
        {
            next(cur, prev);
            if (cur == insert_)
                update_insert = true;

            available += node_size;
            if (available >= bytes_needed) // found enough blocks
            {
                // begin_prev is node before array
                // begin is first node in array
                // prev is second last node in array
                // cur is last node in array
                // end is one past last node
                auto end = get_next(cur, prev);

                assert((cur - begin) % node_size == 0u);
                no_nodes = (cur - begin) / node_size + 1; // add one, can't use end here

                // update next
                if (begin_prev)
                    // change next from begin to end
                    change_next(begin_prev, begin, end);
                else
                    // update first_
                    first_ = end;

                // update prev
                if (end)
                {
                    // change end prev from cur to begin_prev
                    change_prev(end, cur, begin_prev);

                    if (end == insert_)
                        insert_prev_ = begin_prev;
                }

                // update insert
                if (update_insert)
                {
                    insert_ = end;
                    insert_prev_ = begin_prev;
                }

                return begin;
            }
        }
    }
    return nullptr;
}

ordered_free_memory_list::list_impl::pos
    ordered_free_memory_list::list_impl::find_pos(std::size_t,
                                                  char* memory) const FOONATHAN_NOEXCEPT
{
    auto greater = std::greater<char*>();
    auto less = std::less<char*>();

    // starting position is insert_, if set, otherwise first_
    // first_ might be null, too, but this is handled
    // insert_prev_ is the previous node in either case
    char* cur = insert_ ? insert_ : first_;

    if (!cur)
        // empty list
        return {nullptr, nullptr};
    else if (less(cur, memory))
    {
        // memory is greater, advance until greater
        char *prev = insert_prev_;
        next(cur, prev);

        while (cur)
        {
            if (greater(cur, memory))
                break;
            FOONATHAN_MEMORY_IMPL_POINTER_CHECK(cur != memory,
                                                "foonathan::memory::detail::ordered_free_memory_list",
                                                this, memory);
            next(cur, prev);
        }

        return {prev, cur};
    }
    else
    {
        // memory is smaller, go back until smaller
        char* next = get_next(cur, insert_prev_);
        while (cur)
        {
            if (less(cur, memory))
                break;
            FOONATHAN_MEMORY_IMPL_POINTER_CHECK(cur != memory,
                                                "foonathan::memory::detail::ordered_free_memory_list",
                                                this, memory);
            prev(cur, next);
        }

        return {cur, next};
    }
}

ordered_free_memory_list::ordered_free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT
: node_size_(std::max(node_size, min_element_size) + 2 * debug_fence_size),
  capacity_(0u)
{}

ordered_free_memory_list::ordered_free_memory_list(std::size_t node_size,
                                                   void *mem, std::size_t size) FOONATHAN_NOEXCEPT
: ordered_free_memory_list(node_size)
{
    insert(mem, size);
}

ordered_free_memory_list::ordered_free_memory_list(
        ordered_free_memory_list &&other) FOONATHAN_NOEXCEPT
: list_(std::move(other.list_)),
  node_size_(other.node_size_), capacity_(other.capacity_)
{
    other.capacity_ = 0u;
}

ordered_free_memory_list &ordered_free_memory_list::operator=(
        ordered_free_memory_list &&other) FOONATHAN_NOEXCEPT
{
    list_ = std::move(other.list_);
    node_size_ = other.node_size_;
    capacity_ = other.capacity_;

    other.capacity_ = 0u;
    return *this;
}

void ordered_free_memory_list::insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    auto no_nodes = size / node_size_;
    list_.insert(node_size_, mem, no_nodes, true);
    capacity_ += no_nodes;
}

void* ordered_free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    assert(capacity_ > 0u);
    --capacity_;

    auto node = list_.erase(node_size_);

    return debug_fill_new(node, node_size());
}

void* ordered_free_memory_list::
    allocate(std::size_t n, std::size_t node_size) FOONATHAN_NOEXCEPT
{
    auto bytes_needed = n * node_size + 2 * debug_fence_size;

    std::size_t no_nodes;
    auto nodes = list_.erase(node_size_, bytes_needed, no_nodes);
    if (!nodes)
        return nullptr;

    capacity_ -= no_nodes;
    return nodes;
}

void ordered_free_memory_list::deallocate(void* ptr) FOONATHAN_NOEXCEPT
{
    auto node = debug_fill_free(ptr, node_size());

    list_.insert(node_size_, node, 1, false);

    ++capacity_;
}

void ordered_free_memory_list::
        deallocate(void *ptr, std::size_t n, std::size_t node_size) FOONATHAN_NOEXCEPT
{
    auto node = debug_fill_free(ptr, n * node_size);

    auto bytes = n * node_size + 2 * debug_fence_size;
    auto no_nodes = bytes / node_size_ + (bytes % node_size_ != 0);
    list_.insert(node_size_, node, no_nodes, false);

    capacity_ += no_nodes;
}

std::size_t ordered_free_memory_list::node_size() const FOONATHAN_NOEXCEPT
{
    return node_size_ - 2 * debug_fence_size;
}
