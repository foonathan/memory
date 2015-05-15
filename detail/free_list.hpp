// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED

#include <cstddef>

#include "../config.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // manages free memory blocks for a pool
        // they are stored in a list and returned for subsequent allocations
        // there are two versions for the functions: ordered and non ordered
        // non ordered is faster, because it does not keep the ist sorted
        // ordered allows arrays because multiple free blocks are stored after each other
        // note: type must be trivially destructible!
        // debug: allocate() and deallocate() mark memory as new and freed, respectively
        // node_size is increased via two times fence size and fence is put in front and after
        // pointer checking checks for double frees and uses the ordered functions only
        // since the list needs to be traversed anyway
        class free_memory_list
        {
        public:
            // minimum element size
            static FOONATHAN_CONSTEXPR auto min_element_size = sizeof(char*);
            // alignment
            static FOONATHAN_CONSTEXPR auto min_element_alignment = FOONATHAN_ALIGNOF(char*);

            //=== constructor ===//
            free_memory_list(std::size_t el_size) FOONATHAN_NOEXCEPT;

            // does not own memory!
            free_memory_list(std::size_t el_size,
                             void *mem, std::size_t size) FOONATHAN_NOEXCEPT;

            free_memory_list(free_memory_list &&other) FOONATHAN_NOEXCEPT;
            ~free_memory_list() FOONATHAN_NOEXCEPT = default;

            free_memory_list& operator=(free_memory_list &&other) FOONATHAN_NOEXCEPT;

            //=== insert/allocation/deallocation ===//
            // inserts a new memory block, by splitting it up and setting the links
            // does not own memory!
            // pre: size != 0
            void insert(void *mem, std::size_t size) FOONATHAN_NOEXCEPT;
            void insert_ordered(void *mem, std::size_t size) FOONATHAN_NOEXCEPT;

            // returns single block from the list
            // pre: !empty()
            void* allocate() FOONATHAN_NOEXCEPT;

            // returns the start to multiple blocks after each other
            // pre: !empty()
            // can return nullptr if no block found
            // won't necessarily work if non-ordered functions are called
            void* allocate(std::size_t n) FOONATHAN_NOEXCEPT
            {
                return allocate(n, node_size());
            }

            // allocates array for a different node size
            // pre: other_node_size <= node_size()
            void* allocate(std::size_t n, std::size_t other_node_size) FOONATHAN_NOEXCEPT;

            void deallocate(void *ptr) FOONATHAN_NOEXCEPT;
            void deallocate(void *ptr, std::size_t n) FOONATHAN_NOEXCEPT
            {
                deallocate(ptr, n, node_size());
            }
            void deallocate(void *ptr, std::size_t n, std::size_t other_node_size) FOONATHAN_NOEXCEPT;

            void deallocate_ordered(void *ptr) FOONATHAN_NOEXCEPT;
            void deallocate_ordered(void *ptr, std::size_t n) FOONATHAN_NOEXCEPT
            {
                deallocate_ordered(ptr, n, node_size());
            }
            void deallocate_ordered(void *ptr, std::size_t n,
                                    std::size_t other_node_size) FOONATHAN_NOEXCEPT;

            //=== getter ===//
            std::size_t node_size() const FOONATHAN_NOEXCEPT;

            // number of nodes remaining
            std::size_t capacity() const FOONATHAN_NOEXCEPT
            {
                return capacity_;
            }

            bool empty() const FOONATHAN_NOEXCEPT
            {
                return !first_;
            }

        private:
            void insert_between(char *pre, char *after,
                                void *mem, std::size_t no_blocks) FOONATHAN_NOEXCEPT;

            char *first_, *last_, *last_next_;
            std::size_t node_size_, capacity_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
