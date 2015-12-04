// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED

#include "config.hpp"

namespace foonathan { namespace memory
{
    struct memory_block
    {
        void *memory;
        std::size_t size;

        memory_block() FOONATHAN_NOEXCEPT
        : memory_block(nullptr, size) {}

        memory_block(void *mem, std::size_t size) FOONATHAN_NOEXCEPT
        : memory(mem), size(size) {}

        memory_block(void *begin, void *end) FOONATHAN_NOEXCEPT
        : memory_block(begin, static_cast<char*>(end) - static_cast<char*>(begin)) {}
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_ARENA_HPP_INCLUDED
