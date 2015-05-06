// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_THREADING_HPP_INCLUDED
#define FOONATHAN_MEMORY_THREADING_HPP_INCLUDED

/// \file
/// \brief Adapters to share allocators between threads.

#include <mutex>

#include "allocator_traits.hpp"

namespace foonathan { namespace memory
{
	/// \brief An allocator adapter that uses a mutex for synchronizing.
    /// \detail It locks the mutex for each function called.
    /// \ingroup memory
    template <class RawAllocator, class Mutex = std::mutex>
    class thread_safe_allocator : RawAllocator
    {
    public:
        using raw_allocator = RawAllocator;
        using mutex = Mutex;
        
        using is_stateful = std::true_type;
        
        thread_safe_allocator(raw_allocator &&alloc = {})
        : raw_allocator(std::move(alloc)) {}
        
        /// @{
        /// \brief (De-)Allocation functions lock the mutex, perform the call and unlocks it.
        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            std::lock_guard<mutex> lock(mutex_);
            return traits::allocate_node(get_allocator(), size, alignment);
        }
        
        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            std::lock_guard<mutex> lock(mutex_);
            return traits::allocate_array(get_allocator(), count, size, alignment);
        }
        
        void deallocate_node(void *ptr,
                              std::size_t size, std::size_t alignment) noexcept
        {
            std::lock_guard<mutex> lock(mutex_);
            traits::deallocate_node(get_allocator(), ptr, size, alignment);
        }
        
        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) noexcept
        {
            std::lock_guard<mutex> lock(mutex_);
            traits::deallocate_array(get_allocator(), ptr, count, size, alignment);
        }
        /// @}
        
        /// @{
        /// \brief Getter functions only lock the mutex if the underlying allocator contains state.
        /// \detail If there is no state, there return value is always the same,
        /// so no locking is necessary.
        std::size_t max_node_size() const
        {
            may_lock lock(mutex_);
            return traits::max_node_size(get_allocator());
        }
        
        std::size_t max_array_size() const
        {
            may_lock lock(mutex_);
            return traits::max_array_size(get_allocator());
        }
        
        std::size_t max_alignment() const
        {
            may_lock lock(mutex_);
            return traits::max_alignment(get_allocator());
        }
        /// @}
        
        /// @{
        /// \brief Returns a reference to the allocator.
        /// \detail It is not synchronized, so race conditions might occur.
        raw_allocator& get_allocator() noexcept
        {
            return *this;
        }
        
        const raw_allocator& get_allocator() const noexcept
        {
            return *this;
        }
        /// @}
        
    private:
        struct dummy_lock
        {
            dummy_lock(const mutex &) noexcept {}
        };
    
        using traits = allocator_traits<RawAllocator>;
        using may_lock = typename std::conditional<traits::is_stateful::value,
                            std::lock_guard<mutex>,  dummy_lock>::type;
        
        mutable mutex mutex_;
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_THREADING_HPP_INCLUDED
