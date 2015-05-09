// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "small_free_list.hpp"

#include <cassert>
#include <limits>
#include <new>

#include "align.hpp"
#include "../debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

namespace
{    
    static FOONATHAN_CONSTEXPR auto alignment_div = sizeof(chunk) / detail::max_alignment;
    static FOONATHAN_CONSTEXPR auto alignment_mod = sizeof(chunk) % detail::max_alignment;
    // offset from chunk to actual list
    static FOONATHAN_CONSTEXPR auto chunk_memory_offset = alignment_mod == 0u ? sizeof(chunk)
                                        : (alignment_div + 1) * detail::max_alignment;
    // maximum nodes per chunk                                    
    static FOONATHAN_CONSTEXPR auto chunk_max_nodes = std::numeric_limits<unsigned char>::max();
    
    // returns the memory of the actual free list of a chunk
    unsigned char* list_memory(void *c) FOONATHAN_NOEXCEPT
    {
        return static_cast<unsigned char*>(c) + chunk_memory_offset;
    }
    
    // creates a chunk at mem
    // mem must have at least the size chunk_memory_offset + no_nodes * node_size
    chunk* create_chunk(void *mem, std::size_t node_size, unsigned char no_nodes) FOONATHAN_NOEXCEPT
    {
        auto c = ::new(mem) chunk;
        c->first_node = 0;
        c->no_nodes = no_nodes;
        c->capacity = no_nodes;
        auto p = list_memory(c);
        for (unsigned char i = 0u; i != no_nodes; p += node_size)
            *p = ++i;
        return c;
    }
    
    // inserts a chunk into the chunk list
    // pushs it at the back of the list
    void insert_chunk(chunk &list, chunk *c) FOONATHAN_NOEXCEPT
    {
        c->prev = list.prev;
        c->next = &list;
        list.prev = c;
        if (list.next == &list)
            list.next = c;
    }
    
    // whether or not a pointer can be from a certain chunk
    bool from_chunk(chunk *c, std::size_t node_size, void *mem) FOONATHAN_NOEXCEPT
    {
        // comparision not strictly legal, but works
        return list_memory(c) <= mem
            && mem < list_memory(c) + node_size * c->no_nodes;
    }
}

FOONATHAN_CONSTEXPR std::size_t small_free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t small_free_memory_list::min_element_alignment;

small_free_memory_list::small_free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT
: alloc_chunk_(&dummy_chunk_), dealloc_chunk_(&dummy_chunk_), unused_chunk_(nullptr),
  node_size_(node_size), capacity_(0u) {}
  
 small_free_memory_list::small_free_memory_list(std::size_t node_size,
                                    void *mem, std::size_t size) FOONATHAN_NOEXCEPT
: small_free_memory_list(node_size)
{
    insert(mem, size);
}

void small_free_memory_list::insert(void *memory, std::size_t size) FOONATHAN_NOEXCEPT
{
    auto chunk_unit = chunk_memory_offset + node_size_ * chunk_max_nodes;
    auto no_chunks = size / chunk_unit;
    auto mem = static_cast<char*>(memory);
    for (std::size_t i = 0; i != no_chunks; ++i)
    {
        auto c = create_chunk(mem, node_size_, chunk_max_nodes);
        c->next = nullptr;
        c->prev = unused_chunk_;
        unused_chunk_ = c;
        mem += chunk_unit;
    }
    auto remaining = size % chunk_unit - chunk_memory_offset;
    if (remaining > node_size_)
    {
        auto c = create_chunk(mem, node_size_, static_cast<unsigned char>(remaining / node_size_));
        c->next = nullptr;
        c->prev = unused_chunk_;
        unused_chunk_ = c;
    }
    auto inserted_memory = no_chunks * chunk_max_nodes + remaining / node_size_;
    assert(inserted_memory > 0u && "too small memory size");
    capacity_ += inserted_memory;
}

void* small_free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    if (alloc_chunk_->capacity == 0u)
        find_chunk(1);
    assert(alloc_chunk_->capacity != 0u);
    auto memory = list_memory(alloc_chunk_) + alloc_chunk_->first_node * node_size_;
    alloc_chunk_->first_node = *memory;
    --alloc_chunk_->capacity;
    --capacity_;
    detail::debug_fill(memory, node_size(), debug_magic::new_memory);
    return memory;
}

void small_free_memory_list::deallocate(void *node) FOONATHAN_NOEXCEPT
{
    if (!from_chunk(dealloc_chunk_, node_size_, node))
    {
        auto next = dealloc_chunk_->next, prev = dealloc_chunk_->prev;
        while (next != dealloc_chunk_ || prev != dealloc_chunk_)
        {
            if (from_chunk(next, node_size_, node))
            {
                dealloc_chunk_ = next;
                break;
            }
            else if (from_chunk(prev, node_size_, node))
            {
                dealloc_chunk_ = prev;
                break;
            }
            next = next->next;
            prev = prev->prev;
        }
    }
    assert(from_chunk(dealloc_chunk_, node_size_, node));
    detail::debug_fill(node, node_size(), debug_magic::freed_memory);
    auto node_mem = static_cast<unsigned char*>(node);
    *node_mem = dealloc_chunk_->first_node;
    auto offset = static_cast<std::size_t>(node_mem - list_memory(dealloc_chunk_));
    assert(offset % node_size_ == 0);
    dealloc_chunk_->first_node = static_cast<unsigned char>(offset / node_size_);
    ++dealloc_chunk_->capacity;
    ++capacity_;
}

bool small_free_memory_list::find_chunk(std::size_t n) FOONATHAN_NOEXCEPT
{
    assert(capacity_ >= n && n <= chunk_max_nodes);
    if (alloc_chunk_->capacity >= n)
        return true;
    else if (unused_chunk_)
    {
        auto c = unused_chunk_;
        unused_chunk_ = unused_chunk_->prev;
        insert_chunk(dummy_chunk_, c);
        alloc_chunk_ = c;
        return true;
    }

    auto next = dealloc_chunk_->next;
    auto prev = dealloc_chunk_->prev;
    while (next != dealloc_chunk_ || prev != dealloc_chunk_)
    {
        if (next->capacity >= n)
        {
            alloc_chunk_ = next;
            return true;
        }
        else if (prev->capacity >= n)
        {
            alloc_chunk_ = prev;
            return true;
        }
        next = next->next;
        prev = prev->prev;
    }
    return true;
}
