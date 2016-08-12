// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "temporary_allocator.hpp"

#include <new>
#include <type_traits>

#include "default_allocator.hpp"
#include "error.hpp"

using namespace foonathan::memory;

namespace
{
    void default_growth_tracker(std::size_t) FOONATHAN_NOEXCEPT
    {
    }

    using temporary_impl_allocator        = default_allocator;
    using temporary_impl_allocator_traits = allocator_traits<temporary_impl_allocator>;
}

detail::temporary_block_allocator::temporary_block_allocator(std::size_t block_size)
    FOONATHAN_NOEXCEPT : tracker_(default_growth_tracker),
                         block_size_(block_size)
{
}

detail::temporary_block_allocator::growth_tracker detail::temporary_block_allocator::
    set_growth_tracker(growth_tracker t) FOONATHAN_NOEXCEPT
{
    auto old = tracker_;
    tracker_ = t;
    return old;
}

detail::temporary_block_allocator::growth_tracker detail::temporary_block_allocator::
    get_growth_tracker() FOONATHAN_NOEXCEPT
{
    return tracker_;
}

memory_block detail::temporary_block_allocator::allocate_block()
{
    auto alloc  = temporary_impl_allocator();
    auto memory = temporary_impl_allocator_traits::allocate_array(alloc, block_size_, 1,
                                                                  detail::max_alignment);
    block_size_ *= growing_block_allocator<temporary_impl_allocator>::growth_factor();
    return memory_block(memory, block_size_);
}

void detail::temporary_block_allocator::deallocate_block(memory_block block)
{
    auto alloc = temporary_impl_allocator();
    temporary_impl_allocator_traits::deallocate_array(alloc, block.memory, block.size, 1,
                                                      detail::max_alignment);
}

temporary_allocator::temporary_allocator(temporary_stack& stack) : unwind_(stack), prev_(stack.top_)
{
    FOONATHAN_MEMORY_ASSERT(!prev_ || prev_->is_active());
    stack.top_ = this;
}

temporary_allocator::~temporary_allocator()
{
    if (is_active())
        unwind_.get_stack().top_ = prev_;
}

void* temporary_allocator::allocate(std::size_t size, std::size_t alignment)
{
    FOONATHAN_MEMORY_ASSERT_MSG(is_active(), "object isn't the active allocator");
    return unwind_.get_stack().stack_.allocate(size, alignment);
}

bool temporary_allocator::is_active() const FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(unwind_.will_unwind());
    auto res = unwind_.get_stack().top_ == this;
    // check that prev is actually before this
    FOONATHAN_MEMORY_ASSERT(!res || !prev_ || prev_->unwind_.get_marker() <= unwind_.get_marker());
    return res;
}

