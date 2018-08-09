// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "new_allocator.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
#include <memory>
#endif

#include <new>
#include <foonathan/get_new_handler.hpp>

#include "error.hpp"

using namespace foonathan::memory;

allocator_info detail::new_allocator_impl::info() FOONATHAN_NOEXCEPT
{
    return {FOONATHAN_MEMORY_LOG_PREFIX "::new_allocator", nullptr};
}

void* detail::new_allocator_impl::allocate(std::size_t size, size_t) FOONATHAN_NOEXCEPT
{
    void* memory = nullptr;
    while (true)
    {
        memory = ::operator new(size, std::nothrow);
        if (memory)
            break;

        auto handler = foonathan_comp::get_new_handler();
        if (handler)
        {
            FOONATHAN_TRY
            {
                handler();
            }
            FOONATHAN_CATCH_ALL
            {
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }
    return memory;
}

void detail::new_allocator_impl::deallocate(void* ptr, std::size_t, size_t) FOONATHAN_NOEXCEPT
{
    ::operator delete(ptr);
}

std::size_t detail::new_allocator_impl::max_node_size() FOONATHAN_NOEXCEPT
{
#if FOONATHAN_HOSTED_IMPLEMENTATION
    return std::allocator_traits<std::allocator<char>>::max_size({});
#else
    return std::size_t(-1);
#endif
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class detail::lowlevel_allocator<detail::new_allocator_impl>;
template class foonathan::memory::allocator_traits<new_allocator>;
#endif
