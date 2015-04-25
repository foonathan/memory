// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "small_free_list.hpp"

#include <cassert>
#include <limits>
#include <new>

using namespace foonathan::memory;
using namespace detail;

small_free_memory_list::small_free_memory_list(std::size_t node_size) noexcept
: alloc_chunk_(&dummy_chunk_), dealloc_chunk_(&dummy_chunk_),
  node_size_(node_size), capacity_(0u) {}
  
 small_free_memory_list::small_free_memory_list(std::size_t node_size,
                                    void *mem, std::size_t size) noexcept
: small_free_memory_list(node_size)
{
    insert(mem, size);
}

void small_free_memory_list::insert(void *memory, std::size_t size) noexcept
{
    auto chunk_unit = memory_offset + node_size_ * chunk::max_nodes;
    auto no_chunks = size / chunk_unit;
    auto mem = static_cast<char*>(memory);
    for (std::size_t i = 0; i != no_chunks; ++i)
    {
        auto c = ::new(static_cast<void*>(mem)) chunk(node_size_);
        c->prev = &dummy_chunk_;
        c->next = dummy_chunk_.next;
        dummy_chunk_.next = c;
        if (dummy_chunk_.prev == &dummy_chunk_)
            dummy_chunk_.prev = c;
        mem += chunk_unit;
    }
    auto remaining = size % chunk_unit - memory_offset;
    auto last_chunk_size = remaining / node_size_;
    auto c = ::new(static_cast<void*>(mem)) chunk(node_size_, last_chunk_size);
    c->prev = &dummy_chunk_;
    c->next = dummy_chunk_.next;
    dummy_chunk_.next = c;
    if (dummy_chunk_.prev == &dummy_chunk_)
        dummy_chunk_.prev = c;
    capacity_ += no_chunks * chunk::max_nodes + last_chunk_size; 
}

void* small_free_memory_list::allocate() noexcept
{
    --capacity_;
    if (alloc_chunk_->empty())
        find_chunk(1);
    assert(!alloc_chunk_->empty());
    return alloc_chunk_->allocate(node_size_);
}

void small_free_memory_list::deallocate(void *node) noexcept
{
    ++capacity_;
    if (!dealloc_chunk_->from_chunk(node, node_size_))
    {
        auto next = dealloc_chunk_->next, prev = dealloc_chunk_->prev;
        while (true)
        {
            if (next->from_chunk(node, node_size_))
            {
                dealloc_chunk_ = next;
                break;
            }
            else if (prev->from_chunk(node, node_size_))
            {
                dealloc_chunk_ = prev;
                break;
            }
            next = next->next;
            prev = prev->prev;
            assert(next != dealloc_chunk_ || prev != dealloc_chunk_
                && "node not from any chunk, looped through entire list");
        }
    }
    dealloc_chunk_->deallocate(node, node_size_);
}

bool small_free_memory_list::find_chunk(std::size_t n) noexcept
{
    assert(capacity_ >= n);
    if (alloc_chunk_->no_nodes == n)
        return true;
    auto next = alloc_chunk_->next;
    auto prev = alloc_chunk_->prev;
    while (next != alloc_chunk_ || prev != alloc_chunk_)
    {
        if (next->no_nodes >= n)
        {
            alloc_chunk_ = next;
            return true;
        }
        else if (prev->no_nodes >= n)
        {
            alloc_chunk_ = prev;
            return true;
        }
        next = next->next;
        prev = prev->prev;
    }
    return false;
}
