// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED

/// \file
/// \brief Adapters to wrap \ref concept::RawAllocator objects.

#include <limits>
#include <new>
#include <type_traits>
#include <utility>

#include "config.hpp"
#include "allocator_traits.hpp"
#include "threading.hpp"
#include "tracking.hpp"

namespace foonathan { namespace memory
{
    /// \brief Stores a raw allocator using a certain storage policy.
    /// \details Accesses are synchronized via a mutex.<br>
    /// The storage policy requires a typedef \c raw_allocator actually stored,
    /// a constructor taking any type that is stored,
    /// and a \c get_allocator() function for \c const and \c non-const returning the allocator.
    /// \ingroup memory
    template <class RawAllocator,
              class StoragePolicy,
              class Mutex>
    class allocator_storage
    : StoragePolicy,
      detail::mutex_storage<detail::mutex_for<typename StoragePolicy::raw_allocator, Mutex>>
    {
        using traits = allocator_traits<typename StoragePolicy::raw_allocator>;
        using actual_mutex = const detail::mutex_storage<
                                detail::mutex_for<typename StoragePolicy::raw_allocator, Mutex>>;
    public:
        /// \brief The stored allocator type.
        using raw_allocator = RawAllocator;

        /// \brief The used storage policy.
        using storage_policy = StoragePolicy;

        /// \brief The used mutex.
        using mutex = Mutex;

        /// \brief It is stateful, it the traits say so.
        using is_stateful = typename traits::is_stateful;

        /// \brief Passes it the allocator.
        /// \details Depending on the policy, it will either be moved
        /// or only its address taken.<br>
        /// The constructor is only available if it is valid.
        template <class Alloc>
        allocator_storage(Alloc &&alloc,
            typename std::enable_if<
            std::is_constructible<storage_policy,
                                  decltype(std::forward<Alloc>(alloc))
                                 >::value, void*>::type = nullptr)
        : storage_policy(std::forward<Alloc>(alloc)) {}

        /// @{
        /// \brief Forwards the function to the stored allocator.
        /// \details It uses the \ref allocator_traits to wrap the call.
        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            return traits::allocate_node(alloc, size, alignment);
        }

        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            return traits::allocate_array(alloc, count, size, alignment);
        }

        void deallocate_node(void *ptr, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            traits::deallocate_node(alloc, ptr, size, alignment);
        }

        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            std::lock_guard<actual_mutex> lock(*this);
            auto&& alloc = get_allocator();
            traits::deallocate_array(alloc, ptr, count, size, alignment);
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
        /// \brief Returns the stored allocator.
        /// \details It is most likely a reference
        /// but might be a temporary for stateless allocators.
        /// \note In case of a thread safe policy, this does not lock any mutexes.
        auto get_allocator() FOONATHAN_NOEXCEPT
        -> decltype(std::declval<storage_policy>().get_allocator())
        {
            return storage_policy::get_allocator();
        }

        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<storage_policy>().get_allocator())
        {
            return storage_policy::get_allocator();
        }
        /// @}

        /// @{
        /// \brief Returns a reference to the allocator while keeping it locked.
        /// \details It returns a proxy object that holds the lock.
        /// It has overloaded operator* and -> to give access to the allocator
        /// but it can't be reassigned to a different allocator object.
        detail::locked_allocator<raw_allocator, actual_mutex> lock() FOONATHAN_NOEXCEPT
        {
            return {get_allocator(), *this};
        }

        detail::locked_allocator<const raw_allocator, actual_mutex> lock() const FOONATHAN_NOEXCEPT
        {
            return {get_allocator(), *this};
        }
        /// @}.
    };

    /// \brief A direct storage policy.
    /// \details Just stores the allocator directly.
    /// \ingroup memory
    template <class RawAllocator>
    class direct_storage : RawAllocator
    {
    public:
        using raw_allocator = RawAllocator;

        direct_storage(RawAllocator &&allocator)
        : RawAllocator(std::move(allocator)) {}

        RawAllocator& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const RawAllocator& get_allocator() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }
    };

    /// \brief Wraps any class that has specialized the \ref allocator_traits and gives it the proper interface.
    /// \details It just forwards all function to the traits and makes it easier to use them.<br>
    /// It is implemented via \ref allocator_storage with the \ref direct_storage policy.
    /// It does not use a mutex, since there is no need.
    /// \ingroup memory
    template <class RawAllocator>
    using allocator_adapter = allocator_storage<RawAllocator,
                                                direct_storage<RawAllocator>,
                                                dummy_mutex>;

    /// \brief Creates an \ref allocator_adapter.
    /// \relates allocator_adapter
    template <class RawAllocator>
    auto make_allocator_adapter(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> allocator_adapter<typename std::decay<RawAllocator>::type>
    {
        return allocator_adapter<typename std::decay<RawAllocator>::type>
                    (std::forward<RawAllocator>(allocator));
    }

    namespace detail
    {
        // stores a pointer to an allocator
        template <class RawAllocator, bool Stateful>
        class reference_storage_impl
        {
        protected:
            reference_storage_impl(RawAllocator &allocator) FOONATHAN_NOEXCEPT
            : alloc_(&allocator) {}

            using reference_type = RawAllocator&;

            reference_type get_allocator() const FOONATHAN_NOEXCEPT
            {
                return *alloc_;
            }

        private:
            RawAllocator *alloc_;
        };

        // doesn't store anything for stateless allocators
        // construct an instance on the fly
        template <class RawAllocator>
        class reference_storage_impl<RawAllocator, false>
        {
        protected:
            reference_storage_impl(const RawAllocator &) FOONATHAN_NOEXCEPT {}

            using reference_type = RawAllocator;

            reference_type get_allocator() const FOONATHAN_NOEXCEPT
            {
                return {};
            }
        };
    } // namespace detail

    /// \brief A storage policy storing a reference to an allocator.
    /// \details For stateless allocators, it is constructed on the fly.
    /// \ingroup memory
    template <class RawAllocator, class Mutex = default_mutex>
    class reference_storage
    : detail::reference_storage_impl<RawAllocator,
        allocator_traits<RawAllocator>::is_stateful::value>
    {
        using storage = detail::reference_storage_impl<RawAllocator,
                            allocator_traits<RawAllocator>::is_stateful::value>;
    public:
        using raw_allocator = RawAllocator;

        reference_storage(const raw_allocator &alloc = {}) FOONATHAN_NOEXCEPT
        : storage(alloc) {}

        reference_storage(raw_allocator &alloc) FOONATHAN_NOEXCEPT
        : storage(alloc) {}

        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> typename storage::reference_type
        {
            return storage::get_allocator();
        }
    };

    /// \brief A \ref concept::RawAllocator storing a pointer to an allocator, thus making it copyable.
    /// \details It adapts any class by forwarding all requests to the stored allocator via the \ref allocator_traits.<br>
    /// A mutex or \ref dummy_mutex can be specified that is locked prior to accessing the allocator.<br>
    /// For stateless allocators there is no locking or storing overhead whatsover,
    /// they are just created as needed on the fly.<br>
    /// It is implemented via \ref allocator_storage with the \ref reference_storage policy.
    /// \ingroup memory
    template <class RawAllocator, class Mutex = default_mutex>
    using allocator_reference = allocator_storage<RawAllocator,
                                        reference_storage<RawAllocator>, Mutex>;

    /// @{
    /// \brief Creates a \ref allocator_reference.
    /// \relates allocator_reference
    template <class RawAllocator>
    auto make_allocator_reference(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> allocator_reference<typename std::decay<RawAllocator>::type>
    {
        return {std::forward<RawAllocator>(allocator)};
    }

    template <class Mutex, class RawAllocator>
    auto make_allocator_reference(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
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
        raw_allocator_allocator(const impl_allocator &alloc = {}) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(alloc) {}

        raw_allocator_allocator(impl_allocator &alloc) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(alloc) {}

        raw_allocator_allocator(const allocator_reference<RawAllocator> &alloc) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(alloc) {}

        template <typename U>
        raw_allocator_allocator(const raw_allocator_allocator<U, RawAllocator> &alloc) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(alloc.get_impl_allocator()) {}

        //=== allocation/deallocation ===//
        pointer allocate(size_type n, void * = nullptr)
        {
            void *mem = nullptr;
            if (n == 1)
                mem = this->allocate_node(sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
            else
                mem = this->allocate_array(n, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
            return static_cast<pointer>(mem);
        }

        void deallocate(pointer p, size_type n) FOONATHAN_NOEXCEPT
        {
            if (n == 1)
                this->deallocate_node(p, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
            else
                this->deallocate_array(p, n, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }

        //=== construction/destruction ===//
        template <typename U, typename ... Args>
        void construct(U *p, Args&&... args)
        {
            void* mem = p;
            ::new(mem) U(std::forward<Args>(args)...);
        }

        template <typename U>
        void destroy(U *p) FOONATHAN_NOEXCEPT
        {
            p->~U();
        }

        //=== getter ===//
        size_type max_size() const FOONATHAN_NOEXCEPT
        {
            return this->max_array_size() / sizeof(value_type);
        }

        auto get_impl_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<impl_allocator>>().get_allocator())
        {
            return this->get_allocator();
        }

    private:
        template <typename U> // stateful
        bool equal_to(std::true_type, const raw_allocator_allocator<U, RawAllocator> &other) const FOONATHAN_NOEXCEPT
        {
            return &get_impl_allocator() == &other.get_impl_allocator();
        }

        template <typename U> // non=stateful
        bool equal_to(std::false_type, const raw_allocator_allocator<U, RawAllocator> &) const FOONATHAN_NOEXCEPT
        {
            return true;
        }

        template <typename T1, typename T2, class Impl>
        friend bool operator==(const raw_allocator_allocator<T1, Impl> &lhs,
                    const raw_allocator_allocator<T2, Impl> &rhs) FOONATHAN_NOEXCEPT;
    };

    /// \brief Makes an \ref raw_allocator_allocator.
    /// \relates raw_allocator_allocator
    template <typename T, class RawAllocator>
    auto make_std_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> raw_allocator_allocator<T, typename std::decay<RawAllocator>::type>
    {
        return {std::forward<RawAllocator>(allocator)};
    }

    template <typename T, typename U, class Impl>
    bool operator==(const raw_allocator_allocator<T, Impl> &lhs,
                    const raw_allocator_allocator<U, Impl> &rhs) FOONATHAN_NOEXCEPT
    {
        return lhs.equal_to(typename allocator_traits<Impl>::is_stateful{}, rhs);
    }

    template <typename T, typename U, class Impl>
    bool operator!=(const raw_allocator_allocator<T, Impl> &lhs,
                    const raw_allocator_allocator<U, Impl> &rhs) FOONATHAN_NOEXCEPT
    {
        return !(lhs == rhs);
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
