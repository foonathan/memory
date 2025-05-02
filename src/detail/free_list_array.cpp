// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "detail/free_list_array.hpp"

#include "detail/assert.hpp"
#include "detail/ilog2.hpp"

using namespace foonathan::memory;
using namespace detail;

std::size_t log2_access_policy::index_from_size(std::size_t size) noexcept
{
    FOONATHAN_MEMORY_ASSERT_MSG(size, "size must not be zero");
    return ilog2_ceil(size);
}

std::size_t log2_access_policy::size_from_index(std::size_t index) noexcept
{
    return std::size_t(1) << index;
}
