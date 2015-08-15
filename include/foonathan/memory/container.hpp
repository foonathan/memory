// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED
#define FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED

/// \file
/// \brief Aliasas for STL containers to apply \c RawAllocator more easily.
/// \note Only available on a hosted implementation.

#include "config.hpp"
#if !FOONATHAN_HOSTED_IMPLEMENTATION
    #error "This header is only available for a hosted implementation."
#endif

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

#include "std_allocator.hpp"

namespace foonathan { namespace memory
{
    /// @{
    /// \brief Aliases for STL containers using a \c RawAllocator.
    /// \details It is just a shorthand to pass a \c RawAllocator to a container.
    /// \ingroup memory
#define FOONATHAN_MEMORY_IMPL_CONTAINER1(Name) \
    template <typename T, class RawAllocator, class Mutex = default_mutex> \
    using Name = std::Name<T, std_allocator<T, RawAllocator, Mutex>>;

    FOONATHAN_MEMORY_IMPL_CONTAINER1(vector)
    FOONATHAN_MEMORY_IMPL_CONTAINER1(deque)
    FOONATHAN_MEMORY_IMPL_CONTAINER1(list)
    FOONATHAN_MEMORY_IMPL_CONTAINER1(forward_list)

#undef FOONATHAN_MEMORY_IMPL_CONTAINER1

    template <typename T, class RawAllocator, class Mutex = default_mutex>
    using set = std::set<T, std::less<T>, std_allocator<T, RawAllocator, Mutex>>;

    template <typename T, class RawAllocator, class Mutex = default_mutex>
    using multiset = std::multiset<T, std::less<T>, std_allocator<T, RawAllocator, Mutex>>;

    template <typename Key, typename Value, class RawAllocator, class Mutex = default_mutex>
    using map = std::map<Key, Value, std::less<Key>,
                    std_allocator<std::pair<const Key, Value>, RawAllocator, Mutex>>;

    template <typename Key, typename Value, class RawAllocator, class Mutex = default_mutex>
    using multimap = std::multimap<Key, Value, std::less<Key>,
                    std_allocator<std::pair<const Key, Value>, RawAllocator, Mutex>>;

    template <typename T, class RawAllocator, class Mutex = default_mutex>
    using unordered_set = std::unordered_set<T, std::hash<T>, std::equal_to<T>,
                        std_allocator<T, RawAllocator, Mutex>>;

    template <typename T, class RawAllocator, class Mutex = default_mutex>
    using unordered_multiset = std::unordered_multiset<T, std::hash<T>, std::equal_to<T>,
                        std_allocator<T, RawAllocator, Mutex>>;

    template <typename Key, typename Value, class RawAllocator, class Mutex = default_mutex>
    using unordered_map = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                        std_allocator<std::pair<const Key, Value>, RawAllocator, Mutex>>;

    template <typename Key, typename Value, class RawAllocator, class Mutex = default_mutex>
    using unordered_multimap = std::unordered_multimap<Key, std::hash<Key>, std::equal_to<Key>,
                        std_allocator<std::pair<const Key, Value>, RawAllocator, Mutex>>;

#define FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER(Name) \
    template <typename T, class RawAllocator, class Mutex = default_mutex> \
    using Name = std::Name<T, deque<T, RawAllocator, Mutex>>;
    FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER(stack)
    FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER(queue)
    FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER(priority_queue)
#undef FOONATHAN_MEMORY_IMPL_CONTAINER_ADAPTER
    /// @}

    /// @{
    /// \brief Convienience function to create a container adapter.
    /// \details Creates this function and passes it the underlying container with certain allocator.
    /// \ingroup memory
    template <typename T, class RawAllocator,
            class Container = deque<T, RawAllocator>>
    std::stack<T, Container> make_stack(RawAllocator &allocator)
    {
        return std::stack<T, Container>{Container(allocator)};
    }

    template <typename T, class RawAllocator,
            class Container = deque<T, RawAllocator>>
    std::queue<T, Container> make_queue(RawAllocator &allocator)
    {
        return std::queue<T, Container>{Container(allocator)};
    }

    template <typename T, class RawAllocator,
            class Container = deque<T, RawAllocator>,
            class Compare = std::less<T>>
    std::priority_queue<T, Container, Compare>
        make_priority_queue(RawAllocator &allocator, Compare comp = {})
    {
        return std::priority_queue<T, Container, Compare>
                {detail::move(comp), Container(allocator)};
    }
    /// @}

    #define FOONATHAN_MEMORY_IMPL_IN_CONTAINER_HPP
    #include "container_node_sizes.hpp"
    #undef FOONATHAN_MEMORY_IMPL_IN_CONTAINER_HPP
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED
