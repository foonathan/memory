// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

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
