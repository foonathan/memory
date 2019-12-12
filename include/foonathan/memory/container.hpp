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
#include <scoped_allocator>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "std_allocator.hpp"
#include "threading.hpp"

namespace foonathan
{
    namespace memory
    {
        /// \ingroup memory adapter
        /// @{

        /// Alias template for an STL container that uses a certain
        /// \concept{concept_rawallocator,RawAllocator}. It is just a shorthand for a passing in the \c
        /// RawAllocator wrapped in a \ref foonathan::memory::std_allocator.
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(vector, std::vector<T, std_allocator<T, RawAllocator>>);

        /// Same as above but uses \c std::scoped_allocator_adaptor so the allocator is inherited by all
        /// nested containers.
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            vector_scoped_alloc,
            std::vector<T, std::scoped_allocator_adaptor<std_allocator<T, RawAllocator>>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(deque, std::deque<T, std_allocator<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            deque_scoped_alloc,
            std::deque<T, std::scoped_allocator_adaptor<std_allocator<T, RawAllocator>>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(list, std::list<T, std_allocator<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            list_scoped_alloc,
            std::list<T, std::scoped_allocator_adaptor<std_allocator<T, RawAllocator>>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(forward_list,
                                 std::forward_list<T, std_allocator<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            forward_list_scoped_alloc,
            std::forward_list<T, std::scoped_allocator_adaptor<std_allocator<T, RawAllocator>>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(set, std::set<T, std::less<T>, std_allocator<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            set_scoped_alloc,
            std::set<T, std::less<T>,
                     std::scoped_allocator_adaptor<std_allocator<T, RawAllocator>>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(multiset,
                                 std::multiset<T, std::less<T>, std_allocator<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            multiset_scoped_alloc,
            std::multiset<T, std::less<T>,
                          std::scoped_allocator_adaptor<std_allocator<T, RawAllocator>>>);

        /// \copydoc vector
        template <typename Key, typename Value, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            map, std::map<Key, Value, std::less<Key>,
                          std_allocator<std::pair<const Key, Value>, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename Key, typename Value, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            map_scoped_alloc,
            std::map<Key, Value, std::less<Key>,
                     std::scoped_allocator_adaptor<
                         std_allocator<std::pair<const Key, Value>, RawAllocator>>>);

        /// \copydoc vector
        template <typename Key, typename Value, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            multimap, std::multimap<Key, Value, std::less<Key>,
                                    std_allocator<std::pair<const Key, Value>, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename Key, typename Value, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            multimap_scoped_alloc,
            std::multimap<Key, Value, std::less<Key>,
                          std::scoped_allocator_adaptor<
                              std_allocator<std::pair<const Key, Value>, RawAllocator>>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            unordered_set,
            std::unordered_set<T, std::hash<T>, std::equal_to<T>, std_allocator<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            unordered_set_scoped_alloc,
            std::unordered_set<T, std::hash<T>, std::equal_to<T>,
                               std::scoped_allocator_adaptor<std_allocator<T, RawAllocator>>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(unordered_multiset,
                                 std::unordered_multiset<T, std::hash<T>, std::equal_to<T>,
                                                         std_allocator<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            unordered_multiset_scoped_alloc,
            std::unordered_multiset<T, std::hash<T>, std::equal_to<T>,
                                    std::scoped_allocator_adaptor<std_allocator<T, RawAllocator>>>);

        /// \copydoc vector
        template <typename Key, typename Value, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            unordered_map,
            std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                               std_allocator<std::pair<const Key, Value>, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename Key, typename Value, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            unordered_map_scoped_alloc,
            std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                               std::scoped_allocator_adaptor<
                                   std_allocator<std::pair<const Key, Value>, RawAllocator>>>);

        /// \copydoc vector
        template <typename Key, typename Value, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            unordered_multimap,
            std::unordered_multimap<Key, Value, std::hash<Key>, std::equal_to<Key>,
                                    std_allocator<std::pair<const Key, Value>, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename Key, typename Value, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            unordered_multimap_scoped_alloc,
            std::unordered_multimap<Key, Value, std::hash<Key>, std::equal_to<Key>,
                                    std::scoped_allocator_adaptor<
                                        std_allocator<std::pair<const Key, Value>, RawAllocator>>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(stack, std::stack<T, deque<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(stack_scoped_alloc,
                                 std::stack<T, deque_scoped_alloc<T, RawAllocator>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(queue, std::queue<T, deque<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(queue_scoped_alloc,
                                 std::queue<T, deque_scoped_alloc<T, RawAllocator>>);

        /// \copydoc vector
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(priority_queue, std::priority_queue<T, deque<T, RawAllocator>>);
        /// \copydoc vector_scoped_alloc_alloc
        template <typename T, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(priority_queue_scoped_alloc,
                                 std::priority_queue<T, deque_scoped_alloc<T, RawAllocator>>);

        /// \copydoc vector
        template <class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            string,
            std::basic_string<char, std::char_traits<char>, std_allocator<char, RawAllocator>>);
        /// @}

        /// @{
        /// Convenience function to create a container adapter using a certain
        /// \concept{concept_rawallocator,RawAllocator}. \returns An empty adapter with an
        /// implementation container using a reference to a given allocator. \ingroup memory adapter
        template <typename T, class RawAllocator, class Container = deque<T, RawAllocator>>
        std::stack<T, Container> make_stack(RawAllocator& allocator)
        {
            return std::stack<T, Container>{Container(allocator)};
        }

        /// \copydoc make_stack
        template <typename T, class RawAllocator, class Container = deque<T, RawAllocator>>
        std::queue<T, Container> make_queue(RawAllocator& allocator)
        {
            return std::queue<T, Container>{Container(allocator)};
        }

        /// \copydoc make_stack
        template <typename T, class RawAllocator, class Container = deque<T, RawAllocator>,
                  class Compare = std::less<T>>
        std::priority_queue<T, Container, Compare> make_priority_queue(RawAllocator& allocator,
                                                                       Compare       comp = {})
        {
            return std::priority_queue<T, Container, Compare>{detail::move(comp),
                                                              Container(allocator)};
        }
        /// @}

#if !defined(DOXYGEN)

#include "detail/container_node_sizes.hpp"

#if !defined(FOONATHAN_MEMORY_NO_NODE_SIZE)
        /// \exclude
        namespace detail
        {
            template <typename T, class StdAllocator>
            struct shared_ptr_node_size
            {
                static_assert(sizeof(T) != sizeof(T), "unsupported allocator type");
            };

            template <typename T, class RawAllocator>
            struct shared_ptr_node_size<T, std_allocator<T, RawAllocator>>
            : std::conditional<allocator_traits<RawAllocator>::is_stateful::value,
                               memory::shared_ptr_stateful_node_size<T>,
                               memory::shared_ptr_stateless_node_size<T>>::type
            {
                static_assert(sizeof(std_allocator<T, RawAllocator>) <= sizeof(void*),
                              "fix node size debugger");
            };

        } // namespace detail

        template <typename T, class StdAllocator>
        struct shared_ptr_node_size : detail::shared_ptr_node_size<T, StdAllocator>
        {
        };
#endif

#else
        /// \ingroup memory adapter
        /// @{

        /// Contains the node size of a node based STL container with a specific type.
        /// These classes are auto-generated and only available if the tools are build and without
        /// cross-compiling.
        template <typename T>
        struct forward_list_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct list_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct set_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct multiset_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct unordered_set_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct unordered_multiset_node_size
        : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct map_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct multimap_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct unordered_map_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T>
        struct unordered_multimap_node_size
        : std::integral_constant<std::size_t, implementation_defined>
        {
        };

        /// \copydoc forward_list_node_size
        template <typename T, class StdAllocator>
        struct shared_ptr_node_size : std::integral_constant<std::size_t, implementation_defined>
        {
        };
/// @}
#endif

#if !defined(FOONATHAN_MEMORY_NO_NODE_SIZE)
        /// The node size required by \ref allocate_shared.
        /// \note This is similar to \ref shared_ptr_node_size but takes a
        /// \concept{concept_rawallocator,RawAllocator} instead.
        template <typename T, class RawAllocator>
        struct allocate_shared_node_size : shared_ptr_node_size<T, std_allocator<T, RawAllocator>>
        {
        };
#endif
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_CONTAINER_HPP_INCLUDED
