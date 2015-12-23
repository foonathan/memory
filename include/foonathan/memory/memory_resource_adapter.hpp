// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED

#include "detail/align.hpp"
#include "detail/utility.hpp"
#include "config.hpp"
#include "allocator_traits.hpp"

#define COMP_IN_PARENT_HEADER
#include "comp/pmr.hpp"
#undef COMP_IN_PARENT_HEADER

namespace foonathan { namespace memory
{
    using memory_resource = foonathan_memory_comp::memory_resource;

    template <class RawAllocator>
    class memory_resource_adapter
    : public memory_resource, FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
    {
    public:
        using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;

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
            auto max = traits_type::max_node_size(*this);
            if (bytes <= max)
                return traits_type::allocate_node(*this, bytes, alignment);
            auto div = bytes / max;
            auto mod = bytes % max;
            auto n = div + bool(mod);
            return traits_type::allocate_array(*this, n, max, alignment);
        }

        void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override
        {
            auto max = traits_type::max_node_size(*this);
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

    class memory_resource_allocator
    {
    public:
        memory_resource_allocator(memory_resource *ptr) FOONATHAN_NOEXCEPT
        : ptr_(ptr) {}

        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            return ptr_->allocate(size, alignment);
        }

        void deallocate_node(void *ptr, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            ptr_->deallocate(ptr, size, alignment);
        }

        std::size_t max_alignment() const FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }

        memory_resource* resource() const FOONATHAN_NOEXCEPT
        {
            return ptr_;
        }

    private:
        memory_resource *ptr_;
    };

    template <class RawAllocator>
    struct is_shared_allocator;

    template <>
    struct is_shared_allocator<memory_resource_allocator> : std::true_type {};
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED
