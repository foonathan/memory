// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "new_allocator.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
#include <memory>
#endif

#include <new>

#include "error.hpp"

using namespace foonathan::memory;

allocator_info detail::new_allocator_impl::info() noexcept
{
    return {FOONATHAN_MEMORY_LOG_PREFIX "::new_allocator", nullptr};
}

void* detail::new_allocator_impl::allocate(std::size_t size, size_t) noexcept
{
    void* memory = nullptr;
    while (true)
    {
        memory = ::operator new(size, std::nothrow);
        if (memory)
            break;

        auto handler = std::get_new_handler();
        if (handler)
        {
#if FOONATHAN_HAS_EXCEPTION_SUPPORT
            try
            {
                handler();
            }
            catch (...)
            {
                return nullptr;
            }
#else
            handler();
#endif
        }
        else
        {
            return nullptr;
        }
    }
    return memory;
}

void detail::new_allocator_impl::deallocate(void* ptr, std::size_t, size_t) noexcept
{
    ::operator delete(ptr);
}

std::size_t detail::new_allocator_impl::max_node_size() noexcept
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
