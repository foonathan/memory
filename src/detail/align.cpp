// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/align.hpp"

#include "detail/error_helpers.hpp"
#include "ilog2.hpp"

using namespace foonathan::memory;
using namespace detail;

std::size_t foonathan::memory::detail::align_offset(void *ptr, std::size_t alignment) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(is_valid_alignment(alignment));
    auto address = reinterpret_cast<std::uintptr_t>(ptr);
    auto misaligned = address & (alignment - 1);
    return misaligned != 0 ? (alignment - misaligned) : 0;
}

bool foonathan::memory::detail::is_aligned(void *ptr, std::size_t alignment) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(is_valid_alignment(alignment));
    auto address = reinterpret_cast<std::uintptr_t>(ptr);
    return address % alignment == 0u;
}

std::size_t foonathan::memory::detail::alignment_for(std::size_t size) FOONATHAN_NOEXCEPT
{
    return size >= max_alignment ? max_alignment : (1 << ilog2(size));
}
