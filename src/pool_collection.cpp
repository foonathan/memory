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
    bool is_power_of_two(Integral no) FOONATHAN_NOEXCEPT
    {
        return no && (no & (no - 1)) == 0;
    }

    // ilog2 is a ceiling implementation of log2 for integers
    // that is: ilog2(4) == 2, ilog2(5) == 3
#if defined __GNUC__
    // we have a builtin to count leading zeros, use it
    // subtract one if power of two, otherwise 0
    // multiple overloads to support each size of std::size_t
    std::size_t ilog2(unsigned int no) FOONATHAN_NOEXCEPT
    {
        return sizeof(no) * CHAR_BIT - unsigned(__builtin_clz(no)) - unsigned(is_power_of_two(no));
    }

    std::size_t ilog2(unsigned long no) FOONATHAN_NOEXCEPT
    {
        return sizeof(no) * CHAR_BIT - unsigned(__builtin_clzl(no)) - unsigned(is_power_of_two(no));
    }

    std::size_t ilog2(unsigned long long no) FOONATHAN_NOEXCEPT
    {
        return sizeof(no) * CHAR_BIT - unsigned(__builtin_clzll(no)) - unsigned(is_power_of_two(no));
    }
#elif FLT_RADIX == 2
    // floating points exponent are for base 2, use ilogb to get the exponent
    // subtract one if power of two, otherwise zero
    std::size_t ilog2(std::size_t no) FOONATHAN_NOEXCEPT
    {
        return std::ilogb(no) - unsigned(is_power_of_two(no));
    }
#else
    // just ceil log2
    std::size_t ilog2(std::size_t no) FOONATHAN_NOEXCEPT
    {
        std::ceil(std::log2(no));
    }
#endif

    // ilog2 of the minimum node size
    const auto min_node_size_log = ilog2(free_memory_list::min_element_size);
}

free_list_array::free_list_array(fixed_memory_stack &stack,
                            std::size_t max_node_size) FOONATHAN_NOEXCEPT
{
    assert(max_node_size >= free_memory_list::min_element_size && "too small max_node_size");
    no_pools_ = ilog2(max_node_size) - min_node_size_log + 1;

    nodes_ = static_cast<node_free_memory_list*>(stack.allocate(no_pools_ * sizeof(node_free_memory_list),
                                                        FOONATHAN_ALIGNOF(node_free_memory_list)));
    arrays_ = static_cast<array_free_memory_list*>(stack.allocate(no_pools_ * sizeof(array_free_memory_list),
                                                        FOONATHAN_ALIGNOF(array_free_memory_list)));
    assert(nodes_ && arrays_ && "insufficient memory block size");
    for (std::size_t i = 0u; i != no_pools_; ++i)
    {
        std::size_t node_size = 1u << (i + min_node_size_log);
        ::new(static_cast<void*>(nodes_ + i)) node_free_memory_list(node_size);
        ::new(static_cast<void*>(arrays_ + i)) array_free_memory_list(node_size);
    }
}

node_free_memory_list& free_list_array::get_node(std::size_t node_size) FOONATHAN_NOEXCEPT
{
    auto i = ilog2(node_size);
    if (i < min_node_size_log)
        i = min_node_size_log;
    return nodes_[i - min_node_size_log];
}

const node_free_memory_list& free_list_array::get_node(std::size_t node_size) const FOONATHAN_NOEXCEPT
{
    auto i = ilog2(node_size);
    if (i < min_node_size_log)
        i = min_node_size_log;
    return nodes_[i - min_node_size_log];
}

array_free_memory_list& free_list_array::get_array(std::size_t node_size) FOONATHAN_NOEXCEPT
{
    auto i = ilog2(node_size);
    if (i < min_node_size_log)
        i = min_node_size_log;
    return arrays_[i - min_node_size_log];
}

const array_free_memory_list& free_list_array::get_array(std::size_t node_size) const FOONATHAN_NOEXCEPT
{
    auto i = ilog2(node_size);
    if (i < min_node_size_log)
        i = min_node_size_log;
    return arrays_[i];
}

std::size_t free_list_array::max_node_size() const FOONATHAN_NOEXCEPT
{
    return 1 << (size() + min_node_size_log);
}
