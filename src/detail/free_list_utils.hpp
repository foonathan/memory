// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_SRC_DETAIL_FREE_LIST_UTILS_HPP_INCLUDED
#define FOONATHAN_MEMORY_SRC_DETAIL_FREE_LIST_UTILS_HPP_INCLUDED

#include <cstdint>

#include "config.hpp"
#include "detail/align.hpp"
#include "detail/assert.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        //=== storage ===///
        // reads stored integer value
        inline std::uintptr_t get_int(void *address) FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(address);
            FOONATHAN_MEMORY_ASSERT(is_aligned(address, FOONATHAN_ALIGNOF(std::uintptr_t)));
            return *static_cast<std::uintptr_t*>(address);
        }

        // sets stored integer value
        inline void set_int(void *address, std::uintptr_t i) FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(address);
            FOONATHAN_MEMORY_ASSERT(is_aligned(address, FOONATHAN_ALIGNOF(std::uintptr_t)));
            *static_cast<std::uintptr_t*>(address) = i;
        }

        // pointer to integer
        inline std::uintptr_t to_int(char *ptr) FOONATHAN_NOEXCEPT
        {
            return reinterpret_cast<std::uintptr_t>(ptr);
        }

        // integer to pointer
        inline char* from_int(std::uintptr_t i) FOONATHAN_NOEXCEPT
        {
            return reinterpret_cast<char*>(i);
        }

        //=== intrusive linked list ===//
        // reads a stored pointer value
        inline char* list_get_next(void *address) FOONATHAN_NOEXCEPT
        {
            return from_int(get_int(address));
        }

        // stores a pointer value
        inline void list_set_next(void *address, char *ptr) FOONATHAN_NOEXCEPT
        {
            set_int(address, to_int(ptr));
        }

        //=== intrusive xor linked list ===//
        // returns the next pointer given the previous pointer
        inline char* xor_list_get_next(void *address, char *prev) FOONATHAN_NOEXCEPT
        {
            return from_int(get_int(address) ^ to_int(prev));
        }

        // returns the prev pointer given the next pointer
        inline char* xor_list_get_prev(void *address, char *next) FOONATHAN_NOEXCEPT
        {
            return from_int(get_int(address) ^ to_int(next));
        }

        // sets the next and previous pointer
        inline void xor_list_set(void *address, char *prev, char *next) FOONATHAN_NOEXCEPT
        {
            set_int(address, to_int(prev) ^ to_int(next));
        }

        // changes next pointer given the old next pointer
        inline void xor_list_change_next(void *address, char *old_next, char *new_next) FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(address);
            // use old_next to get previous pointer
            auto prev = xor_list_get_prev(address, old_next);
            // don't change previous pointer
            xor_list_set(address, prev, new_next);
        }

        // same for prev
        inline void xor_list_change_prev(void *address, char *old_prev, char *new_prev) FOONATHAN_NOEXCEPT
        {
            FOONATHAN_MEMORY_ASSERT(address);
            auto next = xor_list_get_next(address, old_prev);
            xor_list_set(address, new_prev, next);
        }

        // advances a pointer pair forward
        inline void xor_list_iter_next(char *&cur, char *&prev) FOONATHAN_NOEXCEPT
        {
            auto next = xor_list_get_next(cur, prev);
            prev = cur;
            cur = next;
        }

        // advances a pointer pair backward
        inline void xor_list_iter_prev(char *&cur, char *&next) FOONATHAN_NOEXCEPT
        {
            auto prev = xor_list_get_prev(cur, next);
            next = cur;
            cur = prev;
        }
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_SRC_DETAIL_FREE_LIST_UTILS_HPP_INCLUDED
