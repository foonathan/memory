// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "block_list.hpp"

#include <cassert>
#include <new>

using namespace foonathan::memory;
using namespace detail;

struct block_list_impl::node
{
    node *prev;
    std::size_t size;
    
    node(node *p, std::size_t s) FOONATHAN_NOEXCEPT
    : prev(p), size(s) {}
};

std::size_t block_list_impl::impl_offset()
{
    return sizeof(node);
}

std::size_t block_list_impl::push(void* &memory, std::size_t size) FOONATHAN_NOEXCEPT
{
    auto ptr = ::new(memory) node(head_, size);
    head_ = ptr;
    memory = static_cast<char*>(memory) + sizeof(node);
    return sizeof(node);
}

block_info block_list_impl::push(block_list_impl &other) FOONATHAN_NOEXCEPT
{
    assert(other.head_ && "stack underflow");
    auto top = other.head_;
    other.head_ = top->prev;
    top->prev = head_;
    head_ = top;
    return {top, top->size - sizeof(node)};
}

block_info block_list_impl::pop() FOONATHAN_NOEXCEPT
{
    assert(head_ && "stack underflow");
    auto top = head_;
    head_ = top->prev;
    return {top, top->size};
}
