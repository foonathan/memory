// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// Profiling code to check performance of allocators.

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include "allocator_adapter.hpp"
#include "heap_allocator.hpp"
#include "new_allocator.hpp"
#include "pool_allocator.hpp"
#include "stack_allocator.hpp"

namespace memory = foonathan::memory;

using unit = std::chrono::microseconds;

template <typename F, typename ... Args>
std::size_t measure(F func, Args&&... args)
{
    auto start = std::chrono::system_clock::now();
    func(std::forward<Args>(args)...);
    auto duration = std::chrono::duration_cast<unit>
                        (std::chrono::system_clock::now() - start);
    return std::size_t(duration.count());
}

struct single
{
    std::size_t count;

    single(std::size_t c)
    : count(c) {}

    template <class RawAllocator>
    std::size_t operator()(RawAllocator &alloc, std::size_t size)
    {
        return measure([&]()
                       {
                           for (std::size_t i = 0u; i != count; ++i)
                           {
                               volatile auto ptr = alloc.allocate_node(size, 1);
                               alloc.deallocate_node(ptr, size, 1);
                           }
                       });
    }

    template <class RawAllocator>
    std::size_t operator()(RawAllocator &alloc,
                    std::size_t array_size, std::size_t node_size)
    {
        return measure([&]()
                       {
                           for (std::size_t i = 0u; i != count; ++i)
                           {
                               volatile auto ptr = alloc.allocate_array(array_size, node_size, 1);
                               alloc.deallocate_array(ptr, array_size,
                                                      node_size, 1);
                           }
                       });
    }

    static const char* name() {return "single";}
};

struct basic_bulk
{
    using order_func = void(*)(std::vector<void*>&);

    order_func func;
    std::size_t count;

    basic_bulk(order_func f, std::size_t c)
    : func(f), count(c) {}

    template <class RawAllocator>
    std::size_t operator()(RawAllocator &alloc, std::size_t node_size)
    {
        std::vector<void*> ptrs;
        ptrs.reserve(count);

        auto alloc_t = measure([&]()
                            {
                               for (std::size_t i = 0u; i != count; ++i)
                                ptrs.push_back(alloc.allocate_node(node_size, 1));
                            });
        func(ptrs);
        auto dealloc_t = measure([&]()
                               {
                                   for (auto ptr : ptrs)
                                       alloc.deallocate_node(ptr, node_size, 1);
                               });
        return alloc_t + dealloc_t;
    }

    template<class RawAllocator>
    std::size_t operator()(RawAllocator& alloc, std::size_t array_size, std::size_t node_size)
    {
        std::vector<void*> ptrs;
        ptrs.reserve(count);

        auto alloc_t = measure([&]()
                             {
                                 for (std::size_t i = 0u; i != count; ++i)
                                    ptrs.push_back(
                                            alloc.allocate_array(array_size, node_size, 1));
                             });
        func(ptrs);
        auto dealloc_t = measure([&]()
                               {
                                   for (auto ptr : ptrs)
                                       alloc.deallocate_array(ptr, array_size, node_size, 1);
                               });
        return alloc_t + dealloc_t;
    }
};

struct bulk : basic_bulk
{
    bulk(std::size_t c)
    : basic_bulk([](std::vector<void*> &){}, c) {}

    static const char* name() {return "bulk";}
};

struct bulk_reversed : basic_bulk
{
    bulk_reversed(std::size_t c)
    : basic_bulk([](std::vector<void*>& ptrs)
                {
                    std::reverse(ptrs.begin(), ptrs.end());
                }, c) {}

    static const char* name() {return "bulk_reversed";}
};

struct butterfly : basic_bulk
{
    butterfly(std::size_t c)
    : basic_bulk([](std::vector<void*> &ptrs)
                 {
                     std::shuffle(ptrs.begin(), ptrs.end(), std::mt19937{});
                 }, c)
    {}

    static const char* name() {return "butterfly\n";}
};

const std::size_t sample_size = 1024u;

template<typename F, typename ... Args>
std::size_t benchmark(F measure_func, Args&& ... args)
{
    auto min_time = std::size_t(-1);
    for (std::size_t i = 0u; i != sample_size; ++i)
    {
        auto time = measure_func(std::forward<Args>(args)...);
        if (time < min_time)
            min_time = time;
    }
    return min_time;
}

template <class Func, class ... Allocators>
void benchmark_node(std::size_t count, std::size_t size, Allocators&... allocators)
{
    int dummy[] = {(std::cout << benchmark(Func{count}, allocators, size) << '\t', 0)...};
    (void)dummy;
    std::cout << '\n';
}

template <class Func>
void benchmark_node(std::initializer_list<std::size_t> counts,
                    std::initializer_list<std::size_t> node_sizes)
{
    using namespace foonathan::memory;
    std::cout << Func::name() << "\n\t\tHeap\tNew\tSmall\tNode\tArray\tStack\n";
    for (auto count : counts)
        for (auto size : node_sizes)
        {
            heap_allocator heap_alloc;
            new_allocator new_alloc;
            auto small_alloc = make_allocator_adapter(
                    memory_pool<small_node_pool>{size, count * size * 2});
            auto node_alloc = make_allocator_adapter(
                    memory_pool<node_pool>{size, count * size * 2});
            auto array_alloc = make_allocator_adapter(
                    memory_pool<array_pool>{size, count * size * 2});
            auto stack_alloc = make_allocator_adapter(
                    memory_stack<>{count * size * 2});

            std::cout << count << '*' << std::setw(2) << size << ": \t";
            benchmark_node<Func>(count, size, heap_alloc, new_alloc,
                                 small_alloc, node_alloc, array_alloc,
                                 stack_alloc);
        }
    std::cout << '\n';
}

template <class Func, class Second, class ... Tail>
void benchmark_node(std::initializer_list<std::size_t> counts,
                    std::initializer_list<std::size_t> node_sizes)
{
    benchmark_node<Func>(counts, node_sizes);
    benchmark_node<Second, Tail...>(counts, node_sizes);
}

template<class Func, class ... Allocators>
void benchmark_array(std::size_t count, std::size_t array_size, std::size_t node_size,
                    Allocators& ... allocators)
{
    int dummy[] = {(std::cout << benchmark(Func{count}, allocators, array_size, node_size)
                              << '\t', 0)...};
    (void) dummy;
    std::cout << '\n';
}

template<class Func>
void benchmark_array(std::initializer_list<std::size_t> counts,
                     std::initializer_list<std::size_t> node_sizes,
                     std::initializer_list<std::size_t> array_sizes)
{
    using namespace foonathan::memory;
    std::cout << Func::name() << "\n\t\tHeap\tNew\tNode\tArray\tStack\n";
    for (auto count : counts)
        for (auto node_size : node_sizes)
            for (auto array_size : array_sizes)
            {
                auto mem_needed = count * node_size * array_size * 2;

                heap_allocator heap_alloc;
                new_allocator new_alloc;
                auto node_alloc = make_allocator_adapter(memory_pool<node_pool>{node_size, mem_needed});
                auto array_alloc = make_allocator_adapter(memory_pool<array_pool>{node_size, mem_needed});
                auto stack_alloc = make_allocator_adapter(memory_stack<>{mem_needed});

                std::cout << count << '*' << std::setw(3) << node_size
                          << '*' << std::setw(3) << array_size<< ": \t";
                benchmark_array<Func>(count , array_size, node_size,
                                      heap_alloc, new_alloc, node_alloc, array_alloc, stack_alloc);
            }
    std::cout << '\n';
}

template <class Func, class Second, class ... Tail>
void benchmark_array(std::initializer_list<std::size_t> counts,
                     std::initializer_list<std::size_t> node_sizes,
                     std::initializer_list<std::size_t> array_sizes)
{
    benchmark_array<Func>(counts, node_sizes, array_sizes);
    benchmark_array<Second, Tail...>(counts, node_sizes, array_sizes);
}

int main()
{
    using namespace foonathan::memory;

    std::cout << "Node\n\n";
    benchmark_node<single, bulk, bulk_reversed, butterfly>({256, 512, 1024}, {1, 4, 8, 256});

    std::cout << "Array\n\n";
    benchmark_array<single, bulk, bulk_reversed, butterfly>({256, 512}, {1, 4, 8}, {1, 4, 8});
}
