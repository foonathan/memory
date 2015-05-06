// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED
#define FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED

/// \file
/// \brief Aliasas for STL containers to apply \c RawAllocator more easily.

#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "allocator_adapter.hpp"

namespace foonathan { namespace memory
{
#define FOONATHAN_MEMORY_IMPL_CONTAINER1(Name) \
    template <typename T, class RawAllocator> \
    using Name = std::Name<T, raw_allocator_allocator<T, RawAllocator>>;
    
    FOONATHAN_MEMORY_IMPL_CONTAINER1(vector)
    FOONATHAN_MEMORY_IMPL_CONTAINER1(deque)
    FOONATHAN_MEMORY_IMPL_CONTAINER1(list)
    FOONATHAN_MEMORY_IMPL_CONTAINER1(forward_list)
    
#undef FOONATHAN_MEMORY_IMPL_CONTAINER1
    
    template <typename T, class RawAllocator>
    using set = std::set<T, std::less<T>, raw_allocator_allocator<T, RawAllocator>>;
    
    template <typename T, class RawAllocator>
    using multiset = std::multiset<T, std::less<T>, raw_allocator_allocator<T, RawAllocator>>;
    
    template <typename Key, typename Value, class RawAllocator>
    using map = std::map<Key, Value, std::less<Key>,
                    raw_allocator_allocator<std::pair<const Key, Value>, RawAllocator>>;
    
    template <typename Key, typename Value, class RawAllocator>
    using multimap = std::multimap<Key, Value, std::less<Key>,
                    raw_allocator_allocator<std::pair<const Key, Value>, RawAllocator>>;
                    
    template <typename T, class RawAllocator>
    using unordered_set = std::unordered_set<T, std::hash<T>, std::equal_to<T>,
                        raw_allocator_allocator<T, RawAllocator>>;
    
    template <typename T, class RawAllocator>
    using unordered_multiset = std::unordered_multiset<T, std::hash<T>, std::equal_to<T>,
                        raw_allocator_allocator<T, RawAllocator>>;
                        
    template <typename Key, typename Value, class RawAllocator>
    using unordered_map = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                        raw_allocator_allocator<std::pair<const Key, Value>, RawAllocator>>;
    
    template <typename Key, typename Value, class RawAllocator>
    using unordered_multimap = std::unordered_multimap<Key, std::hash<Key>, std::equal_to<Key>,
                        raw_allocator_allocator<std::pair<const Key, Value>, RawAllocator>>;

#define FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER(Name) \
    template <typename T, class RawAllocator> \
    using Name = std::Name<T, deque<T, RawAllocator>>;
    FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER(stack)
    FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER(queue)
    FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER(priority_queue)
#undef FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED
