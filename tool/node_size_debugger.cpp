// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>
#include <string>

#include "node_size_debugger.hpp"

struct simple_serializer
{
    std::ostream &out;

    void operator()(const debug_result &result) const
    {
        out << result.container_name << ":\n";
        for (auto pair : result.node_sizes)
            out << '\t' << pair.first << '=' << pair.second << '\n';
    }
};

using debuggers = std::tuple<debug_forward_list, debug_list,
                             debug_set, debug_multiset, debug_unordered_set, debug_unordered_multiset,
                             debug_map, debug_multimap, debug_unordered_map, debug_unordered_multimap>;

template <class Debugger, class Serializer>
void serialize_single(const Serializer &serialize)
{
    serialize(debug(Debugger{}));
}

template <class Serializer, typename ... Debuggers>
void serialize_impl(const Serializer &serializer, std::tuple<Debuggers...>)
{
    int dummy[] = {(serialize_single<Debuggers>(serializer), 0)...};
    (void)dummy;
}

template <class Serializer>
void serialize(const Serializer &serializer)
{
    serialize_impl(serializer, debuggers{});
}

int main()
{
    serialize(simple_serializer{std::cout});
}
