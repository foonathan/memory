// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "pool_type.hpp"

#include "detail/free_list.hpp"
#include "detail/small_free_list.hpp"

using namespace foonathan::memory;
using namespace detail;

void detail::insert(node_pool, free_memory_list &free_list,
                    void *ptr, std::size_t size) FOONATHAN_NOEXCEPT
{
    free_list.insert(ptr, size);
}
                    
void detail::insert(array_pool, free_memory_list &free_list,
                    void *ptr, std::size_t size) FOONATHAN_NOEXCEPT
{
    free_list.insert_ordered(ptr, size);
}
            
void detail::insert(small_node_pool, small_free_memory_list &free_list,
                    void *ptr, std::size_t size) FOONATHAN_NOEXCEPT
{
    free_list.insert(ptr, size);
}

void detail::deallocate(node_pool, free_memory_list &free_list, void *node) FOONATHAN_NOEXCEPT
{
    free_list.deallocate(node);
}

void detail::deallocate(array_pool, free_memory_list &free_list, void *node) FOONATHAN_NOEXCEPT
{
    free_list.deallocate_ordered(node);
}

void detail::deallocate(array_pool, free_memory_list &free_list, void *node, std::size_t n) FOONATHAN_NOEXCEPT
{
    free_list.deallocate_ordered(node, n);
}

void detail::deallocate(small_node_pool, small_free_memory_list &free_list, void *node) FOONATHAN_NOEXCEPT
{
    free_list.deallocate(node);
}
