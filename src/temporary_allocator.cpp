// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "temporary_allocator.hpp"

#include <memory>

#include "default_allocator.hpp"
#include "raw_allocator_base.hpp"

using namespace foonathan::memory;

namespace
{
    class stack_impl_allocator : public raw_allocator_base<stack_impl_allocator>
    {
    public:
        stack_impl_allocator() FOONATHAN_NOEXCEPT {}

        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            if (!first_call_)
                tracker_(size);
            else
                first_call_ = false;
            return default_allocator().allocate_node(size, alignment);
        }

        void deallocate_node(void *memory, std::size_t size, std::size_t alignment)
        {
            default_allocator().deallocate_node(memory, size, alignment);
        }

        static temporary_allocator::growth_tracker set_tracker(temporary_allocator::growth_tracker t)
        {
            auto old = tracker_;
            tracker_ = t;
            return old;
        }

    private:
        static void default_tracker(std::size_t) FOONATHAN_NOEXCEPT {}

        static FOONATHAN_THREAD_LOCAL temporary_allocator::growth_tracker tracker_;
        static FOONATHAN_THREAD_LOCAL bool first_call_;
    };

    FOONATHAN_THREAD_LOCAL temporary_allocator::growth_tracker
        stack_impl_allocator::tracker_ = stack_impl_allocator::default_tracker;
    FOONATHAN_THREAD_LOCAL bool stack_impl_allocator::first_call_ = true;

    using stack_type = memory_stack<stack_impl_allocator>;
    using storage_t = std::aligned_storage<sizeof(stack_type), FOONATHAN_ALIGNOF(stack_type)>::type;
    FOONATHAN_THREAD_LOCAL storage_t temporary_stack;
    // whether or not the temporary_stack has been created
    FOONATHAN_THREAD_LOCAL bool is_created = false;

    stack_type& get() FOONATHAN_NOEXCEPT
    {
        assert(is_created);
        return *static_cast<stack_type*>(static_cast<void*>(&temporary_stack));
    }

    stack_type& create(std::size_t size)
    {
        if (!is_created)
        {
            ::new(static_cast<void*>(&temporary_stack)) stack_type(size);
            is_created = true;
        }
        return get();
    }
}

detail::temporary_allocator_dtor_t::temporary_allocator_dtor_t() FOONATHAN_NOEXCEPT
{
    ++nifty_counter_;
}

detail::temporary_allocator_dtor_t::~temporary_allocator_dtor_t() FOONATHAN_NOEXCEPT
{
    if (--nifty_counter_ == 0u && is_created)
    {
        get().~stack_type();
        // at this point the current thread is over, so boolean not necessary
    }
}

FOONATHAN_THREAD_LOCAL std::size_t detail::temporary_allocator_dtor_t::nifty_counter_ = 0u;

temporary_allocator::growth_tracker temporary_allocator::set_growth_tracker(growth_tracker t) FOONATHAN_NOEXCEPT
{
    return stack_impl_allocator::set_tracker(t);
}

temporary_allocator::temporary_allocator(temporary_allocator &&other) FOONATHAN_NOEXCEPT
: marker_(other.marker_), prev_(top_), unwind_(true)
{
    other.unwind_ = false;
    top_ = this;
}

temporary_allocator::~temporary_allocator() FOONATHAN_NOEXCEPT
{
    if (unwind_)
        get().unwind(marker_);
    top_ = prev_;
}

temporary_allocator& temporary_allocator::operator=(temporary_allocator &&other) FOONATHAN_NOEXCEPT
{
    marker_ = other.marker_;
    unwind_ = true;
    other.unwind_ = false;
    return *this;
}

void* temporary_allocator::allocate(std::size_t size, std::size_t alignment)
{
    assert(top_ == this && "must allocate from top temporary allocator");
    return get().allocate(size, alignment);
}

temporary_allocator::temporary_allocator(std::size_t size) FOONATHAN_NOEXCEPT
: marker_(create(size).top()), prev_(nullptr), unwind_(true)
{
    top_ = this;
}

FOONATHAN_THREAD_LOCAL const temporary_allocator* temporary_allocator::top_ = nullptr;

std::size_t allocator_traits<temporary_allocator>::max_node_size(const allocator_type &) FOONATHAN_NOEXCEPT
{
    return get().next_capacity();
}
