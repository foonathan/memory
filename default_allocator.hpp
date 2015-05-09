// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DEFAULT_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_DEFAULT_ALLOCATOR_HPP_INCLUDED

#include "config.hpp"
#include "heap_allocator.hpp"
#include "new_allocator.hpp"

namespace foonathan { namespace memory
{
	/// \brief The default allocator as implementation for the higher-level ones.
    /// \details The higher-level allocator (\ref memory_stack, \ref memory_pool) use this allocator as default.
    /// It must be one of the low-level, statelesss allocators.<br>
    /// You can change it via the CMake variable \c FOONATHAN_MEMORY_DEFAULT_ALLOCATOR,
    /// but it must be one of the following: \ref heap_allocator, \ref new_allocator.
    /// The default is \ref heap_allocator.
    /// \ingroup memory
    using default_allocator = FOONATHAN_MEMORY_IMPL_DEFAULT_ALLOCATOR;
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DEFAULT_ALLOCATOR_HPP_INCLUDED
