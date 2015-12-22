// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED

#include "detail/align.hpp"
#include "config.hpp"

namespace foonathan { namespace memory
{
    class memory_resource
    {
    public:
        virtual ~memory_resource() FOONATHAN_NOEXCEPT {}

        void* allocate(size_t bytes, size_t alignment = detail::max_alignment)
        {
            return do_allocate(bytes, alignment);
        }

        void deallocate(void* p, size_t bytes, size_t alignment = detail::max_alignment)
        {
            do_deallocate(p, bytes, alignment);
        }

        bool is_equal(const memory_resource& other) const FOONATHAN_NOEXCEPT
        {
            return do_is_equal(other);
        }

    protected:
        virtual void* do_allocate(size_t bytes, size_t alignment) = 0;

        virtual void do_deallocate(void* p, size_t bytes, size_t alignment) = 0;

        virtual bool do_is_equal(const memory_resource& other) const FOONATHAN_NOEXCEPT = 0;
    };

    bool operator==(const memory_resource& a, const memory_resource& b) FOONATHAN_NOEXCEPT
    {
        return &a == &b || a.is_equal(b);
    }

    bool operator!=(const memory_resource& a, const memory_resource& b) FOONATHAN_NOEXCEPT
    {
        return !(a == b);
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED
