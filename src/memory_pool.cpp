// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "memory_pool.hpp"

#include "debugging.hpp"

using namespace foonathan::memory;

void detail::memory_pool_leak_handler::operator()(std::ptrdiff_t amount)
{
    get_leak_handler()({FOONATHAN_MEMORY_LOG_PREFIX "::memory_pool", this}, amount);
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class foonathan::memory::memory_pool<node_pool>;
template class foonathan::memory::memory_pool<array_pool>;
template class foonathan::memory::memory_pool<small_node_pool>;

template class foonathan::memory::allocator_traits<memory_pool<node_pool>>;
template class foonathan::memory::allocator_traits<memory_pool<array_pool>>;
template class foonathan::memory::allocator_traits<memory_pool<small_node_pool>>;

template class foonathan::memory::composable_allocator_traits<memory_pool<node_pool>>;
template class foonathan::memory::composable_allocator_traits<memory_pool<array_pool>>;
template class foonathan::memory::composable_allocator_traits<memory_pool<small_node_pool>>;
#endif
