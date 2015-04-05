// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "temporary_allocator.hpp"

#include <memory>

#include "raw_allocator_base.hpp"

using namespace foonathan::memory;

namespace
{
    thread_local class
    {
    public:
        class impl_allocator : public raw_allocator_base<impl_allocator>
        {
        public:
            void* allocate_node(std::size_t size, std::size_t alignment);
            
            void deallocate_node(void *memory, std::size_t size, std::size_t alignment)
            {
                heap_allocator().deallocate_node(memory, size, alignment);
            }
        };
        
        using stack_type = memory_stack<impl_allocator>;
    
        stack_type& get() noexcept
        {
            assert(is_created_);
            return *static_cast<stack_type*>(static_cast<void*>(&storage_));
        }
        
        stack_type& create(std::size_t size)
        {
            if (!is_created_)
            {
                ::new(static_cast<void*>(&storage_)) stack_type(size);
                is_created_ = true;
            }
            return get();
        }
        
        temporary_allocator::growth_tracker set_tracker(temporary_allocator::growth_tracker t)
        {
            auto old = tracker_;
            tracker_ = t;
            return old;
        }
    
    private:
        static void default_tracker(std::size_t) noexcept {}
    
        using storage_t = typename std::aligned_storage<sizeof(stack_type), alignof(stack_type)>::type;
        storage_t storage_;
        temporary_allocator::growth_tracker tracker_ = default_tracker;
        bool is_created_ = false, first_call_ = true;
    } temporary_stack;

    void* decltype(temporary_stack)::impl_allocator::
                allocate_node(std::size_t size, std::size_t alignment)
    {
        if (!temporary_stack.first_call_)
            temporary_stack.tracker_(size);
        else
            temporary_stack.first_call_ = false;
        return heap_allocator().allocate_node(size, alignment);
    }
}

detail::temporary_allocator_dtor_t::temporary_allocator_dtor_t() noexcept
{
    ++nifty_counter_;
}

detail::temporary_allocator_dtor_t::~temporary_allocator_dtor_t() noexcept
{
    if (--nifty_counter_ == 0u)
    {
        using stack_t = decltype(temporary_stack)::stack_type;
        temporary_stack.get().~stack_t();
        // at this point the current thread is over, so boolean not necessary
    }
}

thread_local std::size_t detail::temporary_allocator_dtor_t::nifty_counter_ = 0u;

temporary_allocator::growth_tracker temporary_allocator::set_growth_tracker(growth_tracker t) noexcept
{
    return temporary_stack.set_tracker(t);
}

temporary_allocator::temporary_allocator(temporary_allocator &&other) noexcept
: marker_(other.marker_), prev_(top_), unwind_(true)
{
    other.unwind_ = false;
    top_ = this;
}

temporary_allocator::~temporary_allocator() noexcept
{
    if (unwind_)
        temporary_stack.get().unwind(marker_);
    top_ = prev_;
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
    assert(top_ == this && "must allocate from top temporary allocator");
    return temporary_stack.get().allocate(size, alignment);
}

temporary_allocator::temporary_allocator(std::size_t size) noexcept
: marker_(temporary_stack.create(size).top()), prev_(nullptr), unwind_(true)
{
    top_ = this;
}

thread_local const temporary_allocator* temporary_allocator::top_ = nullptr;

std::size_t allocator_traits<temporary_allocator>::max_node_size(const allocator_type &) noexcept
{
    return temporary_stack.get().next_capacity();
}
