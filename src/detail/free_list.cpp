// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/free_list.hpp"

#include <cstdint>

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <functional>
#endif

#include "detail/align.hpp"
#include "detail/debug_helpers.hpp"
#include "detail/assert.hpp"
#include "debugging.hpp"
#include "error.hpp"

#include "free_list_utils.hpp"

using namespace foonathan::memory;
using namespace detail;

FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_alignment;

free_memory_list::free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT
: first_(nullptr),
  node_size_(node_size > min_element_size ? node_size : min_element_size),
  capacity_(0u)
{}

free_memory_list::free_memory_list(std::size_t node_size,
                 void *mem, std::size_t size) FOONATHAN_NOEXCEPT
: free_memory_list(node_size)
{
    insert(mem, size);
}

free_memory_list::free_memory_list(free_memory_list &&other) FOONATHAN_NOEXCEPT
: first_(other.first_),
  node_size_(other.node_size_), capacity_(other.capacity_)
{
    other.first_ = nullptr;
    other.capacity_ = 0u;
}

free_memory_list& free_memory_list::operator=(free_memory_list &&other) FOONATHAN_NOEXCEPT
{
    free_memory_list tmp(detail::move(other));
    swap(*this, tmp);
    return *this;
}

void foonathan::memory::detail::swap(free_memory_list &a, free_memory_list &b) FOONATHAN_NOEXCEPT
{
    detail::adl_swap(a.first_, b.first_);
    detail::adl_swap(a.node_size_, b.node_size_);
    detail::adl_swap(a.capacity_, b.capacity_);
}

void free_memory_list::insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(mem);
    FOONATHAN_MEMORY_ASSERT(is_aligned(mem, alignment()));

    auto actual_size = node_size_ + 2 * fence_size();
    auto no_nodes = size / actual_size;
    FOONATHAN_MEMORY_ASSERT(no_nodes > 0);

    auto cur = static_cast<char*>(mem);
    for (std::size_t i = 0u; i != no_nodes - 1; ++i)
    {
        list_set_next(cur, cur + actual_size);
        cur += actual_size;
    }
    list_set_next(cur, first_);
    first_ = static_cast<char*>(mem);

    capacity_ += no_nodes;
}

void* free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(!empty());
    --capacity_;

    auto mem = first_;
    first_ = list_get_next(mem);
    return debug_fill_new(mem, node_size_, fence_size());
}

void free_memory_list::deallocate(void* ptr) FOONATHAN_NOEXCEPT
{
    ++capacity_;

    auto node = static_cast<char*>(debug_fill_free(ptr, node_size_, fence_size()));
    list_set_next(node, first_);
    first_ = node;
}

std::size_t free_memory_list::alignment() const FOONATHAN_NOEXCEPT
{
    return alignment_for(node_size_);
}

std::size_t free_memory_list::fence_size() const FOONATHAN_NOEXCEPT
{
    // alignment is fence size
    return debug_fence_size ? alignment() : 0u;
}

FOONATHAN_CONSTEXPR std::size_t ordered_free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t ordered_free_memory_list::min_element_alignment;

void ordered_free_memory_list::list_impl::insert(std::size_t node_size,
                        void *memory, std::size_t no_nodes, bool new_memory) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(no_nodes > 0u);
    auto pos = find_pos(node_size, static_cast<char*>(memory));

    auto cur = static_cast<char*>(memory), prev = pos.prev;
    if (pos.prev)
        // update next pointer of preceding node from pos.after to cur
        xor_list_change_next(pos.prev, pos.after, cur);
    else
        // update first_ pointer
        first_ = cur;

    for (std::size_t i = 0u; i != no_nodes - 1; ++i)
    {
        // previous node is old position of iterator, next node is node_size further
        xor_list_set(cur, prev, cur + node_size);
        xor_list_iter_next(cur, prev);
    }
    // from last node: prev is old position, next is calculated position after
    // cur is now the last node
    xor_list_set(cur, prev, pos.after);

    if (pos.after)
        // update prev pointer of following node from pos.prev to cur
        xor_list_change_prev(pos.after, pos.prev, cur);
    else
        // update last_ pointer
        last_ = cur;

    // point insert to last inserted node, if not new memory
    if (!new_memory)
    {
        insert_ = cur;
        insert_prev_ = prev;
    }
}

void* ordered_free_memory_list::list_impl::erase(std::size_t) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(!empty());

    auto to_erase = first_;

    // first_ has no previous node
    auto new_first = xor_list_get_next(first_, nullptr);
    if (new_first)
        // change new_first previous node from first_ to nullptr
        xor_list_change_prev(new_first, first_, nullptr);
    else
        // update last_ pointer, list is now empty
        last_ = nullptr;

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
    erase(std::size_t node_size, std::size_t bytes_needed) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(!empty());
    if (bytes_needed <= node_size)
        return erase(node_size);

    for (char* cur = last_, *next = nullptr; cur; xor_list_iter_prev(cur, next))
    {
        // whether or not to update insert because it would be removed
        auto update_insert = cur == insert_;

        auto last = cur, end = next;
        auto available = node_size; // we already have node_size bytes available
        while (xor_list_get_prev(cur, next) == cur - node_size)
        {
            xor_list_iter_prev(cur, next);
            if (cur == insert_)
                update_insert = true;

            available += node_size;
            if (available >= bytes_needed) // found enough blocks
            {
                // begin_prev is node before array
                // cur is first node in array
                // last is last node in array
                // end is one after last node
                auto begin_prev = xor_list_get_prev(cur, next);

                FOONATHAN_MEMORY_ASSERT(std::size_t(last - cur) % node_size == 0u);

                // update next
                if (begin_prev)
                    // change next from cur to end
                    xor_list_change_next(begin_prev, cur, end);
                else
                    // update first_
                    first_ = end;

                // update prev
                if (end)
                {
                    // change end prev from last to begin_prev
                    xor_list_change_prev(end, last, begin_prev);

                    // update insert position so that it points out of the array
                    if (end == insert_ || update_insert)
                    {
                        insert_prev_ = begin_prev;
                        insert_ = end;
                    }
                }
                else
                {
                    // update last_
                    last_ = begin_prev;

                    // update insert position
                    if (update_insert)
                    {
                        insert_ = begin_prev;
                        insert_prev_ = begin_prev ? xor_list_get_prev(begin_prev, end) : nullptr;
                    }
                }

                return cur;
            }
        }
    }
    return nullptr;
}

ordered_free_memory_list::list_impl::pos
    ordered_free_memory_list::list_impl::find_pos(std::size_t,
                                                  char* memory) const FOONATHAN_NOEXCEPT
{
#if FOONATHAN_HOSTED_IMPLEMENTATION
    auto greater = std::greater<char*>();
    auto less = std::less<char*>();
#else
    // compare integral values and hope it works
    auto greater = [](char *a, char *b)
    {
        return to_int(a) > to_int(b);
    };
    auto less = [](char *a, char *b)
    {
        return to_int(a) < to_int(b);
    };
#endif

    auto info = allocator_info(FOONATHAN_MEMORY_LOG_PREFIX "::detail::ordered_free_memory_list", this);

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
        xor_list_iter_next(cur, prev);

        while (cur)
        {
            if (greater(cur, memory))
                break;
            detail::debug_check_pointer([&]
                                        {
                                            return cur != memory;
                                        }, info, memory);
            xor_list_iter_next(cur, prev);
        }

        return {prev, cur};
    }
    else
    {
        // memory is smaller, go back until smaller
        char* next = xor_list_get_next(cur, insert_prev_);
        while (cur)
        {
            if (less(cur, memory))
                break;
            detail::debug_check_pointer([&]
                                        {
                                            return cur != memory;
                                        }, info, memory);
            xor_list_iter_prev(cur, next);
        }

        return {cur, next};
    }
}

bool ordered_free_memory_list::list_impl::empty() const FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(bool(first_) == bool(last_));
    return !bool(first_);
}

ordered_free_memory_list::ordered_free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT
: node_size_(node_size > min_element_size ? node_size : min_element_size),
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
: list_(detail::move(other.list_)),
  node_size_(other.node_size_), capacity_(other.capacity_)
{
    other.capacity_ = 0u;
}

ordered_free_memory_list &ordered_free_memory_list::operator=(
        ordered_free_memory_list &&other) FOONATHAN_NOEXCEPT
{
    ordered_free_memory_list tmp(detail::move(other));
    swap(*this, tmp);
    return *this;
}

void foonathan::memory::detail::swap(ordered_free_memory_list &a, ordered_free_memory_list &b) FOONATHAN_NOEXCEPT
{
    detail::adl_swap(a.list_, b.list_);
    detail::adl_swap(a.node_size_, b.node_size_);
    detail::adl_swap(a.capacity_, b.capacity_);
}

void ordered_free_memory_list::insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(is_aligned(mem, alignment()));
    auto no_nodes = size / node_fence_size();
    list_.insert(node_fence_size(), mem, no_nodes, true);
    capacity_ += no_nodes;
}

void* ordered_free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(capacity_ > 0u);
    --capacity_;

    auto node = list_.erase(node_fence_size());

    return debug_fill_new(node, node_size(), alignment());
}

void* ordered_free_memory_list::allocate(std::size_t n) FOONATHAN_NOEXCEPT
{
    auto fence = debug_fence_size ? alignment() : 0u;
    auto bytes_needed = n + 2 * fence;
    auto nodes = list_.erase(node_fence_size(), bytes_needed);
    if (!nodes)
        return nullptr;

    auto no_nodes = bytes_needed / node_fence_size() + (bytes_needed % node_fence_size() != 0);
    capacity_ -= no_nodes;
    return debug_fill_new(nodes, n, fence);
}

void ordered_free_memory_list::deallocate(void* ptr) FOONATHAN_NOEXCEPT
{
    auto node = debug_fill_free(ptr, node_size(), alignment());

    list_.insert(node_fence_size(), node, 1, false);

    ++capacity_;
}

void ordered_free_memory_list::
        deallocate(void *ptr, std::size_t n) FOONATHAN_NOEXCEPT
{
    auto fence = debug_fence_size ? alignment() : 0u;
    auto node = debug_fill_free(ptr, n, fence);

    auto bytes = n + 2 * fence;
    auto no_nodes = bytes / node_fence_size() + (bytes % node_fence_size() != 0);
    list_.insert(node_fence_size(), node, no_nodes, false);

    capacity_ += no_nodes;
}

std::size_t ordered_free_memory_list::node_size() const FOONATHAN_NOEXCEPT
{
    return node_size_;
}

std::size_t ordered_free_memory_list::alignment() const FOONATHAN_NOEXCEPT
{
    return alignment_for(node_size_);
}

std::size_t ordered_free_memory_list::node_fence_size() const FOONATHAN_NOEXCEPT
{
    return node_size_ + (debug_fence_size ? 2 * alignment() : 0u);
}
