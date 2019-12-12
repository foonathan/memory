// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_THREADING_HPP_INCLUDED
#define FOONATHAN_MEMORY_THREADING_HPP_INCLUDED

/// \file
/// The mutex types.

#include <type_traits>

#include "allocator_traits.hpp"
#include "config.hpp"

#include <foonathan/mutex.hpp>

#if FOONATHAN_HOSTED_IMPLEMENTATION
#include <mutex>
#endif

namespace foonathan
{
    namespace memory
    {
        /// A dummy \c Mutex class that does not lock anything.
        /// It is a valid \c Mutex and can be used to disable locking anywhere a \c Mutex is requested.
        /// \ingroup memory core
        struct no_mutex
        {
            void lock() FOONATHAN_NOEXCEPT {}

            bool try_lock() FOONATHAN_NOEXCEPT
            {
                return true;
            }

            void unlock() FOONATHAN_NOEXCEPT {}
        };

        /// Specifies whether or not a \concept{concept_rawallocator,RawAllocator} is thread safe as-is.
        /// This allows to use \ref no_mutex as an optimization.
        /// Note that stateless allocators are implictly thread-safe.
        /// Specialize it only for your own stateful allocators.
        /// \ingroup memory core
        template <class RawAllocator>
        struct is_thread_safe_allocator
        : std::integral_constant<bool, !allocator_traits<RawAllocator>::is_stateful::value>
        {
        };

        namespace detail
        {
            // selects a mutex for an Allocator
            // stateless allocators don't need locking
            template <class RawAllocator, class Mutex>
            using mutex_for =
                typename std::conditional<is_thread_safe_allocator<RawAllocator>::value, no_mutex,
                                          Mutex>::type;

            // storage for mutexes to use EBO
            // it provides const lock/unlock function, inherit from it
            template <class Mutex>
            class mutex_storage
            {
            public:
                mutex_storage() FOONATHAN_NOEXCEPT = default;
                mutex_storage(const mutex_storage&) FOONATHAN_NOEXCEPT {}

                mutex_storage& operator=(const mutex_storage&) FOONATHAN_NOEXCEPT
                {
                    return *this;
                }

                void lock() const
                {
                    mutex_.lock();
                }

                void unlock() const FOONATHAN_NOEXCEPT
                {
                    mutex_.unlock();
                }

            protected:
                ~mutex_storage() FOONATHAN_NOEXCEPT = default;

            private:
                mutable Mutex mutex_;
            };

            template <>
            class mutex_storage<no_mutex>
            {
            public:
                mutex_storage() FOONATHAN_NOEXCEPT = default;

                void lock() const FOONATHAN_NOEXCEPT {}
                void unlock() const FOONATHAN_NOEXCEPT {}

            protected:
                ~mutex_storage() FOONATHAN_NOEXCEPT = default;
            };

            // non changeable pointer to an Allocator that keeps a lock
            // I don't think EBO is necessary here...
            template <class Alloc, class Mutex>
            class locked_allocator
            {
            public:
                locked_allocator(Alloc& alloc, Mutex& m) FOONATHAN_NOEXCEPT : mutex_(&m),
                                                                              alloc_(&alloc)
                {
                    mutex_->lock();
                }

                locked_allocator(locked_allocator&& other) FOONATHAN_NOEXCEPT
                : mutex_(other.mutex_),
                  alloc_(other.alloc_)
                {
                    other.mutex_ = nullptr;
                    other.alloc_ = nullptr;
                }

                ~locked_allocator() FOONATHAN_NOEXCEPT
                {
                    if (mutex_)
                        mutex_->unlock();
                }

                locked_allocator& operator=(locked_allocator&& other) FOONATHAN_NOEXCEPT = delete;

                Alloc& operator*() const FOONATHAN_NOEXCEPT
                {
                    FOONATHAN_MEMORY_ASSERT(alloc_);
                    return *alloc_;
                }

                Alloc* operator->() const FOONATHAN_NOEXCEPT
                {
                    FOONATHAN_MEMORY_ASSERT(alloc_);
                    return alloc_;
                }

            private:
                Mutex* mutex_; // don't use unqiue_lock to avoid dependency
                Alloc* alloc_;
            };

            template <class Alloc, class Mutex>
            locked_allocator<Alloc, Mutex> lock_allocator(Alloc& a, Mutex& m)
            {
                return {a, m};
            }
        } // namespace detail
    }     // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_THREADING_HPP_INCLUDED
