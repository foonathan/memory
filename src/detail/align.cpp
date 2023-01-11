// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "detail/align.hpp"

#include "detail/ilog2.hpp"

using namespace foonathan::memory;
using namespace detail;

bool foonathan::memory::detail::is_aligned(void* ptr, std::size_t alignment) noexcept
{
    FOONATHAN_MEMORY_ASSERT(is_valid_alignment(alignment));
    auto address = reinterpret_cast<std::uintptr_t>(ptr);
    return address % alignment == 0u;
}

std::size_t foonathan::memory::detail::alignment_for(std::size_t size) noexcept
{
    return size >= max_alignment ? max_alignment : (std::size_t(1) << ilog2(size));
}
