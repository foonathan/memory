// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED

#include <cstddef>
#include <cstdint>

#include "align.hpp"
#include "utility.hpp"
#include "../config.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            // stores free blocks for a memory pool
            // memory blocks are fragmented and stored in a list
            // debug: fills memory and uses a bigger node_size for fence memory
            class free_memory_list
            {
            public:
                // minimum element size
                static FOONATHAN_CONSTEXPR auto min_element_size = sizeof(char*);
                // alignment
                static FOONATHAN_CONSTEXPR auto min_element_alignment = FOONATHAN_ALIGNOF(char*);

                //=== constructor ===//
                free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT;

                // calls other constructor plus insert
                free_memory_list(std::size_t node_size, void* mem,
                                 std::size_t size) FOONATHAN_NOEXCEPT;

                free_memory_list(free_memory_list&& other) FOONATHAN_NOEXCEPT;
                ~free_memory_list() FOONATHAN_NOEXCEPT = default;

                free_memory_list& operator=(free_memory_list&& other) FOONATHAN_NOEXCEPT;

                friend void swap(free_memory_list& a, free_memory_list& b) FOONATHAN_NOEXCEPT;

                //=== insert/allocation/deallocation ===//
                // inserts a new memory block, by splitting it up and setting the links
                // does not own memory!
                // mem must be aligned for alignment()
                // pre: size != 0
                void insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT;

                // returns the usable size
                // i.e. how many memory will be actually inserted and usable on a call to insert()
                std::size_t usable_size(std::size_t size) const FOONATHAN_NOEXCEPT
                {
                    return size;
                }

                // returns a single block from the list
                // pre: !empty()
                void* allocate() FOONATHAN_NOEXCEPT;

                // returns a memory block big enough for n bytes
                // might fail even if capacity is sufficient
                void* allocate(std::size_t n) FOONATHAN_NOEXCEPT;

                // deallocates a single block
                void deallocate(void* ptr) FOONATHAN_NOEXCEPT;

                // deallocates multiple blocks with n bytes total
                void deallocate(void* ptr, std::size_t n) FOONATHAN_NOEXCEPT;

                //=== getter ===//
                std::size_t node_size() const FOONATHAN_NOEXCEPT
                {
                    return node_size_;
                }

                // alignment of all nodes
                std::size_t alignment() const FOONATHAN_NOEXCEPT;

                // number of nodes remaining
                std::size_t capacity() const FOONATHAN_NOEXCEPT
                {
                    return capacity_;
                }

                bool empty() const FOONATHAN_NOEXCEPT
                {
                    return first_ == nullptr;
                }

            private:
                std::size_t fence_size() const FOONATHAN_NOEXCEPT;
                void        insert_impl(void* mem, std::size_t size) FOONATHAN_NOEXCEPT;

                char*       first_;
                std::size_t node_size_, capacity_;
            };

            void swap(free_memory_list& a, free_memory_list& b) FOONATHAN_NOEXCEPT;

            // same as above but keeps the nodes ordered
            // this allows array allocations, that is, consecutive nodes
            // debug: fills memory and uses a bigger node_size for fence memory
            class ordered_free_memory_list
            {
            public:
                // minimum element size
                static FOONATHAN_CONSTEXPR auto min_element_size = sizeof(char*);
                // alignment
                static FOONATHAN_CONSTEXPR auto min_element_alignment = FOONATHAN_ALIGNOF(char*);

                //=== constructor ===//
                ordered_free_memory_list(std::size_t node_size) FOONATHAN_NOEXCEPT;

                ordered_free_memory_list(std::size_t node_size, void* mem,
                                         std::size_t size) FOONATHAN_NOEXCEPT
                : ordered_free_memory_list(node_size)
                {
                    insert(mem, size);
                }

                ordered_free_memory_list(ordered_free_memory_list&& other) FOONATHAN_NOEXCEPT;

                ~ordered_free_memory_list() FOONATHAN_NOEXCEPT = default;

                ordered_free_memory_list& operator=(ordered_free_memory_list&& other)
                    FOONATHAN_NOEXCEPT
                {
                    ordered_free_memory_list tmp(detail::move(other));
                    swap(*this, tmp);
                    return *this;
                }

                friend void swap(ordered_free_memory_list& a,
                                 ordered_free_memory_list& b) FOONATHAN_NOEXCEPT;

                //=== insert/allocation/deallocation ===//
                // inserts a new memory block, by splitting it up and setting the links
                // does not own memory!
                // mem must be aligned for alignment()
                // pre: size != 0
                void insert(void* mem, std::size_t size) FOONATHAN_NOEXCEPT;

                // returns the usable size
                // i.e. how many memory will be actually inserted and usable on a call to insert()
                std::size_t usable_size(std::size_t size) const FOONATHAN_NOEXCEPT
                {
                    return size;
                }

                // returns a single block from the list
                // pre: !empty()
                void* allocate() FOONATHAN_NOEXCEPT;

                // returns a memory block big enough for n bytes (!, not nodes)
                // might fail even if capacity is sufficient
                void* allocate(std::size_t n) FOONATHAN_NOEXCEPT;

                // deallocates a single block
                void deallocate(void* ptr) FOONATHAN_NOEXCEPT;

                // deallocates multiple blocks with n bytes total
                void deallocate(void* ptr, std::size_t n) FOONATHAN_NOEXCEPT;

                //=== getter ===//
                std::size_t node_size() const FOONATHAN_NOEXCEPT
                {
                    return node_size_;
                }

                // alignment of all nodes
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

                // returns previous pointer
                char* insert_impl(void* mem, std::size_t size) FOONATHAN_NOEXCEPT;

                char* begin_node() FOONATHAN_NOEXCEPT;
                char* end_node() FOONATHAN_NOEXCEPT;

                std::uintptr_t begin_proxy_, end_proxy_;
                std::size_t    node_size_, capacity_;
                char *         last_dealloc_, *last_dealloc_prev_;
            };

            void swap(ordered_free_memory_list& a, ordered_free_memory_list& b) FOONATHAN_NOEXCEPT;

#if FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECk
            // use ordered version to allow pointer check
            using node_free_memory_list  = ordered_free_memory_list;
            using array_free_memory_list = ordered_free_memory_list;
#else
            using node_free_memory_list  = free_memory_list;
            using array_free_memory_list = ordered_free_memory_list;
#endif
        } // namespace detail
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
