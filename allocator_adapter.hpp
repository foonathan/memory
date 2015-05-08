// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED

/// \file
/// \brief Adapters to wrap \ref concept::RawAllocator objects.

#include <limits>
#include <new>
#include <utility>

#include "config.hpp"
#include "allocator_traits.hpp"
#include "threading.hpp"
#include "tracking.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // stores a pointer to an allocator
        template <class RawAllocator, bool Stateful>
        class allocator_reference_impl
        {
        public:
            allocator_reference_impl(RawAllocator &allocator) noexcept
            : alloc_(&allocator) {}
            
        protected:
            ~allocator_reference_impl() = default;
            
            RawAllocator& get_allocator() const noexcept
            {
                return *alloc_;
            }
            
        private:
            RawAllocator *alloc_;
        };
        
        // doesn't store anything for stateless allocators
        // construct an instance on the fly
        template <class RawAllocator>
        class allocator_reference_impl<RawAllocator, false>
        {
        public:
            allocator_reference_impl() noexcept = default;
            allocator_reference_impl(const RawAllocator&) noexcept {}
            
        protected:
            ~allocator_reference_impl() = default;
            
            RawAllocator get_allocator() const noexcept
            {
                return {};
            }
        };
    } // namespace detail
    
    /// \brief A \ref concept::RawAllocator storing a pointer to an allocator, thus making it copyable.
    /// \detail It adapts any class by forwarding all requests to the stored allocator via the \ref allocator_traits.<br>
    /// A mutex or \ref dummy_mutex can be specified that is locked prior to accessing the allocator.<br>
    /// For stateless allocators there is no locking or storing overhead whatsover,
    /// they are just created as needed on the fly.
    /// \ingroup memory
    template <class RawAllocator, class Mutex = default_mutex>
    class allocator_reference
    : detail::allocator_reference_impl<RawAllocator, allocator_traits<RawAllocator>::is_stateful::value>,
      detail::mutex_storage<detail::mutex_for<RawAllocator, Mutex>>
    {
        using traits = allocator_traits<RawAllocator>;
        using storage = detail::allocator_reference_impl<RawAllocator, traits::is_stateful::value>;
        using actual_mutex = const detail::mutex_storage<detail::mutex_for<RawAllocator, Mutex>>;
    public:
        using raw_allocator = RawAllocator;
        using mutex = Mutex;
        
        using is_stateful = typename traits::is_stateful;

        /// \brief Creates it giving it the \ref allocator_type.
        /// \detail For non-stateful allocators, there exists a default-constructor and a version taking const-ref.
        /// For stateful allocators it takes a non-const reference.
        using storage::storage;
        
        /// @{
        /// \brief All concept functions lock the mutex and call the function on the referenced allocator.
        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            return traits::allocate_node(alloc, size, alignment);
        }
        
        void* allocate_array(std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            return traits::allocate_array(alloc, count, size, alignment);
        }

        void deallocate_node(void *ptr, std::size_t size, std::size_t alignment) noexcept
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            traits::deallocate_node(alloc, ptr, size, alignment);
        }
        
        void deallocate_array(void *array, std::size_t count,
                              std::size_t size, std::size_t alignment) noexcept
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            traits::deallocate_array(alloc, array, count, size, alignment);
        }
        
        std::size_t max_node_size() const
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            return traits::max_node_size(alloc);
        }
        
        std::size_t max_array_size() const
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            return traits::max_array_size(alloc);
        }
        
        std::size_t max_alignment() const
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            return traits::max_alignment(alloc);
        }
        /// @}
        
        /// @{
        /// \brief Returns a reference to the allocator while keeping it locked.
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
        
        /// \brief Returns the \ref raw_allocator.
        /// \detail It is a reference for stateful allocators and a temporary for non-stateful.
        /// \note This function does not perform any locking and is thus not thread safe.
        auto get_allocator() const noexcept
        -> decltype(this->storage::get_allocator())
        {
            return storage::get_allocator();
        }
    };
    
    /// @{
    /// \brief Creates a \ref allocator_reference.
    /// \relates allocator_reference
    template <class RawAllocator>
    auto make_allocator_reference(RawAllocator &&allocator) noexcept
    -> allocator_reference<typename std::decay<RawAllocator>::type> 
    {
        return {std::forward<RawAllocator>(allocator)};
    }
    
    template <class Mutex, class RawAllocator>
    auto make_allocator_reference(RawAllocator &&allocator) noexcept
    -> allocator_reference<typename std::decay<RawAllocator>::type, Mutex> 
    {
        return {std::forward<RawAllocator>(allocator)};
    }
    /// @}

    /// \brief Wraps a \ref concept::RawAllocator to create an \c std::allocator.
    ///
    /// It uses a \ref allocator_reference to store the allocator to allow copy constructing.<br>
    /// The underlying allocator is never moved, only the pointer to it.<br>
    /// \c propagate_on_container_swap is \c true to ensure that the allocator stays with its memory.
    /// \c propagate_on_container_move_assignment is \c true to allow fast move operations.
    /// \c propagate_on_container_copy_assignment is also \c true for consistency.
    /// \ingroup memory
    template <typename T, class RawAllocator>
    class raw_allocator_allocator
    : allocator_reference<RawAllocator>
    {
    public:
        //=== typedefs ===//
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        
        using propagate_on_container_swap = std::true_type;
        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;

        template <typename U>
        struct rebind {using other = raw_allocator_allocator<U, RawAllocator>;};

        using impl_allocator = RawAllocator;

        //=== constructor ===//
        raw_allocator_allocator() = default;
        
        using allocator_reference<RawAllocator>::allocator_reference;
        
        raw_allocator_allocator(const allocator_reference<RawAllocator> &alloc) noexcept
        : allocator_reference<RawAllocator>(alloc) {}

        template <typename U>
        raw_allocator_allocator(const raw_allocator_allocator<U, RawAllocator> &alloc) noexcept
        : allocator_reference<RawAllocator>(alloc.get_impl_allocator()) {}

        //=== allocation/deallocation ===//
        pointer allocate(size_type n, void * = nullptr)
        {
            void *mem = nullptr;
            if (n == 1)
                mem = this->allocate_node(sizeof(value_type), alignof(value_type));
            else
                mem = this->allocate_array(n, sizeof(value_type), alignof(value_type));
            return static_cast<pointer>(mem);
        }

        void deallocate(pointer p, size_type n) noexcept
        {
            if (n == 1)
                this->deallocate_node(p, sizeof(value_type), alignof(value_type));
            else
                this->deallocate_array(p, n, sizeof(value_type), alignof(value_type));
        }

        //=== construction/destruction ===//
        template <typename U, typename ... Args>
        void construct(U *p, Args&&... args)
        {
            void* mem = p;
            ::new(mem) U(std::forward<Args>(args)...);
        }

        template <typename U>
        void destroy(U *p) noexcept
        {
            p->~U();
        }

        //=== getter ===//
        size_type max_size() const noexcept
        {
            return this->max_array_size() / sizeof(value_type);
        }

        auto get_impl_allocator() const noexcept
        -> decltype(this->get_allocator())
        {
            return this->get_allocator();
        }
        
    private:
        template <typename U> // stateful
        bool equal_to(std::true_type, const raw_allocator_allocator<U, RawAllocator> &other) const noexcept
        {
            return &get_impl_allocator() == &other.get_impl_allocator();
        }
        
        template <typename U> // non=stateful
        bool equal_to(std::false_type, const raw_allocator_allocator<U, RawAllocator> &) const noexcept
        {
            return true;
        }
        
        template <typename T1, typename T2, class Impl>
        friend bool operator==(const raw_allocator_allocator<T1, Impl> &lhs,
                    const raw_allocator_allocator<T2, Impl> &rhs) noexcept;
    };

    /// \brief Makes an \ref raw_allocator_allocator.
    /// \relates raw_allocator_allocator
    template <typename T, class RawAllocator>
    auto make_std_allocator(RawAllocator &&allocator) noexcept
    -> raw_allocator_allocator<T, typename std::decay<RawAllocator>::type>
    {
        return {std::forward<RawAllocator>(allocator)};
    }
    
    template <typename T, typename U, class Impl>
    bool operator==(const raw_allocator_allocator<T, Impl> &lhs,
                    const raw_allocator_allocator<U, Impl> &rhs) noexcept
    {
        return lhs.equal_to(typename allocator_traits<Impl>::is_stateful{}, rhs);
    }

    template <typename T, typename U, class Impl>
    bool operator!=(const raw_allocator_allocator<T, Impl> &lhs,
                    const raw_allocator_allocator<U, Impl> &rhs) noexcept
    {
        return !(lhs == rhs);
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
