// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/small_free_list.hpp"

#include <limits>
#include <new>

#include "detail/align.hpp"
#include "detail/debug_helpers.hpp"
#include "detail/assert.hpp"
#include "error.hpp"

#include "free_list_utils.hpp"

using namespace foonathan::memory;
using namespace detail;

struct foonathan::memory::detail::chunk : chunk_base
{
    static const std::size_t memory_offset;
    static const std::size_t max_nodes;

    // gives it the size of the memory block it is created in and the size of a node
    chunk(std::size_t total_memory, std::size_t node_size) FOONATHAN_NOEXCEPT
        : chunk_base(static_cast<unsigned char>((total_memory - memory_offset) / node_size))
    {
        FOONATHAN_MEMORY_ASSERT((total_memory - memory_offset) / node_size <= max_nodes);
        FOONATHAN_MEMORY_ASSERT(capacity > 0);
        auto p = list_memory();
        for (unsigned char i = 0u; i != no_nodes; p += node_size)
            *p = ++i;
    }

    // returns memory of the free list
    unsigned char* list_memory() FOONATHAN_NOEXCEPT
    {
        auto mem = static_cast<void*>(this);
        return static_cast<unsigned char*>(mem) + memory_offset;
    }

    // returns the nth node
    unsigned char* node_memory(unsigned char i, std::size_t node_size) FOONATHAN_NOEXCEPT
    {
        FOONATHAN_MEMORY_ASSERT(i < no_nodes);
        return list_memory() + i * node_size;
    }

    // checks whether a node came from this chunk
    bool from(unsigned char* node, std::size_t node_size) FOONATHAN_NOEXCEPT
    {
        auto begin = list_memory();
        auto end   = list_memory() + no_nodes * node_size;
        return (begin <= node) & (node < end);
    }

    // checks whether a node is already in this chunk
    bool contains(unsigned char* node, std::size_t node_size) FOONATHAN_NOEXCEPT
    {
        auto cur_index = first_free;
        while (cur_index != no_nodes)
        {
            auto cur_mem = node_memory(cur_index, node_size);
            if (cur_mem == node)
                return true;
            cur_index = *cur_mem;
        }
        return false;
    }

    // allocates a single node
    // chunk most not be empty
    unsigned char* allocate(std::size_t node_size) FOONATHAN_NOEXCEPT
    {
        --capacity;

        auto node  = node_memory(first_free, node_size);
        first_free = *node;
        return node;
    }

    // deallocates a single node given its address and index
    // it must be from this chunk
    void deallocate(unsigned char* node, unsigned char node_index) FOONATHAN_NOEXCEPT
    {
        ++capacity;

        *node      = first_free;
        first_free = node_index;
    }
};

const std::size_t chunk::memory_offset =
    sizeof(chunk) % detail::max_alignment == 0 ?
        sizeof(chunk) :
        (sizeof(chunk) / detail::max_alignment + 1) * detail::max_alignment;
const std::size_t chunk::max_nodes = std::numeric_limits<unsigned char>::max();

namespace
{
    // converts a chunk_base to a chunk (if it is one)
    chunk* make_chunk(chunk_base* c) FOONATHAN_NOEXCEPT
    {
        return static_cast<chunk*>(c);
    }

    // same as above but also requires a certain size
    chunk* make_chunk(chunk_base* c, std::size_t size_needed) FOONATHAN_NOEXCEPT
    {
        FOONATHAN_MEMORY_ASSERT(size_needed <= std::numeric_limits<unsigned char>::max());
        return c->capacity >= size_needed ? make_chunk(c) : nullptr;
    }

    // checks if memory was from a chunk, assumes chunk isn't proxy
    chunk* from_chunk(chunk_base* c, unsigned char* node, std::size_t node_size) FOONATHAN_NOEXCEPT
    {
        auto res = make_chunk(c);
        return res->from(node, node_size) ? res : nullptr;
    }

    // inserts already interconnected chunks into the list
    // list will be kept ordered
    void insert_chunks(chunk_base* list, chunk_base* begin, chunk_base* end) FOONATHAN_NOEXCEPT
    {
        FOONATHAN_MEMORY_ASSERT(begin && end);

        if (list->next == list) // empty
        {
            begin->prev = list;
            end->next   = list->next;
            list->next  = begin;
            list->prev  = end;
        }
        else if (less(list->prev, begin)) // insert at end
        {
            list->prev->next = begin;
            begin->prev      = list->prev;
            end->next        = list;
            list->prev       = end;
        }
        else
        {
            auto prev = list;
            auto cur  = list->next;
            while (less(cur, begin))
            {
                prev = cur;
                cur  = cur->next;
            }
            FOONATHAN_MEMORY_ASSERT(greater(cur, end));
            FOONATHAN_MEMORY_ASSERT(prev == list || less(prev, begin));
            prev->next  = begin;
            begin->prev = prev;
            end->next   = cur;
            cur->prev   = end;
        }
    }
}

FOONATHAN_CONSTEXPR std::size_t small_free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t small_free_memory_list::min_element_alignment;

small_free_memory_list::small_free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT
    : node_size_(node_size),
      capacity_(0u),
      alloc_chunk_(&base_),
      dealloc_chunk_(&base_)
{
}

small_free_memory_list::small_free_memory_list(std::size_t node_size, void* mem,
                                               std::size_t size) FOONATHAN_NOEXCEPT
    : small_free_memory_list(node_size)
{
    insert(mem, size);
}

small_free_memory_list::small_free_memory_list(small_free_memory_list&& other) FOONATHAN_NOEXCEPT
    : node_size_(other.node_size_),
      capacity_(other.capacity_),
      // reset markers for simplicity
      alloc_chunk_(&base_),
      dealloc_chunk_(&base_)
{
    if (!other.empty())
    {
        base_.next             = other.base_.next;
        base_.prev             = other.base_.prev;
        other.base_.next->prev = &base_;
        other.base_.prev->next = &base_;

        other.base_.next = &other.base_;
        other.base_.prev = &other.base_;
        other.capacity_  = 0u;
    }
    else
    {
        base_.next = &base_;
        base_.prev = &base_;
    }
}

void foonathan::memory::detail::swap(small_free_memory_list& a,
                                     small_free_memory_list& b) FOONATHAN_NOEXCEPT
{
    auto b_next = b.base_.next;
    auto b_prev = b.base_.prev;

    if (!a.empty())
    {
        b.base_.next       = a.base_.next;
        b.base_.prev       = a.base_.prev;
        b.base_.next->prev = &b.base_;
        b.base_.prev->next = &b.base_;
    }
    else
    {
        b.base_.next = &b.base_;
        b.base_.prev = &b.base_;
    }

    if (!b.empty())
    {
        a.base_.next       = b_next;
        a.base_.prev       = b_prev;
        a.base_.next->prev = &a.base_;
        a.base_.prev->next = &a.base_;
    }
    else
    {
        a.base_.next = &a.base_;
        a.base_.prev = &a.base_;
    }

    detail::adl_swap(a.node_size_, b.node_size_);
    detail::adl_swap(a.capacity_, b.capacity_);

    // reset markers for simplicity
    a.alloc_chunk_ = a.dealloc_chunk_ = &a.base_;
    b.alloc_chunk_ = b.dealloc_chunk_ = &b.base_;
}

void small_free_memory_list::insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(mem);
    FOONATHAN_MEMORY_ASSERT(is_aligned(mem, max_alignment));
    debug_fill_internal(mem, size, false);

    auto actual_size      = node_size_ + 2 * fence_size();
    auto total_chunk_size = chunk::memory_offset + actual_size * chunk::max_nodes;
    auto no_chunks        = size / total_chunk_size;
    auto remainder        = size % total_chunk_size;

    auto memory = static_cast<char*>(mem);
    auto prev   = static_cast<chunk_base*>(nullptr);
    for (auto i = std::size_t(0); i != no_chunks; ++i)
    {
        auto c = ::new (static_cast<void*>(memory)) chunk(total_chunk_size, actual_size);

        c->prev = prev;
        if (prev)
            prev->next = c;
        prev           = c;

        memory += total_chunk_size;
    }

    auto new_nodes = no_chunks * chunk::max_nodes;
    if (remainder >= chunk::memory_offset + actual_size) // at least one node
    {
        auto c  = ::new (static_cast<void*>(memory)) chunk(remainder, actual_size);
        c->prev = prev;
        if (prev)
            prev->next = c;
        prev           = c;

        new_nodes += c->no_nodes;
    }

    FOONATHAN_MEMORY_ASSERT_MSG(new_nodes > 0, "memory block too small");
    insert_chunks(&base_, static_cast<chunk_base*>(mem), prev);
    capacity_ += new_nodes;
}

std::size_t small_free_memory_list::usable_size(std::size_t size) const FOONATHAN_NOEXCEPT
{
    auto actual_size      = node_size_ + 2 * fence_size();
    auto total_chunk_size = chunk::memory_offset + actual_size * chunk::max_nodes;
    auto no_chunks        = size / total_chunk_size;
    auto remainder        = size % total_chunk_size;

    return no_chunks * chunk::max_nodes * actual_size
           + (remainder > chunk::memory_offset ? remainder - chunk::memory_offset : 0u);
}

void* small_free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    auto chunk   = find_chunk_impl(1);
    alloc_chunk_ = chunk;
    FOONATHAN_MEMORY_ASSERT(chunk && chunk->capacity >= 1);

    --capacity_;

    auto mem = chunk->allocate(node_size_ + 2 * fence_size());
    FOONATHAN_MEMORY_ASSERT(mem);
    return detail::debug_fill_new(mem, node_size_, fence_size());
}

void small_free_memory_list::deallocate(void* mem) FOONATHAN_NOEXCEPT
{
    auto info =
        allocator_info(FOONATHAN_MEMORY_LOG_PREFIX "::detail::small_free_memory_list", this);

    auto actual_size = node_size_ + 2 * fence_size();
    auto node = static_cast<unsigned char*>(detail::debug_fill_free(mem, node_size_, fence_size()));

    auto chunk     = find_chunk_impl(node);
    dealloc_chunk_ = chunk;
    // memory was never allocated from list
    detail::debug_check_pointer([&] { return chunk != nullptr; }, info, mem);

    auto offset = node - chunk->list_memory();
    // memory is not at the right position
    debug_check_pointer([&] { return offset % actual_size == 0u; }, info, mem);
    // double-free
    debug_check_double_dealloc([&] { return !chunk->contains(node, actual_size); }, info, mem);

    auto index = offset / actual_size;
    FOONATHAN_MEMORY_ASSERT(index < chunk->no_nodes);
    chunk->deallocate(node, static_cast<unsigned char>(index));

    ++capacity_;
}

std::size_t small_free_memory_list::alignment() const FOONATHAN_NOEXCEPT
{
    return alignment_for(node_size_);
}

std::size_t small_free_memory_list::fence_size() const FOONATHAN_NOEXCEPT
{
    // node size is fence size
    return debug_fence_size ? node_size_ : 0u;
}

chunk* small_free_memory_list::find_chunk_impl(std::size_t n) FOONATHAN_NOEXCEPT
{
    if (auto c = make_chunk(alloc_chunk_, n))
        return c;
    else if (auto c = make_chunk(dealloc_chunk_, n))
        return c;

    auto cur_forward  = alloc_chunk_->next;
    auto cur_backward = alloc_chunk_->prev;

    do
    {
        if (auto c = make_chunk(cur_forward, n))
            return c;
        else if (auto c = make_chunk(cur_backward, n))
            return c;

        cur_forward  = cur_forward->next;
        cur_backward = cur_backward->prev;
        FOONATHAN_MEMORY_ASSERT(cur_forward != alloc_chunk_);
        FOONATHAN_MEMORY_ASSERT(cur_backward != alloc_chunk_);
    } while (true);
    FOONATHAN_MEMORY_UNREACHABLE("there is memory available somewhere...");
    return nullptr;
}

chunk* small_free_memory_list::find_chunk_impl(unsigned char* node, chunk_base* first,
                                               chunk_base* last) FOONATHAN_NOEXCEPT
{
    auto actual_size = node_size_ + 2 * fence_size();

    do
    {
        if (auto c = from_chunk(first, node, actual_size))
            return c;
        else if (auto c = from_chunk(last, node, actual_size))
            return c;

        first = first->next;
        last  = last->prev;
    } while (!greater(first, last));
    return nullptr;
}

chunk* small_free_memory_list::find_chunk_impl(unsigned char* node) FOONATHAN_NOEXCEPT
{
    auto actual_size = node_size_ + 2 * fence_size();

    if (auto c = from_chunk(dealloc_chunk_, node, actual_size))
        return c;
    else if (auto c = from_chunk(alloc_chunk_, node, actual_size))
        return c;
    else if (less(dealloc_chunk_, node))
    {
        // node is in (dealloc_chunk_, base_.prev]
        return find_chunk_impl(node, dealloc_chunk_->next, base_.prev);
    }
    else if (greater(dealloc_chunk_, node))
    {
        // node is in [base.next, dealloc_chunk_)
        return find_chunk_impl(node, base_.next, dealloc_chunk_->prev);
    }
    FOONATHAN_MEMORY_UNREACHABLE("must be in one half");
    return nullptr;
}
