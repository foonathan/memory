// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
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
                static constexpr auto min_element_size = sizeof(char*);
                // alignment
                static constexpr auto min_element_alignment = alignof(char*);

                // minimal size of the block that needs to be inserted
                static constexpr std::size_t min_block_size(std::size_t node_size,
                                                            std::size_t number_of_nodes)
                {
                    return (node_size < min_element_size ? min_element_size : node_size)
                           * number_of_nodes;
                }

                //=== constructor ===//
                free_memory_list(std::size_t node_size) noexcept;

                // calls other constructor plus insert
                free_memory_list(std::size_t node_size, void* mem, std::size_t size) noexcept;

                free_memory_list(free_memory_list&& other) noexcept;
                ~free_memory_list() noexcept = default;

                free_memory_list& operator=(free_memory_list&& other) noexcept;

                friend void swap(free_memory_list& a, free_memory_list& b) noexcept;

                //=== insert/allocation/deallocation ===//
                // inserts a new memory block, by splitting it up and setting the links
                // does not own memory!
                // mem must be aligned for alignment()
                // pre: size != 0
                void insert(void* mem, std::size_t size) noexcept;

                // returns the usable size
                // i.e. how many memory will be actually inserted and usable on a call to insert()
                std::size_t usable_size(std::size_t size) const noexcept
                {
                    // Round down to next multiple of node size.
                    return (size / node_size_) * node_size_;
                }

                // returns a single block from the list
                // pre: !empty()
                void* allocate() noexcept;

                // returns a memory block big enough for n bytes
                // might fail even if capacity is sufficient
                void* allocate(std::size_t n) noexcept;

                // deallocates a single block
                void deallocate(void* ptr) noexcept;

                // deallocates multiple blocks with n bytes total
                void deallocate(void* ptr, std::size_t n) noexcept;

                //=== getter ===//
                std::size_t node_size() const noexcept
                {
                    return node_size_;
                }

                // alignment of all nodes
                std::size_t alignment() const noexcept;

                // number of nodes remaining
                std::size_t capacity() const noexcept
                {
                    return capacity_;
                }

                bool empty() const noexcept
                {
                    return first_ == nullptr;
                }

            private:
                void insert_impl(void* mem, std::size_t size) noexcept;

                char*       first_;
                std::size_t node_size_, capacity_;
            };

            void swap(free_memory_list& a, free_memory_list& b) noexcept;

            // same as above but keeps the nodes ordered
            // this allows array allocations, that is, consecutive nodes
            // debug: fills memory and uses a bigger node_size for fence memory
            class ordered_free_memory_list
            {
            public:
                // minimum element size
                static constexpr auto min_element_size = sizeof(char*);
                // alignment
                static constexpr auto min_element_alignment = alignof(char*);

                // minimal size of the block that needs to be inserted
                static constexpr std::size_t min_block_size(std::size_t node_size,
                                                            std::size_t number_of_nodes)
                {
                    return (node_size < min_element_size ? min_element_size : node_size)
                           * number_of_nodes;
                }

                //=== constructor ===//
                ordered_free_memory_list(std::size_t node_size) noexcept;

                ordered_free_memory_list(std::size_t node_size, void* mem,
                                         std::size_t size) noexcept
                : ordered_free_memory_list(node_size)
                {
                    insert(mem, size);
                }

                ordered_free_memory_list(ordered_free_memory_list&& other) noexcept;

                ~ordered_free_memory_list() noexcept = default;

                ordered_free_memory_list& operator=(ordered_free_memory_list&& other) noexcept
                {
                    ordered_free_memory_list tmp(detail::move(other));
                    swap(*this, tmp);
                    return *this;
                }

                friend void swap(ordered_free_memory_list& a, ordered_free_memory_list& b) noexcept;

                //=== insert/allocation/deallocation ===//
                // inserts a new memory block, by splitting it up and setting the links
                // does not own memory!
                // mem must be aligned for alignment()
                // pre: size != 0
                void insert(void* mem, std::size_t size) noexcept;

                // returns the usable size
                // i.e. how many memory will be actually inserted and usable on a call to insert()
                std::size_t usable_size(std::size_t size) const noexcept
                {
                    // Round down to next multiple of node size.
                    return (size / node_size_) * node_size_;
                }

                // returns a single block from the list
                // pre: !empty()
                void* allocate() noexcept;

                // returns a memory block big enough for n bytes (!, not nodes)
                // might fail even if capacity is sufficient
                void* allocate(std::size_t n) noexcept;

                // deallocates a single block
                void deallocate(void* ptr) noexcept;

                // deallocates multiple blocks with n bytes total
                void deallocate(void* ptr, std::size_t n) noexcept;

                //=== getter ===//
                std::size_t node_size() const noexcept
                {
                    return node_size_;
                }

                // alignment of all nodes
                std::size_t alignment() const noexcept;

                // number of nodes remaining
                std::size_t capacity() const noexcept
                {
                    return capacity_;
                }

                bool empty() const noexcept
                {
                    return capacity_ == 0u;
                }

            private:
                // returns previous pointer
                char* insert_impl(void* mem, std::size_t size) noexcept;

                char* begin_node() noexcept;
                char* end_node() noexcept;

                std::uintptr_t begin_proxy_, end_proxy_;
                std::size_t    node_size_, capacity_;
                char *         last_dealloc_, *last_dealloc_prev_;
            };

            void swap(ordered_free_memory_list& a, ordered_free_memory_list& b) noexcept;

#if FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK
            // use ordered version to allow pointer check
            using node_free_memory_list  = ordered_free_memory_list;
            using array_free_memory_list = ordered_free_memory_list;
#else
            using node_free_memory_list  = free_memory_list;
            using array_free_memory_list = ordered_free_memory_list;
#endif
        } // namespace detail
    }     // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
