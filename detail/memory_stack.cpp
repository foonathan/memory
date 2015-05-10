// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_stack.hpp"

#include "align.hpp"
#include "../debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

void* fixed_memory_stack::allocate(std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
{
    std::size_t remaining = end_ - cur_;
    auto offset = align_offset(cur_, alignment);
    if (offset + size > remaining)
        return nullptr;
    detail::debug_fill(cur_, offset, debug_magic::alignment_buffer);
    cur_ += offset;
    auto memory = cur_;
    cur_ += size;
    detail::debug_fill(memory, size, debug_magic::new_memory);
    return memory;
}
