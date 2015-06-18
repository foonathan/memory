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
    /// a constructor taking the object that is stored,
    /// and a \c get_allocator() function for \c const and \c non-const returning the allocator.
    /// \ingroup memory
    template <class StoragePolicy, class Mutex>
    class allocator_storage
    : StoragePolicy,
      detail::mutex_storage<detail::mutex_for<typename StoragePolicy::raw_allocator, Mutex>>
    {
        using traits = allocator_traits<typename StoragePolicy::raw_allocator>;
        using actual_mutex = const detail::mutex_storage<
                                detail::mutex_for<typename StoragePolicy::raw_allocator, Mutex>>;
    public:
        /// \brief The stored allocator type.
        using raw_allocator = typename StoragePolicy::raw_allocator;

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
        -> decltype(std::declval<const storage_policy>().get_allocator())
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
    using allocator_adapter = allocator_storage<direct_storage<RawAllocator>,
                                                dummy_mutex>;

    /// \brief Creates an \ref allocator_adapter.
    /// \relates allocator_adapter
    template <class RawAllocator>
    auto make_allocator_adapter(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> allocator_adapter<typename std::decay<RawAllocator>::type>
    {
        return (std::forward<RawAllocator>(allocator));
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
    template <class RawAllocator>
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
    using allocator_reference = allocator_storage<reference_storage<RawAllocator>, Mutex>;

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

    /// \brief A storage policy storing a reference to any allocator.
    /// \details For stateless allocators, it is constructed on the fly.<br>
    /// It uses type erasure via virtual functions to store the allocator.
    /// \ingroup memory
    class any_reference_storage
    {
        class base_allocator
        {
        public:
            using is_stateful = std::true_type;

            virtual void clone(void *storage) const FOONATHAN_NOEXCEPT = 0;

            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                return allocate_impl(1, size, alignment);
            }

            void* allocate_array(std::size_t count,
                                        std::size_t size, std::size_t alignment)
            {
                return allocate_impl(count, size, alignment);
            }

            void deallocate_node(void *node,
                        std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                deallocate_impl(node, 1, size, alignment);
            }

            void deallocate_array(void *array,
                        std::size_t count, std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                deallocate_impl(array, count, size, alignment);
            }

            // count 1 means node
            virtual void* allocate_impl(std::size_t count, std::size_t size,
                                        std::size_t alignment) = 0;
            virtual void deallocate_impl(void* ptr, std::size_t count,
                                         std::size_t size,
                                         std::size_t alignment) FOONATHAN_NOEXCEPT = 0;

            std::size_t max_node_size() const
            {
                return max(query::node_size);
            }

            std::size_t max_array_size() const
            {
                return max(query::array_size);
            }

            std::size_t max_alignment() const
            {
                return max(query::alignment);
            }

        private:
            enum class query
            {
                node_size,
                array_size,
                alignment
            };

            virtual std::size_t max(query q) const = 0;

        };
    public:
        using raw_allocator = base_allocator;

        template <class RawAllocator>
        any_reference_storage(RawAllocator &alloc) FOONATHAN_NOEXCEPT
        {
            static_assert(sizeof(basic_allocator<RawAllocator>)
                          <= sizeof(basic_allocator<default_instantiation>),
                    "requires all instantiations to have certain maximum size");
            ::new(static_cast<void*>(&storage_)) basic_allocator<RawAllocator>(alloc);
        }

        template <class RawAllocator>
        any_reference_storage(const RawAllocator &alloc) FOONATHAN_NOEXCEPT
        {
            static_assert(sizeof(basic_allocator<RawAllocator>)
                          <= sizeof(basic_allocator<default_instantiation>),
                          "requires all instantiations to have certain maximum size");
            ::new(static_cast<void*>(&storage_)) basic_allocator<RawAllocator>(alloc);
        }

        any_reference_storage(const base_allocator &alloc) FOONATHAN_NOEXCEPT
        {
            alloc.clone(&storage_);
        }

        raw_allocator& get_allocator() FOONATHAN_NOEXCEPT
        {
            auto mem = static_cast<void*>(&storage_);
            return *static_cast<base_allocator*>(mem);
        }

        const raw_allocator& get_allocator() const FOONATHAN_NOEXCEPT
        {
            auto mem = static_cast<const void*>(&storage_);
            return *static_cast<const base_allocator*>(mem);
        }

    private:
        template <class RawAllocator>
        class basic_allocator
        : public base_allocator,
          private detail::reference_storage_impl<RawAllocator,
                        allocator_traits<RawAllocator>::is_stateful::value>
        {
            using traits = allocator_traits<RawAllocator>;
            using storage = detail::reference_storage_impl<RawAllocator,
                    allocator_traits<RawAllocator>::is_stateful::value>;
        public:
            // non stateful
            basic_allocator(const RawAllocator &alloc) FOONATHAN_NOEXCEPT
            : storage(alloc) {}

            // stateful
            basic_allocator(RawAllocator &alloc) FOONATHAN_NOEXCEPT
            : storage(alloc) {}

        private:
            auto get() const FOONATHAN_NOEXCEPT
            -> typename storage::reference_type
            {
                return storage::get_allocator();
            }

            void clone(void *storage) const FOONATHAN_NOEXCEPT override
            {
                ::new(storage) basic_allocator(get());
            }

            void* allocate_impl(std::size_t count, std::size_t size,
                                std::size_t alignment) override
            {
                auto&& alloc = get();
                if (count == 1u)
                    return traits::allocate_node(alloc, size, alignment);
                return traits::allocate_array(alloc, count, size, alignment);
            }

            void deallocate_impl(void* ptr, std::size_t count,
                                 std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT override
            {
                auto&& alloc = get();
                if (count == 1u)
                    traits::deallocate_node(alloc, ptr, size, alignment);
                else
                    traits::deallocate_array(alloc, ptr, count, size, alignment);
            }

            std::size_t max(query q) const override
            {
                auto&& alloc = get();
                if (q == query::node_size)
                    return traits::max_node_size(alloc);
                else if (q == query::array_size)
                    return traits::max_array_size(alloc);
                return traits::max_alignment(alloc);
            }
        };

        // use a stateful instantiation to determine size and alignment
        struct dummy_allocator
        {
            using is_stateful = std::true_type;
        };

        using default_instantiation = basic_allocator<dummy_allocator>;
        using storage = typename
            std::aligned_storage<sizeof(default_instantiation),
                                 alignof(default_instantiation)>::type;
        storage storage_;
    };

    /// \brief Similar to \ref allocator_reference but can store a reference to any allocator type.
    /// \details A mutex can be specfied to lock any accesses.<br>
    /// It is implemented via \ref allocator_storage with the \ref any_reference_storage policy.
    /// \ingroup memory
    template <class Mutex = default_mutex>
    using any_allocator_reference = allocator_storage<any_reference_storage, Mutex>;

    /// @{
    /// \brief Creates a \ref any_allocator_reference.
    /// \relates any_allocator_reference
    template <class RawAllocator>
    auto make_any_allocator_reference(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> any_allocator_reference<>
    {
        return {std::forward<RawAllocator>(allocator)};
    }

    template <class Mutex, class RawAllocator>
    auto make_any_allocator_reference(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> any_allocator_reference<Mutex>
    {
        return {std::forward<RawAllocator>(allocator)};
    }
    /// @}

    namespace detail
    {
        template <typename T>
        class raw_allocator_allocator_base
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
        };
    } // namespace detail

    /// \brief Wraps a \ref concept::RawAllocator to create an \c std::allocator.
    /// \details It uses a \ref allocator_reference to store the allocator to allow copy constructing.<br>
    /// The underlying allocator is never moved, only the pointer to it.<br>
    /// \c propagate_on_container_swap is \c true to ensure that the allocator stays with its memory.
    /// \c propagate_on_container_move_assignment is \c true to allow fast move operations.
    /// \c propagate_on_container_copy_assignment is also \c true for consistency.
    /// \ingroup memory
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    class raw_allocator_allocator
    : public detail::raw_allocator_allocator_base<T>,
      private allocator_reference<RawAllocator, Mutex>
    {
        using base = detail::raw_allocator_allocator_base<T>;
        using reference = allocator_reference<RawAllocator, Mutex>;
    public:
        //=== typedefs ===//
        template <typename U>
        struct rebind {using other = raw_allocator_allocator<U, RawAllocator, Mutex>;};

        /// \brief The underlying raw allocator.
        using impl_allocator = RawAllocator;

        /// \brief The mutex used for synchronization.
        using mutex = Mutex;

        //=== constructor ===//
        /// \brief Creates it from a temporary raw allocator (only valid if stateless).
        raw_allocator_allocator(const impl_allocator &alloc = {}) FOONATHAN_NOEXCEPT
        : reference(alloc) {}

        /// \brief Creates it from a reference to a raw allocator.
        raw_allocator_allocator(impl_allocator &alloc) FOONATHAN_NOEXCEPT
        : reference(alloc) {}

        /// \brief Creates it from another \ref allocator_reference.
        /// \details It must reference the same type and use the same mutex.
        raw_allocator_allocator(const reference &alloc) FOONATHAN_NOEXCEPT
        : reference(alloc) {}

        /// \brief Conversion from any other \ref allocator_storage is forbidden.
        /// \details This avoids unnecessary nested wrapper.
        template <class StoragePolicy, class OtherMut>
        raw_allocator_allocator(const allocator_storage<StoragePolicy, OtherMut>&) = delete;

        /// \brief Creates it from another \ref raw_allocator_allocator allocating a different type.
        template <typename U>
        raw_allocator_allocator(const raw_allocator_allocator<U, RawAllocator, Mutex> &alloc) FOONATHAN_NOEXCEPT
        : reference(alloc.get_allocator()) {}

        //=== allocation/deallocation ===//
        typename base::pointer allocate(typename base::size_type n, void * = nullptr)
        {
            void *mem = nullptr;
            if (n == 1)
                mem = this->allocate_node(sizeof(typename base::value_type),
                                          FOONATHAN_ALIGNOF(typename base::value_type));
            else
                mem = this->allocate_array(n, sizeof(typename base::value_type),
                                           FOONATHAN_ALIGNOF(typename base::value_type));
            return static_cast<typename base::pointer>(mem);
        }

        void deallocate(typename base::pointer p, typename base::size_type n) FOONATHAN_NOEXCEPT
        {
            if (n == 1)
                this->deallocate_node(p, sizeof(typename base::value_type),
                                      FOONATHAN_ALIGNOF(typename base::value_type));
            else
                this->deallocate_array(p, n, sizeof(typename base::value_type),
                                       FOONATHAN_ALIGNOF(typename base::value_type));
        }

        //=== getter ===//
        typename base::size_type max_size() const FOONATHAN_NOEXCEPT
        {
            return this->max_array_size() / sizeof(typename base::value_type);
        }

        /// \brief Returns the referenced allocator.
        /// \details Depending on whether or not it is stateful,
        /// it is either a temporary or a reference.<br>
        /// \note This function does not lock the mutex.
        /// \see allocator_storage::get_allocator
        using reference::get_allocator;

        /// \brief Returns the referenced allocator while keeping the mutex locked.
        /// \details It returns a proxy object acting like a pointer to the allocator.
        /// \see allocator_storage::lock
        using reference::lock;

    private:
        template <typename U> // stateful
        bool equal_to(std::true_type, const raw_allocator_allocator<U, RawAllocator, mutex> &other) const FOONATHAN_NOEXCEPT
        {
            return &get_allocator() == &other.get_allocator();
        }

        template <typename U> // non-stateful
        bool equal_to(std::false_type, const raw_allocator_allocator<U, RawAllocator, mutex> &) const FOONATHAN_NOEXCEPT
        {
            return true;
        }

        template <typename T1, typename T2, class Impl, class Mut>
        friend bool operator==(const raw_allocator_allocator<T1, Impl, Mut> &lhs,
                               const raw_allocator_allocator<T2, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT;
    };

    /// \brief Makes an \ref raw_allocator_allocator.
    /// \relates raw_allocator_allocator
    template <typename T, class RawAllocator>
    auto make_std_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> raw_allocator_allocator<T, typename std::decay<RawAllocator>::type>
    {
        return {std::forward<RawAllocator>(allocator)};
    }

    /// @{
    /// \brief Compares to \ref raw_allocator_allocator.
    /// \details They are equal if either stateless or reference the same allocator.
    /// \relates raw_allocator_allocator
    template <typename T, typename U, class Impl, class Mut>
    bool operator==(const raw_allocator_allocator<T, Impl, Mut> &lhs,
                    const raw_allocator_allocator<U, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT
    {
        return lhs.equal_to(typename allocator_traits<Impl>::is_stateful{}, rhs);
    }

    template <typename T, typename U, class Impl, class Mut>
    bool operator!=(const raw_allocator_allocator<T, Impl, Mut> &lhs,
                    const raw_allocator_allocator<U, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT
    {
        return !(lhs == rhs);
    }
    /// @}

    namespace detail
    {
        template <template <typename...> class Template, typename T>
        struct is_instantiation_of : std::false_type {};

        template <template <typename...> class Template, typename ... Args>
        struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};
    } // namespace detail

    /// \brief Similar to \ref raw_allocator_allocator, but can take any raw allocator.
    /// \details It uses \ref any_allocator_reference and its type erasure.
    /// \ingroup memory
    template <typename T, class Mutex = default_mutex>
    class any_allocator
    : public detail::raw_allocator_allocator_base<T>,
      private any_allocator_reference<Mutex>
    {
        using base = detail::raw_allocator_allocator_base<T>;
        using reference = any_allocator_reference<Mutex>;
    public:
        //=== typedefs ===//
        template <typename U>
        struct rebind {using other = any_allocator<U, Mutex>;};

        /// \brief The underlying raw allocator.
        /// \details Since this type allows all allocators, it is a polymorphic base class.
        using impl_allocator = typename reference::raw_allocator;

        /// \brief The mutex used for synchronization.
        using mutex = Mutex;

        //=== constructor ===//
        /// \brief Creates it from a temporary raw allocator (only valid if stateless).
        template <class RawAllocator,
                  typename = typename std::enable_if<
                      !detail::is_instantiation_of<any_allocator, RawAllocator>::value &&
                      !detail::is_instantiation_of<raw_allocator_allocator, RawAllocator>::value &&
                      !detail::is_instantiation_of<allocator_storage, RawAllocator>::value
                    >::type>
        any_allocator(const RawAllocator &alloc = {}) FOONATHAN_NOEXCEPT
        : reference(alloc)
        {
           static_assert(!allocator_traits<RawAllocator>::is_stateful::value,
                "allocator must be stateless");
        }

        /// \brief Creates it from a reference to a raw allocator.
        template <class RawAllocator,
                typename = typename std::enable_if<
                        !detail::is_instantiation_of<any_allocator, RawAllocator>::value &&
                        !detail::is_instantiation_of<raw_allocator_allocator, RawAllocator>::value &&
                        !detail::is_instantiation_of<allocator_storage, RawAllocator>::value
                >::type>
        any_allocator(RawAllocator &alloc) FOONATHAN_NOEXCEPT
        : reference(alloc) {}

        /// \brief Creates it from another \ref any_allocator_reference.
        /// \details It must use the same mutex.
        any_allocator(const reference &alloc) FOONATHAN_NOEXCEPT
        : reference(alloc) {}

        /// \brief Creates it from a \ref allocator_reference using the same mutex.
        template <class RawAllocator>
        any_allocator(const allocator_reference<RawAllocator, mutex> &alloc) FOONATHAN_NOEXCEPT
        : reference(alloc.get_allocator()) {}

        /// \brief Conversion from any other \ref allocator_storage is forbidden.
        /// \details This avoids unnecessary nested wrapper.
        template <class StoragePolicy, class OtherMut>
        any_allocator(const allocator_storage<StoragePolicy, OtherMut>&) = delete;

        /// \brief Creates it from another \ref any_allocator allocating a different type.
        /// \details For consistency, it disallows allocator conversion here.
        template <typename U>
        any_allocator(const any_allocator<U, mutex> &alloc) FOONATHAN_NOEXCEPT
        : reference(alloc.get_allocator()) {}

        //=== allocation/deallocation ===//
        typename base::pointer allocate(typename base::size_type n, void * = nullptr)
        {
            // use virtual allocate_impl function that already does a conditional on node/array
            auto mem = get_allocator().allocate_impl(n, sizeof(typename base::value_type),
                                         FOONATHAN_ALIGNOF(typename base::value_type));
            return static_cast<typename base::pointer>(mem);
        }

        void deallocate(typename base::pointer p, typename base::size_type n) FOONATHAN_NOEXCEPT
        {
            // use virtual deallocate_impl function likewise
            get_allocator().deallocate_impl(p, n, sizeof(typename base::value_type),
                                   FOONATHAN_ALIGNOF(typename base::value_type));
        }

        //=== getter ===//
        typename base::size_type max_size() const FOONATHAN_NOEXCEPT
        {
            return this->max_array_size() / sizeof(typename base::value_type);
        }

        /// \brief Returns the referenced allocator.
        /// \details It is a reference to the polymorphic base class used.
        /// \note This function does not lock the mutex.
        /// \see allocator_storage::get_allocator
        using reference::get_allocator;

        /// \brief Returns the referenced allocator while keeping the mutex locked.
        /// \details It returns a proxy object acting like a pointer to the allocator.
        /// \see allocator_storage::lock
        using reference::lock;

        template <typename T1, typename T2, class Mut>
        friend bool operator==(const any_allocator<T1, Mut> &lhs,
                               const any_allocator<T2, Mut> &rhs) FOONATHAN_NOEXCEPT;
    };

    /// \brief Makes an \ref any_allocator.
    /// \relates any_allocator
    template <typename T, class RawAllocator>
    any_allocator<T> make_any_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    {
        return {std::forward<RawAllocator>(allocator)};
    }

    /// @{
    /// \brief Compares to \ref any_allocator.
    /// \details They are equal if they reference the same allocator.
    /// \relates any_allocator
    template <typename T, typename U, class Mut>
    bool operator==(const any_allocator<T, Mut> &lhs,
                    const any_allocator<U, Mut> &rhs) FOONATHAN_NOEXCEPT
    {
        return &lhs.get_allocator() == &rhs.get_allocator();
    }

    template <typename T, typename U, class Mut>
    bool operator!=(const any_allocator<T, Mut> &lhs,
                    const any_allocator<U, Mut> &rhs) FOONATHAN_NOEXCEPT
    {
        return !(lhs == rhs);
    }
    /// @}
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
