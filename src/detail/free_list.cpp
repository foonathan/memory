// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/free_list.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

#include "detail/align.hpp"
#include "debugging.hpp"
#include "error.hpp"

using namespace foonathan::memory;
using namespace detail;

namespace
{
    // reads stored integer value
    std::uintptr_t get_int(void *address) FOONATHAN_NOEXCEPT
    {
        FOONATHAN_MEMORY_ASSERT(address);
        return *static_cast<std::uintptr_t*>(address);
    }

    // sets stored integer value
    void set_int(void *address, std::uintptr_t i) FOONATHAN_NOEXCEPT
    {
        FOONATHAN_MEMORY_ASSERT(address);
        *static_cast<std::uintptr_t*>(address) = i;
    }

    // pointer to integer
    std::uintptr_t to_int(char *ptr) FOONATHAN_NOEXCEPT
    {
        return reinterpret_cast<std::uintptr_t>(ptr);
    }

    // integer to pointer
    char* from_int(std::uintptr_t i) FOONATHAN_NOEXCEPT
    {
        return reinterpret_cast<char*>(i);
    }

    // reads a stored pointer value
    char* get_ptr(void *address) FOONATHAN_NOEXCEPT
    {
        return from_int(get_int(address));
    }

    // stores a pointer value
    void set_ptr(void *address, char *ptr) FOONATHAN_NOEXCEPT
    {
        set_int(address, to_int(ptr));
    }
}

FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_alignment;

void *free_memory_list::cache::allocate(std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
{
    // use alignment as fence size
    auto fence = debug_fence_size ? alignment : 0u;
    if (fence + size + fence > std::size_t(end_ - cur_))
        return nullptr;

    debug_fill(cur_, fence, debug_magic::fence_memory);
    cur_ += fence;

    auto mem = cur_;
    debug_fill(cur_, size, debug_magic::new_memory);
    cur_ += size;

    debug_fill(cur_, fence, debug_magic::fence_memory);
    cur_ += fence;

    return mem;
}

bool free_memory_list::cache::try_deallocate(void *ptr,
                                             std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
{
    auto fence_size = debug_fence_size ? alignment : 0u;
    auto node = static_cast<char*>(ptr);
    if (node + size + fence_size != cur_)
        // cannot be deallocated
        return false;
    debug_fill(node, size, debug_magic::freed_memory);
    cur_ = node - fence_size; // shrink cur back
    return true;
}

std::size_t free_memory_list::cache::no_nodes(std::size_t node_size) const FOONATHAN_NOEXCEPT
{
    auto actual_size = node_size + (debug_fence_size ? 2 * alignment_for(node_size) : 0u);
    return std::size_t(end_ - cur_) / actual_size;
}

std::size_t free_memory_list::list_impl::insert(char *begin, char *end,
                                         std::size_t node_size) FOONATHAN_NOEXCEPT
{
    // increase node size by fence, if necessary
    // alignment is fence memory
    auto actual_size = node_size + (debug_fence_size ? 2 * alignment_for(node_size) : 0u);
    auto no_nodes = std::size_t(end - begin) / actual_size;
    if (no_nodes == 0u)
        return 0u;

    auto cur = begin;
    for (std::size_t i = 0u; i != no_nodes - 1; ++i)
    {
        set_ptr(cur, cur + actual_size);
        cur += actual_size;
    }
    set_ptr(cur, first_);
    first_ = begin;

    return no_nodes;
}

void free_memory_list::list_impl::push(void *ptr, std::size_t node_size) FOONATHAN_NOEXCEPT
{
    // alignment is fence memory
    auto node = debug_fill_free(ptr, node_size, alignment_for(node_size));

    set_ptr(node, first_);
    first_ = node;
}

void *free_memory_list::list_impl::pop(std::size_t node_size) FOONATHAN_NOEXCEPT
{
    if (!first_)
        return nullptr;

    auto mem = first_;
    first_ = get_ptr(mem);

    // alignment is fence memory
    return debug_fill_new(mem, node_size, alignment_for(node_size));
}

free_memory_list::free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT
: node_size_(node_size > min_element_size ? node_size : min_element_size),
  capacity_(0u)
{}

free_memory_list::free_memory_list(std::size_t node_size,
                 void *mem, std::size_t size) FOONATHAN_NOEXCEPT
: free_memory_list(node_size)
{
    insert(mem, size);
}

free_memory_list::free_memory_list(free_memory_list &&other) FOONATHAN_NOEXCEPT
: cache_(std::move(other.cache_)), list_(std::move(other.list_)),
  node_size_(other.node_size_), capacity_(other.capacity_)
{
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
    detail::adl_swap(a.cache_, b.cache_);
    detail::adl_swap(a.list_, b.list_);
    detail::adl_swap(a.node_size_, b.node_size_);
    detail::adl_swap(a.capacity_, b.capacity_);
}

void free_memory_list::insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    // insert into cache and old cache into list
    FOONATHAN_MEMORY_ASSERT(is_aligned(mem, alignment()));

    list_.insert(cache_.top(), cache_.end(), node_size_); // insert cache into list
    cache_ = cache(mem, size); // insert new memory into cache

    capacity_ += cache_.no_nodes(node_size_);
}

void* free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(!empty());
    --capacity_;

    // try to return from list, to reserve cache for arrays
    auto mem = list_.pop(node_size_);
    if (!mem)
        // use cache
        mem = cache_.allocate(node_size_, alignment());
    // mem must not be nullptr now
    FOONATHAN_MEMORY_ASSERT(mem);
    return mem;
}

void* free_memory_list::allocate(std::size_t n) FOONATHAN_NOEXCEPT
{
    auto old_nodes = cache_.no_nodes(node_size_);

    // allocate from cache
    auto mem = cache_.allocate(n, alignment());
    if (!mem)
        return nullptr;

    auto diff = old_nodes - cache_.no_nodes(node_size_);
    capacity_ -= diff;
    return mem;
}

void free_memory_list::deallocate(void* ptr) FOONATHAN_NOEXCEPT
{
    // try to insert into cache
    if (!cache_.try_deallocate(ptr, node_size_, alignment()))
        // insert into list if failed
        list_.push(ptr, node_size_);
    ++capacity_;
}

void free_memory_list::deallocate(void *ptr, std::size_t n) FOONATHAN_NOEXCEPT
{
    auto old_nodes = cache_.no_nodes(node_size_);

    // try to insert into cache
    if (cache_.try_deallocate(ptr, n, alignment()))
    {
        auto diff = cache_.no_nodes(node_size_) - old_nodes;
        capacity_ += diff;
    }
    else // insert into list otherwise
    {
        auto fence = (debug_fence_size ? alignment() : 0u);
        auto node = static_cast<char*>(ptr) - fence;
        capacity_ += list_.insert(node, node + fence + n + fence, node_size_);
    }
}

std::size_t free_memory_list::node_size() const FOONATHAN_NOEXCEPT
{
    return node_size_;
}

bool free_memory_list::empty() const FOONATHAN_NOEXCEPT
{
   return capacity() == 0u;
}

std::size_t free_memory_list::alignment() const FOONATHAN_NOEXCEPT
{
    return alignment_for(node_size_);
}

namespace
{

    // returns the next pointer given the previous pointer
    char* get_next(void *address, char *prev) FOONATHAN_NOEXCEPT
    {
        return from_int(get_int(address) ^ to_int(prev));
    }

    // returns the prev pointer given the next pointer
    char* get_prev(char *address, char *next) FOONATHAN_NOEXCEPT
    {
        return from_int(get_int(address) ^ to_int(next));
    }

    // sets the next and previous pointer
    void set_next_prev(char *address, char *prev, char *next) FOONATHAN_NOEXCEPT
    {
        set_int(address, to_int(prev) ^ to_int(next));
    }

    // changes next pointer given the old next pointer
    void change_next(char *address, char *old_next, char *new_next) FOONATHAN_NOEXCEPT
    {
        FOONATHAN_MEMORY_ASSERT(address);
        // use old_next to get previous pointer
        auto prev = get_prev(address, old_next);
        // don't change previous pointer
        set_next_prev(address, prev, new_next);
    }

    // same for prev
    void change_prev(char *address, char *old_prev, char *new_prev) FOONATHAN_NOEXCEPT
    {
        FOONATHAN_MEMORY_ASSERT(address);
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
    FOONATHAN_MEMORY_ASSERT(no_nodes > 0u);
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
    // cur is now the last node
    set_next_prev(cur, prev, pos.after);

    if (pos.after)
        // update prev pointer of following node from pos.prev to cur
        change_prev(pos.after, pos.prev, cur);
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
    auto new_first = get_next(first_, nullptr);
    if (new_first)
        // change new_first previous node from first_ to nullptr
        change_prev(new_first, first_, nullptr);
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

    for (char* cur = last_, *next = nullptr; cur; prev(cur, next))
    {
        // whether or not to update insert because it would be removed
        auto update_insert = cur == insert_;

        auto last = cur, end = next;
        auto available = node_size; // we already have node_size bytes available
        while (get_prev(cur, next) == cur - node_size)
        {
            prev(cur, next);
            if (cur == insert_)
                update_insert = true;

            available += node_size;
            if (available >= bytes_needed) // found enough blocks
            {
                // begin_prev is node before array
                // cur is first node in array
                // last is last node in array
                // end is one after last node
                auto begin_prev = get_prev(cur, next);

                FOONATHAN_MEMORY_ASSERT(std::size_t(last - cur) % node_size == 0u);

                // update next
                if (begin_prev)
                    // change next from cur to end
                    change_next(begin_prev, cur, end);
                else
                    // update first_
                    first_ = end;

                // update prev
                if (end)
                {
                    // change end prev from last to begin_prev
                    change_prev(end, last, begin_prev);

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
                        insert_prev_ = begin_prev ? get_prev(begin_prev, end) : nullptr;
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
            detail::check_pointer(cur != memory,
                                  {FOONATHAN_MEMORY_IMPL_LOG_PREFIX "::detail::ordered_free_memory_list",
                                   this}, memory);
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
            detail::check_pointer(cur != memory,
                                  {FOONATHAN_MEMORY_IMPL_LOG_PREFIX "::detail::ordered_free_memory_list",
                                   this}, memory);
            prev(cur, next);
        }

        return {cur, next};
    }
}

bool ordered_free_memory_list::list_impl::empty() const FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(bool(first_) == bool(last_));
    return !first_;
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
