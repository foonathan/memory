// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED

/// \file
/// \brief Tag types defining different pool types.

#include <type_traits>

#include "detail/free_list.hpp"
#include "detail/small_free_list.hpp"
#include "config.hpp"

namespace foonathan { namespace memory
{
    /// @{
    /// \brief Tag types defining the type of a memory pool.
    /// \details There are three ypes of pools:
    /// * \ref node_pool: doesn't support array allocations that well.
    /// * \ref array_pool: does support array allocations but slower.
    /// * \ref small_node_pool: optimized for small objects, low memory overhead, but slower.
    /// Does not support array allocations.<br>
    /// \ref node_pool may not always be able to allocate arrays, even if there would be enough space.
    /// It does not keep its nodes ordered, so the user have to do that manually for better array support.
    /// If not, it may grow unnecessarily but is really fast otherwise.<br>
    /// \ref array_pool does keep its nodes ordered, but is thus slower.
    /// The overhead is really big for deallocation in random order.
    /// Array allocations themselves are pretty slow, too, for bigger arrays slower than new.
    /// Use it only if the low memory overhead is really needed.<br>
    /// \ref small_node_pool is a little bit slower than \ref node_pool.
    /// The nodes from the other pools need to be able to store a pointer, in this, they don't.
    /// \ingroup memory
    struct node_pool : std::true_type
    {
        using type = detail::node_free_memory_list;
    };

    struct array_pool : std::true_type
    {
        using type = detail::array_free_memory_list;
    };

    struct small_node_pool : std::false_type
    {
        using type = detail::small_free_memory_list;
    };
    /// @}
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED
