// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "static_allocator.hpp"

#include "debugging.hpp"
#include "memory_arena.hpp"

using namespace foonathan::memory;

void* static_allocator::allocate_node(std::size_t size, std::size_t alignment)
{
    auto mem = stack_.allocate(end_, size, alignment);
    if (!mem)
        FOONATHAN_THROW(out_of_memory(info(), size));
    return mem;
}

allocator_info static_allocator::info() const FOONATHAN_NOEXCEPT
{
    return {FOONATHAN_MEMORY_LOG_PREFIX "::static_allocator", this};
}

memory_block static_block_allocator::allocate_block()
{
    if (cur_ + block_size_ > end_)
        FOONATHAN_THROW(out_of_memory(info(), block_size_));
    auto mem = cur_;
    cur_ += block_size_;
    return {mem, block_size_};
}

void static_block_allocator::deallocate_block(memory_block block) FOONATHAN_NOEXCEPT
{
    detail::check_pointer(static_cast<char*>(block.memory) + block.size == cur_,
                          info(), block.memory);
    cur_ -= block_size_;
}

allocator_info static_block_allocator::info() const FOONATHAN_NOEXCEPT
{
    return {FOONATHAN_MEMORY_LOG_PREFIX "::static_block_allocator", this};
}
