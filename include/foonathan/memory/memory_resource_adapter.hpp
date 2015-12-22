// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED

#include "detail/align.hpp"
#include "detail/utility.hpp"
#include "config.hpp"

namespace foonathan { namespace memory
{
    class memory_resource
    {
    public:
        virtual ~memory_resource() FOONATHAN_NOEXCEPT {}

        void* allocate(std::size_t bytes, std::size_t alignment = detail::max_alignment)
        {
            return do_allocate(bytes, alignment);
        }

        void deallocate(void* p, std::size_t bytes, std::size_t alignment = detail::max_alignment)
        {
            do_deallocate(p, bytes, alignment);
        }

        bool is_equal(const memory_resource& other) const FOONATHAN_NOEXCEPT
        {
            return do_is_equal(other);
        }

    protected:
        virtual void* do_allocate(std::size_t bytes, std::size_t alignment) = 0;

        virtual void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) = 0;

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

    template <class RawAllocator>
    class memory_resource_adapter
    : public memory_resource, FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
    {
    public:
        using allocator_type = allocator_traits<RawAllocator>::allocator_type;

        memory_resource_adapter(allocator_type &&other) FOONATHAN_NOEXCEPT
        : allocator_type(detail::move(other)) {}

        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const allocator_type& get_allocator() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }

    protected:
        using traits_type = allocator_traits<RawAllocator>;

        void* do_allocate(std::size_t bytes, std::size_t alignment) override
        {
            auto max = traits_type::max_node_size();
            if (bytes <= max)
                return traits_type::allocate_node(*this, bytes, alignment);
            auto div = bytes / max;
            auto mod = bytes % max;
            auto n = div + bool(mod);
            return traits_type::allocate_array(*this, n, max, alignment);
        }

        void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override
        {
            auto max = traits_type::max_node_size();
            if (bytes <= max)
                traits_type::deallocate_node(*this, p, bytes, alignment);
            else
            {
                auto div = bytes / max;
                auto mod = bytes % max;
                auto n = div + bool(mod);
                traits_type::deallocate_array(*this, p, n, max, alignment);
            }
        }

        bool do_is_equal(const memory_resource& other) const FOONATHAN_NOEXCEPT override
        {
            return !traits_type::is_stateful::value || this == &other;
        }
    };

    template <>
    class allocator_traits<memory_resource*>
    {
    public:
        using allocator_type = memory_resource*;
        using is_stateful = std::true_type;

        static void* allocate_node(memory_resource *res,
                                   std::size_t size, std::size_t alignment)
        {
            return res->allocate(size, alignment);
        }

        static void* allocate_array(memory_resource *res, std::size_t count,
                                   std::size_t size, std::size_t alignment)
        {
            return res->allocate(count * size, alignment);
        }

        static void deallocate_node(memory_resource *res,
                                   void *p, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            res->deallocate(p, size, alignment);
        }

        static void deallocate_array(memory_resource *res, void *p, std::size_t count,
                                    std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            res->deallocate(p, count * size, alignment);
        }

        static std::size_t max_node_size(const memory_resource *) FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }

        static std::size_t max_array_size(const memory_resource *) FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }

        static std::size_t max_alignment(const memory_resource *) FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED
