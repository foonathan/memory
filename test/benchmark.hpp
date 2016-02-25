// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_TEST_BENCHMARK_HPP_INCLUDED
#define FOONATHAN_MEMORY_TEST_BENCHMARK_HPP_INCLUDED

// Benchmarking functions and allocator scenarios

#include <algorithm>
#include <chrono>
#include <random>
#include <vector>

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

#endif // FOONATHAN_MEMORY_TEST_BENCHMARK_HPP_INCLUDED
