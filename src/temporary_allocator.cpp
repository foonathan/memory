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
    void default_growth_tracker(std::size_t) FOONATHAN_NOEXCEPT {}
    // purposely not initialized due to a weird Windows bug with clang and maybe others?
    // if missing thread support thread_local variables will only be zero initialized
    // initialization of growth tracker only done on access, since this is the only way to set it
    static FOONATHAN_THREAD_LOCAL temporary_allocator::growth_tracker stack_growth_tracker;

    class stack_impl_allocator
    {
    public:
        using is_stateful = std::false_type;

        stack_impl_allocator() FOONATHAN_NOEXCEPT
        : first_call_(true)
        {}

        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            if (first_call_)
                first_call_ = false;
            else if (stack_growth_tracker)
                stack_growth_tracker(size);
            else // not initialized yet, see comment at definition
                stack_growth_tracker = default_growth_tracker;

            return default_allocator().allocate_node(size, alignment);
        }

        void deallocate_node(void *memory, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            default_allocator().deallocate_node(memory, size, alignment);
        }

        std::size_t max_node_size() const FOONATHAN_NOEXCEPT
        {
            return default_allocator().max_node_size();
        }

    private:
        bool first_call_;
    };

    using stack_type = memory_stack<stack_impl_allocator>;
    using storage_t = std::aligned_storage<sizeof(stack_type), FOONATHAN_ALIGNOF(stack_type)>::type;
    FOONATHAN_THREAD_LOCAL storage_t temporary_stack;
    // whether or not the temporary_stack has been created
    FOONATHAN_THREAD_LOCAL bool is_created = false;

    stack_type& get() FOONATHAN_NOEXCEPT
    {
        FOONATHAN_MEMORY_ASSERT(is_created);
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
        is_created = false;
    }
}

FOONATHAN_THREAD_LOCAL std::size_t detail::temporary_allocator_dtor_t::nifty_counter_ = 0u;

temporary_allocator::growth_tracker temporary_allocator::set_growth_tracker(growth_tracker t) FOONATHAN_NOEXCEPT
{
    // check if not initialized, see comment at definition
    auto old = stack_growth_tracker ? stack_growth_tracker : default_growth_tracker;
    stack_growth_tracker = t ? t : default_growth_tracker;
    return old;
}

temporary_allocator::growth_tracker temporary_allocator::get_growth_tracker() FOONATHAN_NOEXCEPT
{
    // check if not initialized, see comment at definition
    return stack_growth_tracker ? stack_growth_tracker : default_growth_tracker;
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
    FOONATHAN_MEMORY_ASSERT_MSG(top_ == this, "must allocate from top temporary allocator");
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
