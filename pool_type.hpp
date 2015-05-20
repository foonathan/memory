// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED

#include <type_traits>

#include "detail/free_list.hpp"
#include "detail/small_free_list.hpp"
#include "config.hpp"

namespace foonathan { namespace memory
{
    /// @{
    /// \brief Tag types defining whether or not a pool supports arrays.
    /// \details An \c array_pool supports both node and arrays.
    /// \ingroup memory
    struct node_pool : std::false_type
    {
        using type = detail::node_free_memory_list;
    };

    struct array_pool : std::true_type
    {
        using type = detail::array_free_memory_list;
    };
    /// @}

    /// \brief Tag type indicating a pool for small objects.
    /// \details A small node pool does not support arrays.
    /// \ingroup memory
    struct small_node_pool : std::false_type
    {
        using type = detail::small_free_memory_list;
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED
