// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DEFAULT_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_DEFAULT_ALLOCATOR_HPP_INCLUDED

/// \file
/// The typedef \ref foonathan::memory::default_allocator.

#include "config.hpp"
#include "heap_allocator.hpp"
#include "new_allocator.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include "malloc_allocator.hpp"
#endif

namespace foonathan { namespace memory
{
    /// The default \concept{concept_rawallocator,RawAllocator} that will be used as implementation allocator in memory arenas.
    /// Arena allocators like \ref memory_stack or \ref memory_pool allocate memory by subdividing a huge block.
    /// They get an implementation allocator that will be used for their internal allocation,
    /// this type is the default value.
    /// \requiredbe Its type can be changed via the CMake option \c FOONATHAN_MEMORY_DEFAULT_ALLCOATOR,
    /// but it must be one of the following: \ref heap_allocator, \ref new_allocator, \ref malloc_allocator.
    /// \defaultbe The default is \ref heap_allocator.
    /// \ingroup memory
    using default_allocator = FOONATHAN_IMPL_DEFINED(FOONATHAN_MEMORY_IMPL_DEFAULT_ALLOCATOR);
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DEFAULT_ALLOCATOR_HPP_INCLUDED
