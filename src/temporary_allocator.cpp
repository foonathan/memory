// Copyright (C) 2015-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "temporary_allocator.hpp"

#include <new>
#include <type_traits>

#include "detail/assert.hpp"
#include "default_allocator.hpp"
#include "error.hpp"

using namespace foonathan::memory;

namespace
{
    void default_growth_tracker(std::size_t) noexcept {}

    using temporary_impl_allocator        = default_allocator;
    using temporary_impl_allocator_traits = allocator_traits<temporary_impl_allocator>;
} // namespace

detail::temporary_block_allocator::temporary_block_allocator(std::size_t block_size) noexcept
: tracker_(default_growth_tracker), block_size_(block_size)
{
}

detail::temporary_block_allocator::growth_tracker detail::temporary_block_allocator::
    set_growth_tracker(growth_tracker t) noexcept
{
    auto old = tracker_;
    tracker_ = t;
    return old;
}

detail::temporary_block_allocator::growth_tracker detail::temporary_block_allocator::
    get_growth_tracker() noexcept
{
    return tracker_;
}

memory_block detail::temporary_block_allocator::allocate_block()
{
    auto alloc  = temporary_impl_allocator();
    auto memory = temporary_impl_allocator_traits::allocate_array(alloc, block_size_, 1,
                                                                  detail::max_alignment);
    auto block  = memory_block(memory, block_size_);
    block_size_ = std::size_t(block_size_
                              * growing_block_allocator<temporary_impl_allocator>::growth_factor());
    return block;
}

void detail::temporary_block_allocator::deallocate_block(memory_block block)
{
    auto alloc = temporary_impl_allocator();
    temporary_impl_allocator_traits::deallocate_array(alloc, block.memory, block.size, 1,
                                                      detail::max_alignment);
}

#if FOONATHAN_MEMORY_TEMPORARY_STACK_MODE >= 2
// lifetime managment through the nifty counter and the list
// note: I could have used a simple `thread_local` variable for the temporary stack
// but this could lead to issues with destruction order
// and more importantly I have to support platforms that can't handle non-trivial thread local's
// hence I need to dynamically allocate the stack's and store them in a container
// on program exit the container is iterated and all stack's are properly destroyed
// if a thread exit can be detected, the dynamic memory of the stack is already released,
// but not the stack itself destroyed

#if !defined(__MINGW64__)

// only use the thread exit detector if we have thread local and are not running on MinGW due to a bug
// see: https://sourceforge.net/p/mingw-w64/bugs/527/
#define FOONATHAN_MEMORY_THREAD_EXIT_DETECTOR 1

#else
#define FOONATHAN_MEMORY_THREAD_EXIT_DETECTOR 0

#if defined(_MSC_VER)
#pragma message(                                                                                   \
    "thread_local doesn't support destructors, need to use the temporary_stack_initializer to ensure proper cleanup of the temporary memory")
#else
#warning                                                                                           \
    "thread_local doesn't support destructors, need to use the temporary_stack_initializer to ensure proper cleanup of the temporary memory"
#endif

#endif

static class detail::temporary_stack_list
{
public:
    std::atomic<temporary_stack_list_node*> first;

    temporary_stack* create_new(std::size_t size)
    {
        auto storage =
            default_allocator().allocate_node(sizeof(temporary_stack), alignof(temporary_stack));
        return ::new (storage) temporary_stack(0, size);
    }

    temporary_stack* find_unused()
    {
        for (auto ptr = first.load(); ptr; ptr = ptr->next_)
        {
            auto value = false;
            if (ptr->in_use_.compare_exchange_strong(value, true))
                return static_cast<temporary_stack*>(ptr);
        }

        return nullptr;
    }

    temporary_stack* create(std::size_t size)
    {
        if (auto ptr = find_unused())
        {
            FOONATHAN_MEMORY_ASSERT(ptr->in_use_);
            ptr->stack_ = detail::temporary_stack_impl(size);
            return ptr;
        }
        return create_new(size);
    }

    void clear(temporary_stack& stack)
    {
        // stack should be empty now, so shrink_to_fit() clears all memory
        stack.stack_.shrink_to_fit();
        stack.in_use_ = false; // mark as free
    }

    void destroy()
    {
        for (auto ptr = first.exchange(nullptr); ptr;)
        {
            auto stack = static_cast<temporary_stack*>(ptr);
            auto next  = ptr->next_;

            stack->~temporary_stack();
            default_allocator().deallocate_node(stack, sizeof(temporary_stack),
                                                alignof(temporary_stack));

            ptr = next;
        }

        FOONATHAN_MEMORY_ASSERT_MSG(!first.load(),
                                    "destroy() called while other threads are still running");
    }
} temporary_stack_list_obj;

namespace
{
    thread_local std::size_t      nifty_counter;
    thread_local temporary_stack* temp_stack = nullptr;

#if FOONATHAN_MEMORY_THREAD_EXIT_DETECTOR
    // don't use this on a bug
    thread_local struct thread_exit_detector_t
    {
        ~thread_exit_detector_t() noexcept
        {
            if (temp_stack)
                // clear automatically on thread exit, as the initializer's destructor does
                // note: if another's thread_local variable destructor is called after this one
                // and that destructor uses the temporary allocator
                // the stack needs to grow again
                // but who does temporary allocation in a destructor?!
                temporary_stack_list_obj.clear(*temp_stack);
        }
    } thread_exit_detector;
#endif
} // namespace

detail::temporary_stack_list_node::temporary_stack_list_node(int) noexcept : in_use_(true)
{
    next_ = temporary_stack_list_obj.first.load();
    while (!temporary_stack_list_obj.first.compare_exchange_weak(next_, this))
        ;
#if FOONATHAN_MEMORY_THREAD_EXIT_DETECTOR
    (void)&thread_exit_detector; // ODR-use it, so it will be created
#endif
}

detail::temporary_allocator_dtor_t::temporary_allocator_dtor_t() noexcept
{
    ++nifty_counter;
}

detail::temporary_allocator_dtor_t::~temporary_allocator_dtor_t() noexcept
{
    if (--nifty_counter == 0u && temp_stack)
        temporary_stack_list_obj.destroy();
}

temporary_stack_initializer::temporary_stack_initializer(std::size_t initial_size)
{
    if (!temp_stack)
        temp_stack = temporary_stack_list_obj.create(initial_size);
}

temporary_stack_initializer::~temporary_stack_initializer() noexcept
{
    // don't destroy, nifty counter does that
    // but can get rid of all the memory
    if (temp_stack)
        temporary_stack_list_obj.clear(*temp_stack);
}

temporary_stack& foonathan::memory::get_temporary_stack(std::size_t initial_size)
{
    if (!temp_stack)
        temp_stack = temporary_stack_list_obj.create(initial_size);
    return *temp_stack;
}

#elif FOONATHAN_MEMORY_TEMPORARY_STACK_MODE == 1

namespace
{
    FOONATHAN_THREAD_LOCAL alignas(
        temporary_stack) char temporary_stack_storage[sizeof(temporary_stack)];
    FOONATHAN_THREAD_LOCAL bool is_created = false;

    temporary_stack& get() noexcept
    {
        FOONATHAN_MEMORY_ASSERT(is_created);
        return *static_cast<temporary_stack*>(static_cast<void*>(&temporary_stack_storage));
    }

    void create(std::size_t initial_size)
    {
        if (!is_created)
        {
            ::new (static_cast<void*>(&temporary_stack_storage)) temporary_stack(initial_size);
            is_created = true;
        }
    }
} // namespace

// explicit lifetime managment
temporary_stack_initializer::temporary_stack_initializer(std::size_t initial_size)
{
    create(initial_size);
}

temporary_stack_initializer::~temporary_stack_initializer()
{
    if (is_created)
        get().~temporary_stack();
}

temporary_stack& foonathan::memory::get_temporary_stack(std::size_t initial_size)
{
    create(initial_size);
    return get();
}

#else

// no lifetime managment

temporary_stack_initializer::temporary_stack_initializer(std::size_t initial_size)
{
    if (initial_size != 0u)
        FOONATHAN_MEMORY_WARNING("temporary_stack_initializer() has no effect if "
                                 "FOONATHAN_MEMORY_TEMPORARY_STACK == 0 (pass an initial size of 0 "
                                 "to disable this message)");
}

temporary_stack_initializer::~temporary_stack_initializer() {}

temporary_stack& foonathan::memory::get_temporary_stack(std::size_t)
{
    FOONATHAN_MEMORY_UNREACHABLE("get_temporary_stack() called but stack is disabled by "
                                 "FOONATHAN_MEMORY_TEMPORARY_STACK == 0");
    std::abort();
}

#endif

const temporary_stack_initializer::defer_create_t temporary_stack_initializer::defer_create;

temporary_allocator::temporary_allocator() : temporary_allocator(get_temporary_stack()) {}

temporary_allocator::temporary_allocator(temporary_stack& stack)
: unwind_(stack), prev_(stack.top_), shrink_to_fit_(false)
{
    FOONATHAN_MEMORY_ASSERT(!prev_ || prev_->is_active());
    stack.top_ = this;
}

temporary_allocator::~temporary_allocator() noexcept
{
    if (is_active())
    {
        auto& stack = unwind_.get_stack();
        stack.top_  = prev_;
        unwind_.unwind(); // manually call it now...
        if (shrink_to_fit_)
            // to call shrink_to_fit() afterwards
            stack.stack_.shrink_to_fit();
    }
}

void* temporary_allocator::allocate(std::size_t size, std::size_t alignment)
{
    FOONATHAN_MEMORY_ASSERT_MSG(is_active(), "object isn't the active allocator");
    return unwind_.get_stack().stack_.allocate(size, alignment);
}

void temporary_allocator::shrink_to_fit() noexcept
{
    shrink_to_fit_ = true;
}

bool temporary_allocator::is_active() const noexcept
{
    FOONATHAN_MEMORY_ASSERT(unwind_.will_unwind());
    auto res = unwind_.get_stack().top_ == this;
    // check that prev is actually before this
    FOONATHAN_MEMORY_ASSERT(!res || !prev_ || prev_->unwind_.get_marker() <= unwind_.get_marker());
    return res;
}
