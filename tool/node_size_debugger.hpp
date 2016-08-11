// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_TOOL_NODE_SIZE_DEBUGGER_HPP
#define FOONATHAN_MEMORY_TOOL_NODE_SIZE_DEBUGGER_HPP

#include <algorithm>
#include <memory>
#include <tuple>
#include <type_traits>

#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

// Obtains the node size for a container.
// Since the node type is private to the implementation,
// it cannot be accessed directly.
// It is only available to the allocator through rebinding.
// The allocator simply stores the size of the biggest type, it is rebound to,
// as long as it is not the TestType, the actual value_type of the container.
template <typename T, typename TestType>
class node_size_debugger : public std::allocator<T>
{
public:
    template <typename Other>
    struct rebind
    {
        using other = node_size_debugger<Other, TestType>;
    };

    node_size_debugger() : size_(0u)
    {
        if (!std::is_same<T, TestType>::value)
            size_ = std::max(size_, sizeof(T));
    }

    template <typename U>
    node_size_debugger(node_size_debugger<U, TestType> other) : size_(other.size_)
    {
        if (!std::is_same<T, TestType>::value)
            size_ = std::max(size_, sizeof(T));
    }

    std::size_t node_size() const
    {
        return size_;
    }

private:
    std::size_t size_;

    template <typename U, typename TT>
    friend class node_size_debugger;
};

struct hash
{
    // note: not noexcept! this leads to a cached hash value
    template <typename T>
    std::size_t operator()(const T&) const
    {
        // quality doesn't matter
        return 0;
    }
};

struct debug_forward_list
{
    const char* name() const
    {
        return "forward_list";
    }

    template <typename T>
    std::size_t debug()
    {
        std::forward_list<T, node_size_debugger<T, T>> list;
        list.push_front(T());
        list.push_front(T());
        list.push_front(T());
        return list.get_allocator().node_size() - sizeof(T);
    }
};

struct debug_list
{
    const char* name() const
    {
        return "list";
    }

    template <typename T>
    std::size_t debug()
    {
        std::list<T, node_size_debugger<T, T>> list;
        list.push_front(T());
        list.push_front(T());
        list.push_front(T());
        return list.get_allocator().node_size() - sizeof(T);
    }
};

struct debug_set
{
    const char* name() const
    {
        return "set";
    }

    template <typename T>
    std::size_t debug()
    {
        std::set<T, std::less<T>, node_size_debugger<T, T>> set;
        set.insert(T());
        set.insert(T());
        set.insert(T());
        return set.get_allocator().node_size() - sizeof(T);
    }
};

struct debug_multiset
{
    const char* name() const
    {
        return "multiset";
    }

    template <typename T>
    std::size_t debug()
    {
        std::multiset<T, std::less<T>, node_size_debugger<T, T>> set;
        set.insert(T());
        set.insert(T());
        set.insert(T());
        return set.get_allocator().node_size() - sizeof(T);
    }
};

struct debug_unordered_set
{
    const char* name() const
    {
        return "unordered_set";
    }

    template <typename T>
    std::size_t debug()
    {
        std::unordered_set<T, hash, std::equal_to<T>, node_size_debugger<T, T>> set;
        set.insert(T());
        set.insert(T());
        set.insert(T());
        return set.get_allocator().node_size() - sizeof(T);
    }
};

struct debug_unordered_multiset
{
    const char* name() const
    {
        return "unordered_multiset";
    }

    template <typename T>
    std::size_t debug()
    {
        std::unordered_multiset<T, hash, std::equal_to<T>, node_size_debugger<T, T>> set;
        set.insert(T());
        set.insert(T());
        set.insert(T());
        return set.get_allocator().node_size() - sizeof(T);
    }
};

struct debug_map
{
    const char* name() const
    {
        return "map";
    }

    template <typename T>
    std::size_t debug()
    {
        using type = std::pair<const T, T>;
        std::map<T, T, std::less<T>, node_size_debugger<type, type>> map;
        map.insert(std::make_pair(T(), T()));
        map.insert(std::make_pair(T(), T()));
        map.insert(std::make_pair(T(), T()));
        return map.get_allocator().node_size() - sizeof(typename decltype(map)::value_type);
    }
};

struct debug_multimap
{
    const char* name() const
    {
        return "multimap";
    }

    template <typename T>
    std::size_t debug()
    {
        using type = std::pair<const T, T>;
        std::multimap<T, T, std::less<T>, node_size_debugger<type, type>> map;
        map.insert(std::make_pair(T(), T()));
        map.insert(std::make_pair(T(), T()));
        map.insert(std::make_pair(T(), T()));
        return map.get_allocator().node_size() - sizeof(typename decltype(map)::value_type);
    }
};

struct debug_unordered_map
{
    const char* name() const
    {
        return "unordered_map";
    }

    template <typename T>
    std::size_t debug()
    {
        using type = std::pair<const T, T>;
        std::unordered_map<T, T, hash, std::equal_to<T>, node_size_debugger<type, type>> map;
        map.insert(std::make_pair(T(), T()));
        map.insert(std::make_pair(T(), T()));
        map.insert(std::make_pair(T(), T()));
        return map.get_allocator().node_size() - sizeof(typename decltype(map)::value_type);
    }
};

struct debug_unordered_multimap
{
    const char* name() const
    {
        return "unordered_multimap";
    }

    template <typename T>
    std::size_t debug()
    {
        using type = std::pair<const T, T>;
        std::unordered_multimap<T, T, hash, std::equal_to<T>, node_size_debugger<type, type>> map;
        map.insert(std::make_pair(T(), T()));
        map.insert(std::make_pair(T(), T()));
        map.insert(std::make_pair(T(), T()));
        return map.get_allocator().node_size() - sizeof(typename decltype(map)::value_type);
    }
};

template <typename T, class Debugger>
std::size_t debug_single(Debugger debugger)
{
    return debugger.template debug<T>();
}

// All fundamental types that don't guarantee to have the same alignment (like int and unsigned int).
// It thus covers all fundamental alignments and all possible node sizes.
// Does not support extended alignments!
using test_types = std::tuple<char, bool, short, int, long, long long, float, double, long double>;

// Maps the alignment of the test types to the base size of the node.
// The base size of the node is the node size obtained via the allocator
// but without the storage for the value type.
// It is only dependent on the alignment of the value type.
using node_size_map = std::map<std::size_t, std::size_t>;

struct debug_result
{
    const char*   container_name;
    node_size_map node_sizes;
};

template <class Debugger, typename... Types>
node_size_map debug_impl(Debugger debugger, std::tuple<Types...>)
{
    node_size_map result;
    int           dummy[] = {(result[alignof(Types)] = debug_single<Types>(debugger), 0)...};
    (void)dummy;
    return result;
}

template <class Debugger>
debug_result debug(Debugger debugger)
{
    return {debugger.name(), debug_impl(debugger, test_types{})};
}

#endif //FOONATHAN_MEMORY_TOOL_NODE_SIZE_DEBUGGER_HPP
