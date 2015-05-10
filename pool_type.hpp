// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED

#include <type_traits>

#include "config.hpp"

namespace foonathan { namespace memory
{
    /// @{
    /// \brief Tag types defining whether or not a pool supports arrays.
    /// \details An \c array_pool supports both node and arrays.
    /// \ingroup memory
    struct node_pool : std::false_type {};
    struct array_pool : std::true_type {};    
    /// @}
    
    /// \brief Tag type indicating a pool for small objects.
    /// \details A small node pool does not support arrays.
    /// \ingroup memory
    struct small_node_pool : std::false_type {};
    
    namespace detail
    {
        class free_memory_list;
        class small_free_memory_list;
        
        // either calls insert or insert_ordered
        void insert(node_pool, free_memory_list &free_list,
                    void *ptr, std::size_t size) FOONATHAN_NOEXCEPT;
        void insert(array_pool, free_memory_list &free_list,
                    void *ptr, std::size_t size) FOONATHAN_NOEXCEPT;
        void insert(small_node_pool, small_free_memory_list &free_list,
                    void *ptr, std::size_t size) FOONATHAN_NOEXCEPT;
                    
        // either calls deallocate or deallocate ordered
        void deallocate(node_pool, free_memory_list &free_list, void *node) FOONATHAN_NOEXCEPT;
        void deallocate(array_pool, free_memory_list &free_list, void *node) FOONATHAN_NOEXCEPT;
        void deallocate(array_pool, free_memory_list &free_list, void *node, std::size_t n) FOONATHAN_NOEXCEPT;
        void deallocate(small_node_pool, small_free_memory_list &free_list, void *node) FOONATHAN_NOEXCEPT;
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_POOL_TYPE_HPP_INCLUDED
