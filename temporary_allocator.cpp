// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "temporary_allocator.hpp"

#include <memory>

using namespace foonathan::memory;

namespace
{
    thread_local std::size_t nifty_counter = 0u;
    
    using storage_type = typename std::aligned_storage<sizeof(memory_stack<>), alignof(memory_stack<>)>::type;
    thread_local storage_type temporary_stack;
    
    memory_stack<>& get_stack() noexcept
    {
        return *static_cast<memory_stack<>*>(static_cast<void*>(&temporary_stack));
    }
    
    memory_stack<>& might_construct_stack()
    {
        if (nifty_counter++ == 0u)
            ::new(static_cast<void*>(&temporary_stack)) memory_stack<>(4096u);
        return get_stack();
    }
}

temporary_allocator::temporary_allocator(temporary_allocator &&other) noexcept
: marker_(other.marker_), unwind_(true)
{
    other.unwind_ = false;
}

temporary_allocator::~temporary_allocator() noexcept
{
    if (unwind_)
        get_stack().unwind(marker_);
    if (--nifty_counter == 0u)
        get_stack().~memory_stack<>();
}

temporary_allocator& temporary_allocator::operator=(temporary_allocator &&other) noexcept
{
    marker_ = other.marker_;
    unwind_ = true;
    other.unwind_ = false;
    return *this;
}

void* temporary_allocator::allocate(std::size_t size, std::size_t alignment)
{
    return get_stack().allocate(size, alignment);
}

temporary_allocator::temporary_allocator() noexcept
: marker_(get_stack().top()), unwind_(true) {}

std::size_t allocator_traits<temporary_allocator>::max_node_size(const allocator_type &) noexcept
{
    return get_stack().next_capacity();
}
