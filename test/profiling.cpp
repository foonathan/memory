// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

// Profiling code to check performance of allocators.

#include <iomanip>
#include <iostream>
#include <locale>

#include "allocator_storage.hpp"
#include "heap_allocator.hpp"
#include "new_allocator.hpp"
#include "memory_pool.hpp"
#include "memory_stack.hpp"

using namespace foonathan::memory;

#include "benchmark.hpp"

template <class Func, class... Allocators>
void benchmark_node(std::size_t count, std::size_t size, Allocators&... allocators)
{
    int dummy[] = {(std::cout << benchmark(Func{count}, allocators, size) << '|', 0)...};
    (void)dummy;
    std::cout << '\n';
}

template <class Func>
void benchmark_node(std::initializer_list<std::size_t> counts,
                    std::initializer_list<std::size_t> node_sizes)
{
    std::cout << "##" << Func::name() << "\n";
    std::cout << '\n';
    std::cout << "Size|Heap|New|Small|Node|Array|Stack\n";
    std::cout << "----|-----|---|-----|----|-----|-----\n";
    for (auto count : counts)
        for (auto size : node_sizes)
        {
            auto heap_alloc = [&] { return heap_allocator{}; };
            auto new_alloc  = [&] { return new_allocator{}; };

            auto small_alloc = [&]
            { return memory_pool<small_node_pool>(size, count * size + 1024); };
            auto node_alloc = [&]
            { return memory_pool<node_pool>(size, count * std::max(size, sizeof(char*)) + 1024); };
            auto array_alloc = [&]
            { return memory_pool<array_pool>(size, count * std::max(size, sizeof(char*)) + 1024); };

            auto stack_alloc = [&] { return memory_stack<>(count * size); };

            std::cout << count << "\\*" << size << "|";
            benchmark_node<Func>(count, size, heap_alloc, new_alloc, small_alloc, node_alloc,
                                 array_alloc, stack_alloc);
        }
    std::cout << '\n';
}

template <class Func, class Second, class... Tail>
void benchmark_node(std::initializer_list<std::size_t> counts,
                    std::initializer_list<std::size_t> node_sizes)
{
    benchmark_node<Func>(counts, node_sizes);
    benchmark_node<Second, Tail...>(counts, node_sizes);
}

template <class Func, class... Allocators>
void benchmark_array(std::size_t count, std::size_t array_size, std::size_t node_size,
                     Allocators&... allocators)
{
    int dummy[] = {
        (std::cout << benchmark(Func{count}, allocators, array_size, node_size) << '|', 0)...};
    (void)dummy;
    std::cout << '\n';
}

template <class Func>
void benchmark_array(std::initializer_list<std::size_t> counts,
                     std::initializer_list<std::size_t> node_sizes,
                     std::initializer_list<std::size_t> array_sizes)
{
    using namespace foonathan::memory;
    std::cout << "##" << Func::name() << "\n";
    std::cout << '\n';
    std::cout << "Size|Heap|New|Node|Array|Stack\n";
    std::cout << "----|-----|---|----|-----|-----\n";
    for (auto count : counts)
        for (auto node_size : node_sizes)
            for (auto array_size : array_sizes)
            {
                auto mem_needed = count * std::max(node_size, sizeof(char*)) * array_size + 1024;

                auto heap_alloc = [&] { return heap_allocator{}; };
                auto new_alloc  = [&] { return new_allocator{}; };

                auto node_alloc  = [&] { return memory_pool<node_pool>(node_size, mem_needed); };
                auto array_alloc = [&] { return memory_pool<array_pool>(node_size, mem_needed); };

                auto stack_alloc = [&] { return memory_stack<>(count * mem_needed); };

                std::cout << count << "\\*" << node_size << "\\*" << array_size << "|";
                benchmark_array<Func>(count, array_size, node_size, heap_alloc, new_alloc,
                                      node_alloc, array_alloc, stack_alloc);
            }
    std::cout << '\n';
}

template <class Func, class Second, class... Tail>
void benchmark_array(std::initializer_list<std::size_t> counts,
                     std::initializer_list<std::size_t> node_sizes,
                     std::initializer_list<std::size_t> array_sizes)
{
    benchmark_array<Func>(counts, node_sizes, array_sizes);
    benchmark_array<Second, Tail...>(counts, node_sizes, array_sizes);
}

int main(int argc, char* argv[])
{
    if (argc >= 2)
        sample_size = std::size_t(std::atoi(argv[1]));

    class comma_numpunct : public std::numpunct<char>
    {
    protected:
        virtual char do_thousands_sep() const
        {
            return ',';
        }

        virtual std::string do_grouping() const
        {
            return "\03";
        }
    };

    std::cout.imbue({std::locale(), new comma_numpunct});

    std::cout << "#Node\n\n";
    benchmark_node<single, bulk, bulk_reversed, butterfly>({256, 512, 1024}, {1, 4, 8, 256});
    std::cout << "#Array\n\n";
    benchmark_array<single, bulk, bulk_reversed, butterfly>({256, 512}, {1, 4, 8}, {1, 4, 8});
}
