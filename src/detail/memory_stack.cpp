// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/memory_stack.hpp"

#include "detail/align.hpp"
#include "debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

fixed_memory_stack::fixed_memory_stack(fixed_memory_stack &&other) FOONATHAN_NOEXCEPT
: cur_(other.cur_)
{
    other.cur_ = nullptr;
}

fixed_memory_stack& fixed_memory_stack::operator=(fixed_memory_stack &&other) FOONATHAN_NOEXCEPT
{
    cur_ = other.cur_;
    other.cur_ = nullptr;
    return *this;
}

void* fixed_memory_stack::allocate(const char *end, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
{
    if (cur_ == nullptr)
        return nullptr;

    auto remaining = std::size_t(end - cur_);
    auto offset = align_offset(cur_ + debug_fence_size, alignment);

    if (debug_fence_size + offset + size + debug_fence_size > remaining)
        return nullptr;
    debug_fill(cur_, offset, debug_magic::alignment_memory);
    cur_ += offset;

    debug_fill(cur_, debug_fence_size, debug_magic::fence_memory);
    cur_ += debug_fence_size;

    auto memory = cur_;
    debug_fill(cur_, size, debug_magic::new_memory);
    cur_ += size;

    debug_fill(cur_, debug_fence_size, debug_magic::fence_memory);
    cur_ += debug_fence_size;

    return memory;
}

void fixed_memory_stack::unwind(char *top) FOONATHAN_NOEXCEPT
{
    debug_fill(top, std::size_t(cur_ - top), debug_magic::freed_memory);
    cur_ = top;
}
