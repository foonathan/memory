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
    namespace detail
    {
        // dummy lock type that does not lock anything
        struct dummy_lock
        {
            template <typename T>
            dummy_lock(const T &) noexcept {}
        };
        
        // non changeable pointer to an Allocator that keeps a lock
    	template <class Alloc, class Mutex>
        class locked_allocator
        {
        public: 
            locked_allocator(Alloc &alloc, Mutex &m) noexcept
            : lock_(m), alloc_(&alloc) {}
            
            locked_allocator(locked_allocator &&other) noexcept
            : lock_(std::move(other.lock_)), alloc_(other.alloc_) {}
           
            Alloc& operator*() const noexcept
            {
                return *alloc_;
            }
            
            Alloc* operator->() const noexcept
            {
                return alloc_;
            }
            
        private:        
            std::unique_lock<Mutex> lock_;
            Alloc *alloc_;
        };
    } // namespace detail
    
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
        
        thread_safe_allocator(thread_safe_allocator &&other)
        : raw_allocator(std::move(other)) {}
        
        ~thread_safe_allocator() noexcept = default;
        
        thread_safe_allocator& operator=(thread_safe_allocator &&other)
        {
            raw_allocator::operator=(std::move(other));
            return *this;
        }
        
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
        
        /// @{
        /// \brief Returns a pointer to the allocator while keeping it locked.
        /// \detail It returns a proxy object that holds the lock.
        /// It has overloaded operator* and -> to give access to the allocator
        /// but it can't be reassigned to a different allocator object.
        detail::locked_allocator<raw_allocator, mutex> lock() noexcept
        {
            return {*this, mutex_};
        }
        
        detail::locked_allocator<const raw_allocator, mutex> lock() const noexcept
        {
            return {*this, mutex_};
        }
        /// @}
        
    private:    
        using traits = allocator_traits<RawAllocator>;
        using may_lock = typename std::conditional<traits::is_stateful::value,
                            std::lock_guard<mutex>, detail::dummy_lock>::type;
        
        mutable mutex mutex_;
    };
    
    /// @{
    /// \brief Creates a \ref thread_safe_allocator.
    /// \relates thread_safe_allocator
    template <class RawAllocator>
    auto make_thread_safe_allocator(RawAllocator &&allocator)
    -> thread_safe_allocator<typename std::decay<RawAllocator>::type>
    {
        return std::forward<RawAllocator>(allocator);
    }
    
    template <class Mutex, class RawAllocator>
    auto make_thread_safe_allocator(RawAllocator &&allocator)
    -> thread_safe_allocator<typename std::decay<RawAllocator>::type, Mutex>
    {
        return std::forward<RawAllocator>(allocator);
    }
    /// @}
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_THREADING_HPP_INCLUDED
