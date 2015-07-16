// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cstring>
#include <iomanip>
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

struct verbose_serializer
{
    std::ostream &out;

    void operator()(const debug_result &result) const
    {
        out << "For container '" << result.container_name << "':\n";
        for (auto pair : result.node_sizes)
            out << '\t' << "With an alignment of " << std::setw(2) << pair.first
                << " is the base node size " << std::setw(2) << pair.second << ".\n";
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

const char* exe_name = "foonathan_memory_node_size_debugger";
std::string exe_spaces(std::strlen(exe_name), ' ');

void print_help(std::ostream &out)
{
    out << "Usage: " << exe_name << " [--version][--help]\n";
    out << "       "  << exe_spaces << " [--simple][--verbose]\n";
    out << "Obtains information about the internal node sizes of the STL containers.\n";
    out << '\n';
    out << "   --simple\tprints node sizes in the form 'alignment=base-node-size'\n";
    out << "   --verbose\tprints node sizes in a more verbose form\n";
    out << "   --help\tdisplay this help and exit\n";
    out << "   --version\toutput version information and exit\n";
    out << '\n';
    out << "The base node size is the size of the node without the storage for the value type.\n"
        << "Add 'sizeof(value_type)' to the base node size for the appropriate alignment to get the whole size.\n";
    out << "With no options prints base node sizes of all containers in a simple manner.\n";
}

void print_version(std::ostream &out)
{
    out << exe_name << " version " << VERSION << '\n';
}

int print_invalid_option(std::ostream &out, const char *option)
{
    out << exe_name << ": invalid option -- '";
    while (*option == '-')
        ++option;
    out << option << "'\n";
    out << "Try '" << exe_name << " --help' for more information.\n";
    return 2;
}

int main(int argc, char *argv[])
{
    if (argc == 1 || argv[1] == std::string("--simple"))
        serialize(simple_serializer{std::cout});
    else if (argv[1] == std::string("--verbose"))
        serialize(verbose_serializer{std::cout});
    else if (argv[1] == std::string("--help"))
        print_help(std::cout);
    else if (argv[1] == std::string("--version"))
        print_version(std::cout);
    else
        return print_invalid_option(std::cout, argv[1]);
}
