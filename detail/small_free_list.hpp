// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <limits>

namespace foonathan { namespace memory
{
	namespace detail
    {
        // the same as free_memory_list but optimized for small node sizes
    	class small_free_memory_list
        {
        public:
            // minimum element size
            static constexpr auto min_element_size = 1;
            // alignment
            static constexpr auto min_element_alignment = 1;
            
            //=== constructor ===//
            small_free_memory_list(std::size_t node_size) noexcept;
            
            // does not own memory!
            small_free_memory_list(std::size_t node_size,
                             void *mem, std::size_t size) noexcept;
            
            //=== insert/alloc/dealloc ===//
            // inserts new memory of given size into the free list
            void insert(void *mem, std::size_t size) noexcept;
            
            // allocates a node big enough for the node size
            // pre: !empty()
            void* allocate() noexcept;
            
            // deallocates the node previously allocated via allocate()
            void deallocate(void *node) noexcept;
            
            // hint for allocate() to be prepared to allocate n nodes
            // it searches for a chunk that has n nodes free
            // returns false, if there is none like that
            // never fails for n == 1 if not empty()
            // pre: capacity() >= n * node_size()
            bool find_chunk(std::size_t n) noexcept;
            
            //=== getter ===//
            std::size_t node_size() const noexcept
            {
                return node_size_;
            }
            
            // number of nodes remaining
            std::size_t capacity() const noexcept
            {
                return capacity_;
            }
            
            bool empty() const noexcept
            {
                return dummy_chunk_.next->empty();
            }
                 
        private:
            struct chunk
            {
                using size_type = unsigned char;
                static constexpr auto max_nodes = std::numeric_limits<size_type>::max();
                
                chunk *next, *prev;
                size_type first_node, no_nodes;
                
                // creates the dummy chunk for the chunk list
                chunk() noexcept
                : next(this), prev(this), first_node(0), no_nodes(0) {}
                
                // initializes the small free list
                // assumes memory_offset + count_nodes * node_size memory beginning with this
                // the node_size must always stay the same
                // does not make it part of the chunk list
                chunk(size_type node_size, size_type count_nodes = max_nodes) noexcept
                : first_node(0), no_nodes(count_nodes)
                {
                    assert(node_size >= sizeof(size_type));
                    auto p = list_memory();
                    for (size_type i = 0; i != count_nodes; p += node_size)
                        *p = ++i;
                }
                
                size_type* list_memory() noexcept
                {
                    void* this_mem = this;
                    return static_cast<size_type*>(this_mem) + memory_offset;
                }
                
                const size_type* list_memory() const noexcept
                {
                    const void* this_mem = this;
                    return static_cast<const size_type*>(this_mem) + memory_offset;
                }
                
                // whether or not this chunk is empty
                bool empty() const noexcept
                {
                    return no_nodes == 0u;
                }
                
                // allocates memory of given node size
                void* allocate(std::size_t node_size) noexcept
                {
                    auto memory = list_memory() + first_node * node_size;
                    first_node = *memory;
                    --no_nodes;
                    return memory;
                }
                
                // whether or not a given memory block is from this chunk
                bool from_chunk(void *memory, std::size_t node_size) const noexcept
                {
                    // comparision should be allowed, since the pointer points into memory from a memory block list
                    return list_memory() <= memory && memory < list_memory() + node_size * max_nodes;  
                }
                
                // deallocates memory of given node size
                void deallocate(void *memory, std::size_t node_size) noexcept
                {
                    auto node_mem = static_cast<size_type*>(memory);
                    *node_mem = first_node;
                    first_node = (node_mem - list_memory()) / node_size;
                    ++no_nodes;
                }
            };
            static constexpr auto alignment_div = sizeof(chunk) / alignof(std::max_align_t);
            static constexpr auto alignment_mod = sizeof(chunk) % alignof(std::max_align_t);
            static constexpr auto memory_offset = alignment_mod == 0u ? sizeof(chunk)
                                                : (alignment_div + 1) * alignof(std::max_align_t);
            chunk dummy_chunk_;
            chunk *alloc_chunk_, *dealloc_chunk_;
            std::size_t node_size_, capacity_;
        };
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_SMALL_FREE_LIST_HPP_INCLUDED
