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
        // dummy mutex that does not lock anything
        struct dummy_mutex
        {
            void lock() noexcept {}
            void unlock() noexcept {}
        };
        
        // selects a mutex for an Allocator
        template <class RawAllocator, class Mutex>
        using mutex_for = typename std::conditional<allocator_traits<RawAllocator>::is_stateful::value,
                                                    Mutex, dummy_mutex>::type;
        
        // storage for mutexes to use EBO
        template <class Mutex>
        class mutex_storage
        {
        public:
            void lock() const
            {
                mutex_.lock();
            }
            
            void unlock() const noexcept
            {
                mutex_.unlock();
            }
            
        protected:
            mutex_storage() noexcept = default;
            ~mutex_storage() noexcept = default;
        private:
            mutable Mutex mutex_;
        };
        
        template <>
        class mutex_storage<dummy_mutex>
        {
        public:
            void lock() const noexcept {}
            void unlock() const noexcept {}
        protected:
            mutex_storage() noexcept = default;
            ~mutex_storage() noexcept = default;
        };
        
        // non changeable pointer to an Allocator that keeps a lock
        // I don't think EBO is necessary here...
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
    /// It will not look anything if the allocator is stateless.
    /// \ingroup memory
    template <class RawAllocator, class Mutex = std::mutex>
    class thread_safe_allocator : RawAllocator,
                                  detail::mutex_storage<detail::mutex_for<RawAllocator, Mutex>>
    {
        using traits = allocator_traits<RawAllocator>;
        using actual_mutex = detail::mutex_storage<detail::mutex_for<RawAllocator, Mutex>>;
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
        
        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            std::lock_guard<actual_mutex> lock(*this);
            return traits::allocate_node(get_allocator(), size, alignment);
        }
        
        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            std::lock_guard<actual_mutex> lock(*this);
            return traits::allocate_array(get_allocator(), count, size, alignment);
        }
        
        void deallocate_node(void *ptr,
                              std::size_t size, std::size_t alignment) noexcept
        {
            std::lock_guard<actual_mutex> lock(*this);
            traits::deallocate_node(get_allocator(), ptr, size, alignment);
        }
        
        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) noexcept
        {
            std::lock_guard<actual_mutex> lock(*this);
            traits::deallocate_array(get_allocator(), ptr, count, size, alignment);
        }

        std::size_t max_node_size() const
        {
            std::lock_guard<actual_mutex> lock(*this);
            return traits::max_node_size(get_allocator());
        }
        
        std::size_t max_array_size() const
        {
            std::lock_guard<actual_mutex> lock(*this);
            return traits::max_array_size(get_allocator());
        }
        
        std::size_t max_alignment() const
        {
            std::lock_guard<actual_mutex> lock(*this);
            return traits::max_alignment(get_allocator());
        }
        
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
        detail::locked_allocator<raw_allocator, actual_mutex> lock() noexcept
        {
            return {*this, *this};
        }
        
        detail::locked_allocator<const raw_allocator, actual_mutex> lock() const noexcept
        {
            return {*this, *this};
        }
        /// @}
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
