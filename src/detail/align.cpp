// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/align.hpp"

#include "ilog2.hpp"

using namespace foonathan::memory;
using namespace detail;

bool foonathan::memory::detail::is_aligned(void *ptr, std::size_t alignment) FOONATHAN_NOEXCEPT
{
    FOONATHAN_MEMORY_ASSERT(is_valid_alignment(alignment));
    auto address = reinterpret_cast<std::uintptr_t>(ptr);
    return address % alignment == 0u;
}

std::size_t foonathan::memory::detail::alignment_for(std::size_t size) FOONATHAN_NOEXCEPT
{
    return size >= max_alignment ? max_alignment : (std::size_t(1) << ilog2(size));
}
