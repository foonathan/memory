// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED
#define FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::memory_resource_adapter and \ref foonathan::memory::memory_resource_allocator to allow usage of PMRs.

#include "detail/assert.hpp"
#include "detail/utility.hpp"
#include "config.hpp"
#include "allocator_traits.hpp"

#include FOONATHAN_MEMORY_IMPL_MEMORY_RESOURCE_HEADER

namespace foonathan
{
    namespace memory
    {
        /// The \c memory_resource abstract base class used in the implementation.
        /// Since most implementation do not currently have the class defined,
        /// the exact type can be customized via the CMake options \c FOONATHAN_MEMORY_MEMORY_RESOURCE and \c FOONATHAN_MEMORY_MEMORY_RESOURCE_HEADER.
        /// By default, it uses the version of foonathan/compatibility.<br>
        /// See the polymorphic memory resource proposal for the member documentation.
        /// \ingroup memory adapter
        FOONATHAN_ALIAS_TEMPLATE(memory_resource, FOONATHAN_MEMORY_IMPL_MEMORY_RESOURCE);

        /// Wraps a \concept{concept_rawallocator,RawAllocator} and makes it a \ref memory_resource.
        /// \ingroup memory adapter
        template <class RawAllocator>
        class memory_resource_adapter
            : public memory_resource,
              FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
        {
        public:
            using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;

            /// \effects Creates the resource by moving in the allocator.
            memory_resource_adapter(allocator_type&& other) FOONATHAN_NOEXCEPT
                : allocator_type(detail::move(other))
            {
            }

            /// @{
            /// \returns A reference to the wrapped allocator.
            allocator_type& get_allocator() FOONATHAN_NOEXCEPT
            {
                return *this;
            }

            const allocator_type& get_allocator() const FOONATHAN_NOEXCEPT
            {
                return *this;
            }
            /// @}

        protected:
            using traits_type = allocator_traits<RawAllocator>;

            /// \effects Allocates raw memory with given size and alignment.
            /// It forwards to \c allocate_node() or \c allocate_array() depending on the size.
            /// \returns The new memory as returned by the \concept{concept_rawallocator,RawAllocator}.
            /// \throws Anything thrown by the allocation function.
            void* do_allocate(std::size_t bytes, std::size_t alignment) override
            {
                auto max = traits_type::max_node_size(*this);
                if (bytes <= max)
                    return traits_type::allocate_node(*this, bytes, alignment);
                auto div = bytes / max;
                auto mod = bytes % max;
                auto n   = div + (mod != 0);
                return traits_type::allocate_array(*this, n, max, alignment);
            }

            /// \effects Deallocates memory previously allocated by \ref do_allocate.
            /// It forwards to \c deallocate_node() or \c deallocate_array() depending on the size.
            /// \throws Nothing.
            void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override
            {
                auto max = traits_type::max_node_size(*this);
                if (bytes <= max)
                    traits_type::deallocate_node(*this, p, bytes, alignment);
                else
                {
                    auto div = bytes / max;
                    auto mod = bytes % max;
                    auto n   = div + (mod != 0);
                    traits_type::deallocate_array(*this, p, n, max, alignment);
                }
            }

            /// \returns Whether or not \c *this is equal to \c other
            /// by comparing the addresses.
            bool do_is_equal(const memory_resource& other) const FOONATHAN_NOEXCEPT override
            {
                return this == &other;
            }
        };

        /// Wraps a \ref memory_resource and makes it a \concept{concept_rawallocator,RawAllocator}.
        /// \ingroup memory adapter
        class memory_resource_allocator
        {
        public:
            /// \effects Creates it by giving it a pointer to the \ref memory_resource.
            /// \requires \c ptr must not be \c nullptr.
            memory_resource_allocator(memory_resource* ptr) FOONATHAN_NOEXCEPT : ptr_(ptr)
            {
                FOONATHAN_MEMORY_ASSERT(ptr);
            }

            /// \effects Allocates a node by forwarding to the \c allocate() function.
            /// \returns The node as returned by the \ref memory_resource.
            /// \throws Anything thrown by the \c allocate() function.
            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                return ptr_->allocate(size, alignment);
            }

            /// \effects Deallocates a node by forwarding to the \c deallocate() function.
            void deallocate_node(void* ptr, std::size_t size,
                                 std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                ptr_->deallocate(ptr, size, alignment);
            }

            /// \returns The maximum alignment which is the maximum value of type \c std::size_t.
            std::size_t max_alignment() const FOONATHAN_NOEXCEPT
            {
                return std::size_t(-1);
            }

            /// \returns A pointer to the used \ref memory_resource, this is never \c nullptr.
            memory_resource* resource() const FOONATHAN_NOEXCEPT
            {
                return ptr_;
            }

        private:
            memory_resource* ptr_;
        };

        /// @{
        /// \returns Whether `lhs` and `rhs` share the same resource.
        /// \relates memory_resource_allocator
        inline bool operator==(const memory_resource_allocator& lhs,
                               const memory_resource_allocator& rhs) FOONATHAN_NOEXCEPT
        {
            return lhs.resource() == rhs.resource();
        }

        inline bool operator!=(const memory_resource_allocator& lhs,
                               const memory_resource_allocator& rhs) FOONATHAN_NOEXCEPT
        {
            return !(lhs == rhs);
        }
/// @}

#if !defined(DOXYGEN)
        template <class RawAllocator>
        struct is_shared_allocator;
#endif

        /// Specialization of \ref is_shared_allocator to mark \ref memory_resource_allocator as shared.
        /// This allows using it as \ref allocator_reference directly.
        /// \ingroup memory adapter
        template <>
        struct is_shared_allocator<memory_resource_allocator> : std::true_type
        {
        };
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_MEMORY_RESOURCE_ADAPTER_HPP_INCLUDED
