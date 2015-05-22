// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_stack.hpp"

#include "align.hpp"
#include "../debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

fixed_memory_stack::fixed_memory_stack(fixed_memory_stack &&other) FOONATHAN_NOEXCEPT
: cur_(other.cur_), end_(other.end_)
{
    other.cur_ = nullptr;
    other.end_ = nullptr;
}

fixed_memory_stack& fixed_memory_stack::operator=(fixed_memory_stack &&other) FOONATHAN_NOEXCEPT
{
    cur_ = other.cur_;
    end_ = other.end_;
    other.cur_ = nullptr;
    other.end_ = nullptr;
    return *this;
}

void* fixed_memory_stack::allocate(std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
{
    std::size_t remaining = end_ - cur_;
    std::size_t offset = align_offset(cur_, alignment);
    std::size_t front_fence = offset > debug_fence_size ? 0u : debug_fence_size - offset;
    std::size_t back_fence = debug_fence_size;
    if (offset + front_fence + size + back_fence > remaining)
        return nullptr;
    debug_fill(cur_, offset, debug_magic::alignment_memory);
    cur_ += offset;
    debug_fill(cur_, front_fence, debug_magic::fence_memory);
    cur_ += front_fence;
    auto memory = cur_;
    debug_fill(cur_, size, debug_magic::new_memory);
    cur_ += size;
    debug_fill(cur_, back_fence, debug_magic::fence_memory);
    cur_ += back_fence;
    return memory;
}

void fixed_memory_stack::unwind(char *top) FOONATHAN_NOEXCEPT
{
    debug_fill(top, cur_ - top, debug_magic::freed_memory);
    cur_ = top;
}
