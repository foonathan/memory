// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED

#include <cstddef>
#include <utility>

#include "../config.hpp"

namespace foonathan { namespace memory
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
            free_memory_list(std::size_t node_size,
                             void *mem, std::size_t size) FOONATHAN_NOEXCEPT;

            free_memory_list(free_memory_list &&other) FOONATHAN_NOEXCEPT;
            ~free_memory_list() FOONATHAN_NOEXCEPT = default;

            free_memory_list& operator=(free_memory_list &&other) FOONATHAN_NOEXCEPT;

            friend void swap(free_memory_list &a, free_memory_list &b) FOONATHAN_NOEXCEPT;

            //=== insert/allocation/deallocation ===//
            // inserts a new memory block, by splitting it up and setting the links
            // does not own memory!
            // mem must be aligned for alignment()
            // pre: size != 0
            void insert(void *mem, std::size_t size) FOONATHAN_NOEXCEPT;

            // returns a single block from the list
            // pre: !empty()
            void* allocate() FOONATHAN_NOEXCEPT;

            // returns a memory block big enough for n bytes (!, not nodes)
            // might fail even if capacity is sufficient
            void* allocate(std::size_t n) FOONATHAN_NOEXCEPT;

            // deallocates a single block
            void deallocate(void *ptr) FOONATHAN_NOEXCEPT;

            // deallocates multiple blocks with n bytes total
            void deallocate(void *ptr, std::size_t n) FOONATHAN_NOEXCEPT;

            //=== getter ===//
            std::size_t node_size() const FOONATHAN_NOEXCEPT;

            // number of nodes remaining
            std::size_t capacity() const FOONATHAN_NOEXCEPT
            {
                return capacity_;
            }

            bool empty() const FOONATHAN_NOEXCEPT;

            // alignment of all nodes
            std::size_t alignment() const FOONATHAN_NOEXCEPT;

        private:
            // cache for new nodes that were never used
            // it works like a stack and is continous, so supports arrays
            class cache
            {
            public:
                cache() FOONATHAN_NOEXCEPT
                : cur_(nullptr), end_(nullptr) {}

                cache(void *memory, std::size_t size) FOONATHAN_NOEXCEPT
                : cur_(static_cast<char*>(memory)), end_(cur_ + size) {}

                cache(cache &&other) FOONATHAN_NOEXCEPT
                : cur_(other.cur_), end_(other.end_)
                {
                    other.cur_ = other.end_ = nullptr;
                }

                ~cache() FOONATHAN_NOEXCEPT = default;

                cache& operator=(cache &&other) FOONATHAN_NOEXCEPT
                {
                    cur_ = other.cur_;
                    end_ = other.end_;
                    other.cur_ = other.end_ = nullptr;
                    return *this;
                }

                // allocates memory of given size and alignment
                // takes care of debug filling
                // returns nullptr if no memory available
                void* allocate(std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT;

                // tries to deallocate memory
                // only works if deallocation in reversed order
                // returns true if succesfully deallocated
                bool try_deallocate(void *ptr, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT;

                char* top() FOONATHAN_NOEXCEPT
                {
                    return cur_;
                }

                char* end() FOONATHAN_NOEXCEPT
                {
                    return end_;
                }

                // number of nodes that can be allocated from the cache
                std::size_t no_nodes(std::size_t node_size) const FOONATHAN_NOEXCEPT;

            private:
                char *cur_, *end_;
            };

            // intrusive list for unused memory nodes
            // gives only a stack like interface
            class list_impl
            {
            public:
                list_impl() FOONATHAN_NOEXCEPT
                : first_(nullptr) {}

                list_impl(list_impl &&other) FOONATHAN_NOEXCEPT
                : first_(other.first_)
                {
                    other.first_ = nullptr;
                }

                ~list_impl() FOONATHAN_NOEXCEPT = default;

                list_impl& operator=(list_impl &&other) FOONATHAN_NOEXCEPT
                {
                    first_ = other.first_;
                    other.first_ = nullptr;
                    return *this;
                }

                // inserts all memory from an intervall into the list
                // it will be inserted into the front
                std::size_t insert(char *begin, char *end, std::size_t node_size) FOONATHAN_NOEXCEPT;

                // pushes a single node into the list
                // it takes care of debug filling
                void push(void *ptr, std::size_t node_size) FOONATHAN_NOEXCEPT;

                // pops the first node from the list
                // it takes care of debug fillilng
                // returns nullptr if empty
                void* pop(std::size_t node_size) FOONATHAN_NOEXCEPT;

            private:
                char *first_;
            };

            cache cache_;
            list_impl list_;
            std::size_t node_size_, capacity_;
        };

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

            // calls other constructor plus insert
            ordered_free_memory_list(std::size_t node_size,
                             void *mem, std::size_t size) FOONATHAN_NOEXCEPT;

            ordered_free_memory_list(ordered_free_memory_list &&other) FOONATHAN_NOEXCEPT;
            ~ordered_free_memory_list() FOONATHAN_NOEXCEPT = default;

            ordered_free_memory_list& operator=(ordered_free_memory_list &&other) FOONATHAN_NOEXCEPT;

            friend void swap(ordered_free_memory_list &a, ordered_free_memory_list &b) FOONATHAN_NOEXCEPT;

            //=== insert/allocation/deallocation ===//
            // inserts a new memory block, by splitting it up and setting the links
            // does not own memory!
            // mem must be aligned for alignment()
            // pre: size != 0
            void insert(void *mem, std::size_t size) FOONATHAN_NOEXCEPT;

            // returns a single block from the list
            // pre: !empty()
            void* allocate() FOONATHAN_NOEXCEPT;

            // returns a memory block big enough for n bytes (!, not nodes)
            // might fail even if capacity is sufficient
            void* allocate(std::size_t n) FOONATHAN_NOEXCEPT;

            // deallocates a single block
            void deallocate(void *ptr) FOONATHAN_NOEXCEPT;

            // deallocates multiple blocks with n bytes total
            void deallocate(void *ptr, std::size_t n) FOONATHAN_NOEXCEPT;

            //=== getter ===//
            std::size_t node_size() const FOONATHAN_NOEXCEPT;

            // number of nodes remaining
            std::size_t capacity() const FOONATHAN_NOEXCEPT
            {
                return capacity_;
            }

            bool empty() const FOONATHAN_NOEXCEPT
            {
                return list_.empty();
            }

            // alignment of all nodes
            std::size_t alignment() const FOONATHAN_NOEXCEPT;

        private:
            // node size with fence
            std::size_t node_fence_size() const FOONATHAN_NOEXCEPT;

            // xor linked list storing the free nodes
            // keeps the list ordered to support arrays
            class list_impl
            {
            public:
                list_impl() FOONATHAN_NOEXCEPT
                : first_(nullptr), last_(nullptr),
                  insert_(nullptr), insert_prev_(nullptr) {}

                list_impl(std::size_t node_size,
                    void *memory, std::size_t no_nodes) FOONATHAN_NOEXCEPT
                : list_impl()
                {
                    insert(node_size, memory, no_nodes, false);
                }

                list_impl(list_impl &&other) FOONATHAN_NOEXCEPT
                : first_(other.first_), last_(other.last_),
                  insert_(other.insert_), insert_prev_(other.insert_prev_)
                {
                    other.first_ = other.last_ = nullptr;
                    other.insert_ = other.insert_prev_ = nullptr;
                }

                ~list_impl() FOONATHAN_NOEXCEPT = default;

                list_impl& operator=(list_impl &&other) FOONATHAN_NOEXCEPT
                {
                    list_impl tmp(std::move(other));
                    swap(*this, tmp);
                    return *this;
                }

                friend void swap(list_impl &a, list_impl &b) FOONATHAN_NOEXCEPT
                {
                    std::swap(a.first_, b.first_);
                    std::swap(a.last_, b.last_);
                    std::swap(a.insert_, b.insert_);
                    std::swap(a.insert_prev_, b.insert_prev_);
                }

                // inserts nodes into the list
                // node_size is the node_size_ member of the actual free list class
                void insert(std::size_t node_size,
                            void* memory, std::size_t no_nodes, bool new_memory) FOONATHAN_NOEXCEPT;

                // erases nodes from the list
                // node_size is the node_size_ member of the actual free list class
                void* erase(std::size_t node_size) FOONATHAN_NOEXCEPT;
                void* erase(std::size_t node_size, std::size_t bytes_needed) FOONATHAN_NOEXCEPT;

                bool empty() const FOONATHAN_NOEXCEPT;

            private:
                struct pos {char *prev, *after;};

                // finds the position to insert memory
                pos find_pos(std::size_t node_size, char* memory) const FOONATHAN_NOEXCEPT;

                char *first_, *last_;
                char *insert_, *insert_prev_; // pointer to last insert position
            } list_;

            std::size_t node_size_, capacity_;
        };

        #if FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECk
            // use ordered version to allow pointer check
            using node_free_memory_list = ordered_free_memory_list;
            using array_free_memory_list = ordered_free_memory_list;
        #else
            using node_free_memory_list = free_memory_list;
            using array_free_memory_list = ordered_free_memory_list;
        #endif
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
