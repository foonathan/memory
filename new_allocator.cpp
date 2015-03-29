// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "new_allocator.hpp"

using namespace foonathan::memory;

void* new_allocator::allocate_node(std::size_t size, std::size_t)
{
    return ::operator new(size);
}

void new_allocator::deallocate_node(void* node, std::size_t size, std::size_t) noexcept
{
    ::operator delete(node);
}
