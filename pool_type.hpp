// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED

namespace foonathan { namespace memory
{
	/// @{
    /// \brief Tag types defining whether or not a pool supports arrays.
    /// \detail An \c array_pool supports both node and arrays.
    /// \ingroup memory
    struct node_pool : std::false_type {};
    struct array_pool : std::true_type {};    
    /// @}
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED
