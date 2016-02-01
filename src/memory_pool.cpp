// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_pool.hpp"

using namespace foonathan::memory;

template class memory_pool<node_pool>;
template class memory_pool<array_pool>;
template class memory_pool<small_node_pool>;

template class allocator_traits<memory_pool<node_pool>>;
template class allocator_traits<memory_pool<array_pool>>;
template class allocator_traits<memory_pool<small_node_pool>>;
