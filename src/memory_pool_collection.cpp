// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_pool_collection.hpp"

using namespace foonathan::memory;

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
    template class memory_pool_collection<node_pool, identity_buckets>;
    template class memory_pool_collection<array_pool, identity_buckets>;
    template class memory_pool_collection<small_node_pool, identity_buckets>;

    template class memory_pool_collection<node_pool, log2_buckets>;
    template class memory_pool_collection<array_pool, log2_buckets>;
    template class memory_pool_collection<small_node_pool, log2_buckets>;

    template class allocator_traits<memory_pool_collection<node_pool, identity_buckets>>;
    template class allocator_traits<memory_pool_collection<array_pool, identity_buckets>>;
    template class allocator_traits<memory_pool_collection<small_node_pool, identity_buckets>>;

    template class allocator_traits<memory_pool_collection<node_pool, log2_buckets>>;
    template class allocator_traits<memory_pool_collection<array_pool, log2_buckets>>;
    template class allocator_traits<memory_pool_collection<small_node_pool, log2_buckets>>;
#endif
