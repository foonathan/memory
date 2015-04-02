// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "temporary_allocator.hpp"

#include <memory>

using namespace foonathan::memory;

namespace
{    
    using storage_type = typename std::aligned_storage<sizeof(memory_stack<>), alignof(memory_stack<>)>::type;
    thread_local storage_type temporary_stack;
    thread_local bool is_created = false;
    thread_local std::size_t nifty_counter = 0u; // for destructor call
    
    memory_stack<>& get_stack() noexcept
    {
        return *static_cast<memory_stack<>*>(static_cast<void*>(&temporary_stack));
    }
    
    memory_stack<>& might_construct_stack(std::size_t size)
    {
        if (!is_created)
        {
            ::new(static_cast<void*>(&temporary_stack)) memory_stack<>(size);
            is_created = true;
        }
        return get_stack();
    }
}

detail::temporary_allocator_dtor_t::temporary_allocator_dtor_t() noexcept
{
    ++nifty_counter;
}

detail::temporary_allocator_dtor_t::~temporary_allocator_dtor_t() noexcept
{
    if (--nifty_counter == 0u)
    {
        get_stack().~memory_stack<>();
        // at this point the current thread is over, so boolean not necessary
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

temporary_allocator::temporary_allocator(std::size_t size) noexcept
: marker_(might_construct_stack(size).top()), unwind_(true) {}

std::size_t allocator_traits<temporary_allocator>::max_node_size(const allocator_type &) noexcept
{
    return get_stack().next_capacity();
}
