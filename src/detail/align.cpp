// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "detail/align.hpp"

#include "detail/ilog2.hpp"

using namespace foonathan::memory;
using namespace detail;

bool foonathan::memory::detail::is_aligned(void* ptr, std::size_t alignment) noexcept
{
    FOONATHAN_MEMORY_ASSERT(is_valid_alignment(alignment));
    auto address = reinterpret_cast<std::uintptr_t>(ptr);
    return (address & (alignment - 1)) == 0u;
}

std::size_t foonathan::memory::detail::alignment_for(std::size_t size) noexcept
{
    auto largest_possible_alignment = size & ~(size - 1);
    return largest_possible_alignment > max_alignment ? max_alignment : largest_possible_alignment;
}
