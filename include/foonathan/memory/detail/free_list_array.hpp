// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_FREE_LIST_ARRAY_HPP
#define FOONATHAN_MEMORY_DETAIL_FREE_LIST_ARRAY_HPP

#include "align.hpp"
#include "assert.hpp"
#include "memory_stack.hpp"
#include "../config.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            // an array of free_memory_list types
            // indexed via size, AccessPolicy does necessary conversions
            // requires trivial destructible FreeList type
            template <class FreeList, class AccessPolicy>
            class free_list_array
            {
                // not supported on GCC 4.7
                //static_assert(std::is_trivially_destructible<FreeList>::value,
                //            "free list must be trivially destructible");
            public:
                // creates sufficient elements to support up to given maximum node size
                // all lists are initially empty
                // actual number is calculated via policy
                // memory is taken from fixed_memory_stack, it must be sufficient
                free_list_array(fixed_memory_stack& stack, const char* end,
                                std::size_t max_node_size) FOONATHAN_NOEXCEPT
                    : no_elements_(AccessPolicy::index_from_size(max_node_size) - min_size_index
                                   + 1)
                {
                    array_ =
                        static_cast<FreeList*>(stack.allocate(end, no_elements_ * sizeof(FreeList),
                                                              FOONATHAN_ALIGNOF(FreeList)));
                    FOONATHAN_MEMORY_ASSERT(array_);
                    for (std::size_t i = 0u; i != no_elements_; ++i)
                    {
                        auto node_size = AccessPolicy::size_from_index(i + min_size_index);
                        ::new (static_cast<void*>(array_ + i)) FreeList(node_size);
                    }
                }

                // move constructor, does not actually move the elements, just the pointer
                free_list_array(free_list_array&& other) FOONATHAN_NOEXCEPT
                    : array_(other.array_),
                      no_elements_(other.no_elements_)
                {
                    other.array_       = nullptr;
                    other.no_elements_ = 0u;
                }

                // destructor, does nothing, list must be trivially destructible!
                ~free_list_array() FOONATHAN_NOEXCEPT = default;

                free_list_array& operator=(free_list_array&& other) FOONATHAN_NOEXCEPT
                {
                    array_       = other.array_;
                    no_elements_ = other.no_elements_;

                    other.array_       = nullptr;
                    other.no_elements_ = 0u;
                    return *this;
                }

                // access free list for given size
                FreeList& get(std::size_t node_size) const FOONATHAN_NOEXCEPT
                {
                    auto i = AccessPolicy::index_from_size(node_size);
                    if (i < min_size_index)
                        i = min_size_index;
                    return array_[i - min_size_index];
                }

                // number of free lists
                std::size_t size() const FOONATHAN_NOEXCEPT
                {
                    return no_elements_;
                }

                // maximum supported node size
                std::size_t max_node_size() const FOONATHAN_NOEXCEPT
                {
                    return AccessPolicy::size_from_index(no_elements_ + min_size_index - 1);
                }

            private:
                static const std::size_t min_size_index;

                FreeList*   array_;
                std::size_t no_elements_;
            };

            template <class FL, class AP>
            const std::size_t free_list_array<FL, AP>::min_size_index =
                AP::index_from_size(FL::min_element_size);

            // AccessPolicy that maps size to indices 1:1
            // creates a free list for each size!
            struct identity_access_policy
            {
                static std::size_t index_from_size(std::size_t size) FOONATHAN_NOEXCEPT
                {
                    return size;
                }

                static std::size_t size_from_index(std::size_t index) FOONATHAN_NOEXCEPT
                {
                    return index;
                }
            };

            // AccessPolicy that maps sizes to the integral log2
            // this creates more nodes and never wastes more than half the size
            struct log2_access_policy
            {
                static std::size_t index_from_size(std::size_t size) FOONATHAN_NOEXCEPT;
                static std::size_t size_from_index(std::size_t index) FOONATHAN_NOEXCEPT;
            };
        } // namespace detail
    }
} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_DETAIL_FREE_LIST_ARRAY_HPP
