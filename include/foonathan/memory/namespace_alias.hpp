// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_NAMESPACE_ALIAS_HPP_INCLUDED
#define FOONATHAN_MEMORY_NAMESPACE_ALIAS_HPP_INCLUDED

/// \file
/// Convenient namespace alias.

/// \defgroup memory memory

/// \defgroup core Core components

/// \defgroup allocator Allocator implementations

/// \defgroup adapter Adapters and Wrappers

/// \defgroup storage Allocator storage

/// \namespace foonathan
/// Foonathan namespace.

/// \namespace foonathan::memory
/// Memory namespace.

/// \namespace foonathan::memory::literals
/// Literals namespace.

namespace foonathan
{
    namespace memory
    {
    }
} // namespace foonathan

namespace memory = foonathan::memory;

#endif // FOONATHAN_MEMORY_NAMESPACE_ALIAS_HPP_INCLUDED
