// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED
#define FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED

/// \file
/// Aliasas for STL containers using a certain \concept{concept_rawallocator,RawAllocator}.
/// \note Only available on a hosted implementation.

#include "config.hpp"
#if !FOONATHAN_HOSTED_IMPLEMENTATION
    #error "This header is only available for a hosted implementation."
#endif

#include <functional>
#include <utility>

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
#include "threading.hpp"

namespace foonathan { namespace memory
{
    /// \addtogroup memory
    /// @{

    /// Alias template for an STL container that uses a certain \concept{concept_rawallocator,RawAllocator}.
    /// It is just a shorthand for a passing in the \c RawAllocator wrapped in a \ref foonathan::memory::std_allocator.
    /// The \c Mutex type defaults to the \ref foonathan::memory::default_mutex.
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(vector, std::vector<T, std_allocator<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(deque, std::deque<T, std_allocator<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(list, std::list<T, std_allocator<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(forward_list, std::forward_list<T, std_allocator<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(set, std::set<T, std::less<T>, std_allocator<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(multiset, std::multiset<T, std::less<T>, std_allocator<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename Key, typename Value, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(map, std::map<Key, Value, std::less<Key>,
                    std_allocator<std::pair<const Key, Value>, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename Key, typename Value, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(multimap, std::multimap<Key, Value, std::less<Key>,
                    std_allocator<std::pair<const Key, Value>, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(unordered_set, std::unordered_set<T, std::hash<T>, std::equal_to<T>,
                        std_allocator<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(unordered_multiset, std::unordered_multiset<T, std::hash<T>, std::equal_to<T>,
                        std_allocator<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename Key, typename Value, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(unordered_map, std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                        std_allocator<std::pair<const Key, Value>, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename Key, typename Value, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(unordered_multimap, std::unordered_multimap<Key, std::hash<Key>, std::equal_to<Key>,
                        std_allocator<std::pair<const Key, Value>, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(stack, std::stack<T, deque<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(queue, std::queue<T, deque<T, RawAllocator, Mutex>>);

    /// \copydoc vector
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(priority_queue, std::priority_queue<T, deque<T, RawAllocator, Mutex>>);
    /// @}

    /// @{
    /// Convenience function to create a container adapter using a certain \concept{concept_rawallocator,RawAllocator}.
    /// \returns An empty adapter with an implementation container using a reference to a given allocator.
    /// \ingroup memory
    template <typename T, class RawAllocator,
            class Container = deque<T, RawAllocator>>
    std::stack<T, Container> make_stack(RawAllocator &allocator)
    {
        return std::stack<T, Container>{Container(allocator)};
    }

    /// \copydoc make_stack
    template <typename T, class RawAllocator,
            class Container = deque<T, RawAllocator>>
    std::queue<T, Container> make_queue(RawAllocator &allocator)
    {
        return std::queue<T, Container>{Container(allocator)};
    }

    /// \copydoc make_stack
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

#if !defined(DOXYGEN)
    #define FOONATHAN_MEMORY_IMPL_IN_CONTAINER_HPP
    #include "container_node_sizes.hpp"
    #undef FOONATHAN_MEMORY_IMPL_IN_CONTAINER_HPP
#else
    /// \addtogroup memory
    /// @{

    /// Contains the node size of a node based STL container with a specific type.
    /// These classes are auto-generated and only available if the tools are build and without cross-compiling.
    template <typename T>
    struct forward_list_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct list_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct set_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct multiset_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct unordered_set_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct unordered_multiset_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct map_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct multimap_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct unordered_map_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};

    /// \copydoc forward_list_node_size
    template <typename T>
    struct unordered_multimap_node_size
    : std::integral_constant<std::size_t, implementation_defined>
    {};
    /// @}
#endif
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED
