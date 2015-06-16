// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/small_free_list.hpp"

#include <cassert>
#include <limits>
#include <new>
#include <utility>

#include "detail/align.hpp"
#include "debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

struct detail::chunk
{
    chunk *next = this, *prev = this;
    unsigned char first_node = 0u, capacity = 0u, no_nodes = 0u;
};

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

    // whether or not a pointer can be from a certain chunk
    bool from_chunk(chunk *c, std::size_t node_size, void *mem) FOONATHAN_NOEXCEPT
    {
        // comparision not strictly legal, but works
        return list_memory(c) <= mem
            && mem < list_memory(c) + node_size * c->no_nodes;
    }

    // whether or not a pointer is in the list of a certain chunk
    bool chunk_contains(chunk *c, std::size_t node_size, void *pointer) FOONATHAN_NOEXCEPT
    {
        auto cur_index = c->first_node;
        while (cur_index != c->no_nodes)
        {
            auto cur_mem = list_memory(c) + cur_index * node_size;
            if (cur_mem == pointer)
                return true;
            cur_index = *cur_mem;
        }
        return false;
    }

    // advances a pointer to the next chunk
    void next(chunk* &c) FOONATHAN_NOEXCEPT
    {
        assert(c);
        c = c->next;
    }

    // advances a pointer to the previous chunk
    void prev(chunk* &c) FOONATHAN_NOEXCEPT
    {
        assert(c);
        c = c->prev;
    }
}

chunk_list::chunk_list(chunk_list &&other) FOONATHAN_NOEXCEPT
: first_(other.first_)
{
    other.first_ = nullptr;
}

chunk_list& chunk_list::operator=(chunk_list &&other) FOONATHAN_NOEXCEPT
{
    chunk_list tmp(std::move(other));
    swap(*this, tmp);
    return *this;
}

void foonathan::memory::detail::swap(chunk_list &a, chunk_list &b) FOONATHAN_NOEXCEPT
{
    std::swap(a.first_, b.first_);
}

void chunk_list::insert(chunk *c) FOONATHAN_NOEXCEPT
{
    // insert at the front

    if (first_ == nullptr)
    {
        c->next = c;
        c->prev = c;
        first_ = c;
    }
    else
    {
        c->next = first_;
        c->prev = first_->prev;
        first_->prev = c;
        first_ = c;
    }
}

chunk* chunk_list::insert(chunk_list &other) FOONATHAN_NOEXCEPT
{
    assert(!other.empty());
    auto c = other.first_;
    if (other.first_ == other.first_->next)
        // only element
        other.first_ = nullptr;
    else
    {
        c->prev->next = c->next;
        c->next->prev = c->prev;
        other.first_ = other.first_->next;
    }
    insert(c);
    return c;
}

FOONATHAN_CONSTEXPR std::size_t small_free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t small_free_memory_list::min_element_alignment;

small_free_memory_list::small_free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT
: alloc_chunk_(nullptr), dealloc_chunk_(nullptr),
  node_size_(node_size + 2 * debug_fence_size), capacity_(0u) {}

small_free_memory_list::small_free_memory_list(std::size_t node_size,
                                    void *mem, std::size_t size) FOONATHAN_NOEXCEPT
: small_free_memory_list(node_size)
{
    insert(mem, size);
}

small_free_memory_list::small_free_memory_list(small_free_memory_list &&other) FOONATHAN_NOEXCEPT
: unused_chunks_(std::move(other.unused_chunks_)), used_chunks_(std::move(other.used_chunks_)),
  alloc_chunk_(other.alloc_chunk_), dealloc_chunk_(other.dealloc_chunk_),
  node_size_(other.node_size_), capacity_(other.capacity_)
{
    other.alloc_chunk_ = other.dealloc_chunk_ = nullptr;
    other.capacity_ = 0u;
}

small_free_memory_list& small_free_memory_list::operator=(small_free_memory_list &&other) FOONATHAN_NOEXCEPT
{
    small_free_memory_list tmp(std::move(other));
    swap(*this, tmp);
    return *this;
}

void foonathan::memory::detail::swap(small_free_memory_list &a, small_free_memory_list &b) FOONATHAN_NOEXCEPT
{
    using std::swap;
    swap(a.unused_chunks_, b.unused_chunks_);
    swap(a.used_chunks_, b.used_chunks_);
    swap(a.alloc_chunk_, b.alloc_chunk_);
    swap(a.dealloc_chunk_, b.dealloc_chunk_);
    swap(a.node_size_, b.node_size_);
    swap(a.capacity_, b.capacity_);
}

void small_free_memory_list::insert(void *memory, std::size_t size) FOONATHAN_NOEXCEPT
{
    auto chunk_unit = chunk_memory_offset + node_size_ * chunk_max_nodes;
    auto no_chunks = size / chunk_unit;
    auto mem = static_cast<char*>(memory);
    for (std::size_t i = 0; i != no_chunks; ++i)
    {
        auto c = create_chunk(mem, node_size_, chunk_max_nodes);
        unused_chunks_.insert(c);
        mem += chunk_unit;
    }
    std::size_t remaining = 0;
    if (size % chunk_unit > chunk_memory_offset)
    {
        remaining = size % chunk_unit - chunk_memory_offset;
        if (remaining > node_size_)
        {
            auto c = create_chunk(mem, node_size_, static_cast<unsigned char>(remaining / node_size_));
            unused_chunks_.insert(c);
        }
    }
    auto inserted_memory = no_chunks * chunk_max_nodes + remaining / node_size_;
    assert(inserted_memory > 0u && "too small memory size");
    capacity_ += inserted_memory;
}

void* small_free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    if (!alloc_chunk_ || alloc_chunk_->capacity == 0u)
        find_chunk(1);
    assert(alloc_chunk_ && alloc_chunk_->capacity != 0u);

    auto node_memory = list_memory(alloc_chunk_) + alloc_chunk_->first_node * node_size_;
    alloc_chunk_->first_node = *node_memory;
    --alloc_chunk_->capacity;
    --capacity_;

    return debug_fill_new(node_memory, node_size());
}

void small_free_memory_list::deallocate(void *memory) FOONATHAN_NOEXCEPT
{
    debug_fill(memory, node_size(), debug_magic::freed_memory);

    auto node_memory = static_cast<unsigned char*>(memory) - debug_fence_size;
    auto dealloc_chunk = chunk_for(node_memory);

    auto info = allocator_info(FOONATHAN_MEMORY_IMPL_LOG_PREFIX "::detail::small_free_memory_list", this);

    // memory was never managed by this list
    check_pointer(dealloc_chunk, info, memory);
    auto offset = static_cast<std::size_t>(node_memory - list_memory(dealloc_chunk));
    // memory is not at the right position
    check_pointer(offset % node_size_ == 0, info, memory);
    // double-free
    check_pointer(!chunk_contains(dealloc_chunk, node_size_, node_memory), info, memory);

    *node_memory = dealloc_chunk->first_node;
    dealloc_chunk->first_node = static_cast<unsigned char>(offset / node_size_);
    ++dealloc_chunk->capacity;
    ++capacity_;
}

std::size_t small_free_memory_list::node_size() const FOONATHAN_NOEXCEPT
{
    // note: node_size_ contains debug fence size, result not
    return node_size_ - 2 * debug_fence_size;
}

bool small_free_memory_list::find_chunk(std::size_t n) FOONATHAN_NOEXCEPT
{
    assert(capacity_ >= n && n <= chunk_max_nodes);
    if (alloc_chunk_ && alloc_chunk_->capacity >= n)
        return true;
    else if (!unused_chunks_.empty())
    {
        alloc_chunk_ = used_chunks_.insert(unused_chunks_);
        if (!dealloc_chunk_)
            dealloc_chunk_ = alloc_chunk_;
        return true;
    }
    assert(dealloc_chunk_);
    if (dealloc_chunk_->capacity >= n)
    {
        alloc_chunk_ = dealloc_chunk_;
        return true;
    }

    auto forward_iter = dealloc_chunk_, backward_iter = dealloc_chunk_;
    do
    {
        forward_iter = forward_iter->next;
        backward_iter = backward_iter->prev;

        if (forward_iter->capacity >= n)
        {
            alloc_chunk_ = forward_iter;
            return true;
        }
        else if (backward_iter->capacity >= n)
        {
            alloc_chunk_ = backward_iter;
            return true;
        }
    } while (forward_iter != backward_iter);
    return false;
}

chunk* small_free_memory_list::chunk_for(void *memory) FOONATHAN_NOEXCEPT
{
    assert(dealloc_chunk_ && alloc_chunk_);
    if (from_chunk(dealloc_chunk_, node_size_, memory))
        return dealloc_chunk_;
    else if (from_chunk(alloc_chunk_, node_size_, memory))
    {
        dealloc_chunk_ = alloc_chunk_;
        return alloc_chunk_;
    }

    auto i = 0u;
    auto forward_iter = dealloc_chunk_, backward_iter = dealloc_chunk_;
    do
    {
        next(forward_iter);
        prev(backward_iter);
        ++i;
        if (from_chunk(forward_iter, node_size_, memory))
        {
            dealloc_chunk_ = forward_iter;
            return forward_iter;
        }
        else if (from_chunk(backward_iter, node_size_, memory))
        {
            dealloc_chunk_ = backward_iter;
            return backward_iter;
        }
    } while (forward_iter != backward_iter);
    // at this point, both iterators point to the same chunk
    // this only happens after the entire list has been searched
    return nullptr;
}
