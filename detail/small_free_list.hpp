// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED

#include <cstddef>

#include "../config.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // a chunk in the free list
        struct chunk
        {
            chunk *next = this, *prev = this;
            unsigned char first_node = 0u, capacity = 0u, no_nodes = 0u;
        };
        
        // the same as free_memory_list but optimized for small node sizes
        // it is slower and does not support arrays
        // but has very small overhead
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
            small_free_memory_list(std::size_t node_size,
                             void *mem, std::size_t size) FOONATHAN_NOEXCEPT;
            
            //=== insert/alloc/dealloc ===//
            // inserts new memory of given size into the free list
            void insert(void *mem, std::size_t size) FOONATHAN_NOEXCEPT;
            
            // allocates a node big enough for the node size
            // pre: !empty()
            void* allocate() FOONATHAN_NOEXCEPT;
            
            // deallocates the node previously allocated via allocate()
            void deallocate(void *node) FOONATHAN_NOEXCEPT;
            
            // hint for allocate() to be prepared to allocate n nodes
            // it searches for a chunk that has n nodes free
            // returns false, if there is none like that
            // never fails for n == 1 if not empty()
            // pre: capacity() >= n * node_size()
            bool find_chunk(std::size_t n) FOONATHAN_NOEXCEPT;
            
            //=== getter ===//
            std::size_t node_size() const FOONATHAN_NOEXCEPT
            {
                return node_size_;
            }
            
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
            // dummy_chunk_ is head/tail for used chunk list
            chunk dummy_chunk_;
            // alloc_chunk_ points to the chunk used for allocation
            // dealloc_chunk_ points to the chunk last used for deallocation
            // unused_chunk_ points to the head of a seperate list consisting of unused chunks
            chunk *alloc_chunk_, *dealloc_chunk_, *unused_chunk_;
            std::size_t node_size_, capacity_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED
