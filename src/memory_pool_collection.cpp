// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_pool_collection.hpp"

#include "debugging.hpp"

using namespace foonathan::memory;

void detail::memory_pool_collection_leak_handler::operator()(std::ptrdiff_t amount)
{
    get_leak_handler()({FOONATHAN_MEMORY_LOG_PREFIX "::memory_pool_collection", this}, amount);
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class foonathan::memory::memory_pool_collection<node_pool, identity_buckets>;
template class foonathan::memory::memory_pool_collection<array_pool, identity_buckets>;
template class foonathan::memory::memory_pool_collection<small_node_pool, identity_buckets>;

template class foonathan::memory::memory_pool_collection<node_pool, log2_buckets>;
template class foonathan::memory::memory_pool_collection<array_pool, log2_buckets>;
template class foonathan::memory::memory_pool_collection<small_node_pool, log2_buckets>;

template class foonathan::memory::allocator_traits<
    memory_pool_collection<node_pool, identity_buckets>>;
template class foonathan::memory::allocator_traits<
    memory_pool_collection<array_pool, identity_buckets>>;
template class foonathan::memory::allocator_traits<
    memory_pool_collection<small_node_pool, identity_buckets>>;

template class foonathan::memory::allocator_traits<memory_pool_collection<node_pool, log2_buckets>>;
template class foonathan::memory::allocator_traits<
    memory_pool_collection<array_pool, log2_buckets>>;
template class foonathan::memory::allocator_traits<
    memory_pool_collection<small_node_pool, log2_buckets>>;

template class foonathan::memory::composable_allocator_traits<
    memory_pool_collection<node_pool, identity_buckets>>;
template class foonathan::memory::composable_allocator_traits<
    memory_pool_collection<array_pool, identity_buckets>>;
template class foonathan::memory::composable_allocator_traits<
    memory_pool_collection<small_node_pool, identity_buckets>>;

template class foonathan::memory::composable_allocator_traits<
    memory_pool_collection<node_pool, log2_buckets>>;
template class foonathan::memory::composable_allocator_traits<
    memory_pool_collection<array_pool, log2_buckets>>;
template class foonathan::memory::composable_allocator_traits<
    memory_pool_collection<small_node_pool, log2_buckets>>;
#endif
