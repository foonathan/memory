// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "pool_collection.hpp"

#include <cmath>
#include <climits>
#include <cstdint>
#include <type_traits>

using namespace foonathan::memory;
using namespace detail;

namespace
{
    template <typename Integral>
    constexpr bool is_power_of_two(Integral no) noexcept
    {
        return no && (no & (no - 1)) == 0;
    }
    
    // ilog2 is a ceiling implementation of log2 for integers
    // that is: ilog2(4) == 2, ilog2(5) == 3
#if defined __GNUC__
    // we have a builtin to count leading zeros, use it
    // subtract one if power of two, otherwise 0
    // multiple overloads to support each size of std::size_t
    constexpr std::size_t ilog2(unsigned int no) noexcept
    {
        return sizeof(no) * CHAR_BIT - __builtin_clz(no) - is_power_of_two(no);
    }
    
    constexpr std::size_t ilog2(unsigned long no) noexcept
    {
        return sizeof(no) * CHAR_BIT - __builtin_clzl(no) - is_power_of_two(no);
    }
    
    constexpr std::size_t ilog2(unsigned long long no) noexcept
    {
        return sizeof(no) * CHAR_BIT - __builtin_clzll(no) - is_power_of_two(no);
    }
#elif FLT_RADIX == 2
    // floating points exponent are for base 2, use ilogb to get the exponent
    // subtract one if power of two, otherwise zero
    std::size_t ilog2(std::size_t no) noexcept
    {
        return std::ilogb(no) - is_power_of_two(no);
    }
#else
    // just ceil log2
    std::size_t ilog2(std::size_t no) noexcept
    {
        std::ceil(std::log2(no));
    }
#endif

    // ilog2 of the minimum node size
    const auto min_node_size_log = ilog2(free_memory_list::min_element_size);
}

static_assert(std::is_trivially_destructible<free_memory_list>::value,
            "free_list_array currently does not call any destructors");

free_list_array::free_list_array(fixed_memory_stack &stack,
                            std::size_t max_node_size) noexcept
{
    assert(max_node_size >= free_memory_list::min_element_size && "too small max_node_size");
    auto no_pools = ilog2(max_node_size) - min_node_size_log + 1;
    auto pools_size = no_pools * sizeof(free_memory_list);
    
    nodes_ = static_cast<free_memory_list*>(stack.allocate(pools_size,
                                                        alignof(free_memory_list)));
    arrays_ = static_cast<free_memory_list*>(stack.allocate(pools_size,
                                                        alignof(free_memory_list)));
    assert(nodes_ && arrays_ && "insufficient memory block size");
    for (std::size_t i = 0u; i != no_pools; ++i)
    {
        auto el_size = 1 << (i + min_node_size_log);
        ::new(static_cast<void*>(nodes_ + i)) free_memory_list(el_size);
        ::new(static_cast<void*>(arrays_ + i)) free_memory_list(el_size);
    }
}

free_memory_list& free_list_array::get_node(std::size_t node_size) noexcept
{
    auto i = ilog2(node_size);
    if (i < min_node_size_log)
        i = min_node_size_log;
    return nodes_[i - min_node_size_log];
}

const free_memory_list& free_list_array::get_node(std::size_t node_size) const noexcept
{
    auto i = ilog2(node_size);
    if (i < min_node_size_log)
        i = min_node_size_log;
    return nodes_[i - min_node_size_log];
}

free_memory_list& free_list_array::get_array(std::size_t node_size) noexcept
{
    auto i = ilog2(node_size);
    if (i < min_node_size_log)
        i = min_node_size_log;
    return arrays_[i - min_node_size_log];
}

const free_memory_list& free_list_array::get_array(std::size_t node_size) const noexcept
{
    auto i = ilog2(node_size);
    if (i < min_node_size_log)
        i = min_node_size_log;
    return arrays_[i];
}

std::size_t free_list_array::max_node_size() const noexcept
{
    return 1 << (size() + min_node_size_log);
}
