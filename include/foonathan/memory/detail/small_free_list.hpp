// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED

#include <cstddef>

#include "../config.hpp"
#include "utility.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            struct chunk_base
            {
                chunk_base* prev = this;
                chunk_base* next = this;

                unsigned char first_free = 0; // first free node for the linked list
                unsigned char capacity   = 0; // total number of free nodes available
                unsigned char no_nodes   = 0; // total number of nodes in memory

                chunk_base() FOONATHAN_NOEXCEPT = default;

                chunk_base(unsigned char no) FOONATHAN_NOEXCEPT : capacity(no), no_nodes(no)
                {
                }
            };

            struct chunk;

            // the same as free_memory_list but optimized for small node sizes
            // it is slower and does not support arrays
            // but has very small overhead
            // debug: allocate() and deallocate() mark memory as new and freed, respectively
            // node_size is increased via two times fence size and fence is put in front and after
            class small_free_memory_list
            {
            public:
                // minimum element size
                static FOONATHAN_CONSTEXPR std::size_t min_element_size = 1;
                // alignment
                static FOONATHAN_CONSTEXPR std::size_t min_element_alignment = 1;

                //=== constructor ===//
                small_free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT;

                // does not own memory!
                small_free_memory_list(std::size_t node_size, void* mem,
                                       std::size_t size) FOONATHAN_NOEXCEPT;

                small_free_memory_list(small_free_memory_list&& other) FOONATHAN_NOEXCEPT;

                ~small_free_memory_list() FOONATHAN_NOEXCEPT = default;

                small_free_memory_list& operator=(small_free_memory_list&& other) FOONATHAN_NOEXCEPT
                {
                    small_free_memory_list tmp(detail::move(other));
                    swap(*this, tmp);
                    return *this;
                }

                friend void swap(small_free_memory_list& a,
                                 small_free_memory_list& b) FOONATHAN_NOEXCEPT;

                //=== insert/alloc/dealloc ===//
                // inserts new memory of given size into the free list
                // mem must be aligned for maximum alignment
                void insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT;

                // returns the usable size
                // i.e. how many memory will be actually inserted and usable on a call to insert()
                std::size_t usable_size(std::size_t size) const FOONATHAN_NOEXCEPT;

                // allocates a node big enough for the node size
                // pre: !empty()
                void* allocate() FOONATHAN_NOEXCEPT;

                // always returns nullptr, because array allocations are not supported
                void* allocate(std::size_t) FOONATHAN_NOEXCEPT
                {
                    return nullptr;
                }

                // deallocates the node previously allocated via allocate()
                void deallocate(void* node) FOONATHAN_NOEXCEPT;

                // forwards to insert()
                void deallocate(void* mem, std::size_t size) FOONATHAN_NOEXCEPT
                {
                    insert(mem, size);
                }

                // hint for allocate() to be prepared to allocate n nodes
                // it searches for a chunk that has n nodes free
                // returns false, if there is none like that
                // never fails for n == 1 if not empty()
                // pre: capacity() >= n * node_size()
                bool find_chunk(std::size_t n) FOONATHAN_NOEXCEPT
                {
                    return find_chunk_impl(n) != nullptr;
                }

                //=== getter ===//
                std::size_t node_size() const FOONATHAN_NOEXCEPT
                {
                    return node_size_;
                }

                // the alignment of all nodes
                std::size_t alignment() const FOONATHAN_NOEXCEPT;

                // number of nodes remaining
                std::size_t capacity() const FOONATHAN_NOEXCEPT
                {
                    return capacity_;
                }

                bool empty() const FOONATHAN_NOEXCEPT
                {
                    return capacity_ == 0u;
                }

            private:
                std::size_t fence_size() const FOONATHAN_NOEXCEPT;

                chunk* find_chunk_impl(std::size_t n = 1) FOONATHAN_NOEXCEPT;
                chunk* find_chunk_impl(unsigned char* node, chunk_base* first,
                                       chunk_base* last) FOONATHAN_NOEXCEPT;
                chunk* find_chunk_impl(unsigned char* node) FOONATHAN_NOEXCEPT;

                chunk_base  base_;
                std::size_t node_size_, capacity_;
                chunk_base *alloc_chunk_, *dealloc_chunk_;
            };

            // for some reason, this is required in order to define it
            void swap(small_free_memory_list& a, small_free_memory_list& b) FOONATHAN_NOEXCEPT;
        } // namespace detail
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED
