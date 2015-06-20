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
    namespace detail
    {
        struct accept_any
        {
            template <typename T>
            accept_any(T &&) {}
        };

        template <class C>
        struct is_derived_constructible_impl
        {
            static void ambig_if_valid(const C &);
            static void ambig_if_valid(accept_any);

            template <typename Arg,
                    typename = decltype(ambig_if_valid({std::declval<Arg>()}))>
            static std::false_type test(int);

            template <typename>
            static std::true_type test(...);
        };

        // is_constructible without access checks
        template <class C, typename Arg>
        using is_derived_constructible = decltype(is_derived_constructible_impl<C>::template test<Arg>(0));

        // whether or not a type is an instantiation of a template
        template <template <typename...> class Template, typename T>
        struct is_instantiation_of : std::false_type {};

        template <template <typename...> class Template, typename ... Args>
        struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};
    } // namespace detail

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
        static_assert(!detail::is_instantiation_of<allocator_storage, typename StoragePolicy::raw_allocator>::value,
            "don't pass it an allocator_storage, it would lead to double wrapping");

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
        template <class Alloc,
            typename = typename std::enable_if<
                detail::is_derived_constructible<storage_policy, Alloc>::value &&
                !std::is_base_of<allocator_storage, Alloc>::value>::type>
        allocator_storage(Alloc &&alloc)
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

        /// \brief Returns the stored allocator.
        /// \details It is most likely a reference
        /// but might be a temporary for stateless allocators.
        /// \note In case of a thread safe policy, this does not lock any mutexes.
        using storage_policy::get_allocator;

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

        RawAllocator& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const RawAllocator& get_allocator() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }

    protected:
        direct_storage(RawAllocator &&allocator)
        : RawAllocator(std::move(allocator)) {}

        direct_storage(direct_storage &&) = default;
        ~direct_storage() FOONATHAN_NOEXCEPT = default;
        direct_storage& operator=(direct_storage &&) = default;
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
        return {std::forward<RawAllocator>(allocator)};
    }

    /// \brief An allocator adapter that uses a mutex for synchronizing.
    /// \details It locks the mutex for each function called.<br>
    /// It is implemented via \ref allocator_storage with the \ref direct_storage policy.
    /// \ingroup memory
    template <class RawAllocator, class Mutex = std::mutex>
    using thread_safe_allocator = allocator_storage<direct_storage<RawAllocator>,
                                                    Mutex>;

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

        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> typename storage::reference_type
        {
            return storage::get_allocator();
        }

    protected:
        reference_storage(const raw_allocator &alloc = {}) FOONATHAN_NOEXCEPT
        : storage(alloc) {}

        reference_storage(raw_allocator &alloc) FOONATHAN_NOEXCEPT
        : storage(alloc) {}

        reference_storage(const reference_storage &) FOONATHAN_NOEXCEPT = default;
        ~reference_storage() FOONATHAN_NOEXCEPT = default;
        reference_storage& operator=(const reference_storage &) FOONATHAN_NOEXCEPT = default;
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

    protected:
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

        any_reference_storage(const any_reference_storage &other) FOONATHAN_NOEXCEPT
        {
            other.get_allocator().clone(&storage_);
        }

        // basic_allocator is trivially destructible
        ~any_reference_storage() FOONATHAN_NOEXCEPT = default;

        any_reference_storage& operator=(const any_reference_storage &other) FOONATHAN_NOEXCEPT
        {
            // no cleanup necessary
            other.get_allocator().clone(&storage_);
            return *this;
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
        template <class RawAllocator, class Mutex>
        struct allocator_reference_for
        {
            using type = allocator_reference<RawAllocator, Mutex>;
            using is_any = std::false_type;
        };

        template <class Mutex1, class Mutex2>
        struct allocator_reference_for<any_allocator_reference<Mutex1>, Mutex2>
        {
            // use user specified mutex, wrap in any_allocator_reference
            using type = any_allocator_reference<Mutex2>;
            using is_any = std::true_type;
        };

        template <class Storage, class Mutex1, class Mutex2>
        struct allocator_reference_for<allocator_storage<Storage, Mutex1>, Mutex2>
        {
            // use the user specified mutex
            using type = allocator_reference<typename Storage::raw_allocator, Mutex2>;
            using is_any = std::false_type;
        };
    } // namespace detail

    /// \brief Wraps a \ref concept::RawAllocator to create an \c std::allocator.
    /// \details To allow copying of the allocator, it will not store an object directly
    /// but instead wraps it into an allocator reference.
    /// This wrapping won't occur, if you pass it an allocator reference already,
    /// then it will stay in the reference.<br>
    /// If you instantiate it with a \ref any_allocator_reference, it will be kept,
    /// allowing it to store any allocator.
    /// \ingroup memory
    template <typename T, class RawAllocator, class Mutex = default_mutex>
    class std_allocator
    : detail::allocator_reference_for<RawAllocator, Mutex>::type
    {
        using is_any = typename detail::allocator_reference_for<RawAllocator, Mutex>::is_any;
        using alloc_reference = typename detail::allocator_reference_for<RawAllocator, Mutex>::type;

        static constexpr auto size = sizeof(T);
        static constexpr auto alignment = FOONATHAN_ALIGNOF(T);

    public:
        //=== typedefs ===//
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        /// \brief Whether or not to swap the allocators, if the containers are swapped.
        /// \details It is \c true to ensure that the allocator stays with its memory.
        using propagate_on_container_swap = std::true_type;

        /// \brief Whether or not the allocators are moved, if the containers are move assigned.
        /// \details It is \c true to allow fast move operations.
        using propagate_on_container_move_assignment = std::true_type;

        /// \brief Whether or not to copy the allocators, if the containers are copy assigned.
        /// \details It is \c true for consistency.
        using propagate_on_container_copy_assignment = std::true_type;

        template <typename U>
        struct rebind {using other = std_allocator<U, RawAllocator, Mutex>;};

        /// \brief The underlying raw allocator.
        /// \details Or the polymorphic base class in case or \ref any_allocator_reference.
        using impl_allocator = typename alloc_reference::raw_allocator;

        /// \brief The mutex used for synchronization.
        using mutex = Mutex;

        //=== constructor ===//
        /// \brief Default constructor is only available for stateless allocators.
        std_allocator() FOONATHAN_NOEXCEPT = default;

        /// \brief Creates it from a reference to a raw allocator.
        /// \details If the instantiation allows any allocator, it will accept any allocator,
        /// otherwise only the \ref impl_allocator.
        template <class RawAlloc,
                typename = typename std::enable_if<
                        // require RawAlloc to be impl_allocator if not polymorphic
                        (is_any::value ? true : std::is_same<RawAlloc, impl_allocator>::value)
                >::type>
        std_allocator(RawAlloc &alloc) FOONATHAN_NOEXCEPT
        : alloc_reference(alloc) {}

        /// \brief Creates it from another \ref alloc_reference or \ref any_allocator_reference.
        /// \details It must reference the same type and use the same mutex.
        std_allocator(const alloc_reference &alloc) FOONATHAN_NOEXCEPT
        : alloc_reference(alloc) {}

        /// \brief Conversion from any other \ref allocator_storage is forbidden.
        /// \details This prevents unnecessary nested wrapper classes.
        template <class StoragePolicy, class OtherMut>
        std_allocator(const allocator_storage<StoragePolicy, OtherMut>&) = delete;

        /// \brief Creates it from another \ref std_allocator allocating a different type.
        /// \details This is required by the \c Allcoator concept.
        template <typename U>
        std_allocator(const std_allocator<U, RawAllocator, Mutex> &alloc) FOONATHAN_NOEXCEPT
        : alloc_reference(alloc.get_allocator()) {}

        //=== allocation/deallocation ===//
        pointer allocate(size_type n, void * = nullptr)
        {
            return static_cast<pointer>(allocate_impl(is_any{}, n));
        }

        void deallocate(pointer p, size_type n) FOONATHAN_NOEXCEPT
        {
            deallocate_impl(is_any{}, p, n);
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

        /// \brief Returns the referenced allocator.
        /// \details It returns a reference for stateful allcoators and \ref any_allocator_reference,
        /// otherwise a temporary object.<br>
        /// \note This function does not lock the mutex.
        /// \see allocator_storage::get_allocator
        using alloc_reference::get_allocator;

        /// \brief Returns the referenced allocator while keeping the mutex locked.
        /// \details It returns a proxy object acting like a pointer to the allocator.
        /// \see allocator_storage::lock
        using alloc_reference::lock;

    private:
        // any_allocator_reference: use virtual function which already does a dispatch on node/array
        void* allocate_impl(std::true_type, size_type n)
        {
            return lock()->allocate_impl(n, size, alignment);
        }

        void deallocate_impl(std::true_type, void *ptr, size_type n)
        {
            lock()->deallocate_impl(ptr, n, size, alignment);
        }

        // alloc_reference: decide between node/array
        void* allocate_impl(std::false_type, size_type n)
        {
            if (n == 1)
                return this->allocate_node(size, alignment);
            else
                return this->allocate_array(n, size, alignment);
        }

        void deallocate_impl(std::false_type, void* ptr, size_type n)
        {
            if (n == 1)
                this->deallocate_node(ptr, size, alignment);
            else
                this->deallocate_array(ptr, n, size, alignment);
        }

        template <typename U> // stateful
        bool equal_to(std::true_type, const std_allocator<U, RawAllocator, mutex> &other) const FOONATHAN_NOEXCEPT
        {
            return &get_allocator() == &other.get_allocator();
        }

        template <typename U> // non-stateful
        bool equal_to(std::false_type, const std_allocator<U, RawAllocator, mutex> &) const FOONATHAN_NOEXCEPT
        {
            return true;
        }

        template <typename T1, typename T2, class Impl, class Mut>
        friend bool operator==(const std_allocator<T1, Impl, Mut> &lhs,
                               const std_allocator<T2, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT;
    };

    /// @{
    /// \brief Makes an \ref std_allocator.
    /// \relates std_allocator
    template <typename T, class RawAllocator>
    auto make_std_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> std_allocator<T, typename std::decay<RawAllocator>::type>
    {
        return {std::forward<RawAllocator>(allocator)};
    }

    template <typename T, class Mutex, class RawAllocator>
    auto make_std_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> std_allocator<T, typename std::decay<RawAllocator>::type, Mutex>
    {
        return {std::forward<RawAllocator>(allocator)};
    }
    /// @}

    /// @{
    /// \brief Compares to \ref std_allocator.
    /// \details They are equal if either stateless or reference the same allocator.
    /// \relates std_allocator
    template <typename T, typename U, class Impl, class Mut>
    bool operator==(const std_allocator<T, Impl, Mut> &lhs,
                    const std_allocator<U, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT
    {
        using allocator = typename decltype(lhs)::impl_allocator;
        return lhs.equal_to(typename allocator_traits<allocator>::is_stateful{}, rhs);
    }

    template <typename T, typename U, class Impl, class Mut>
    bool operator!=(const std_allocator<T, Impl, Mut> &lhs,
                    const std_allocator<U, Impl, Mut> &rhs) FOONATHAN_NOEXCEPT
    {
        return !(lhs == rhs);
    }
    /// @}

    /// \brief Handy typedef to create an \ref std_allocator that can take any raw allocator.
    /// \details It uses \ref any_allocator_reference and its type erasure.<br>
    /// It is just an instantiation of \ref std_allocator with \ref any_allocator_reference.
    /// \ingroup memory
    template <typename T, class Mutex = default_mutex>
    using any_allocator = std_allocator<T, any_allocator_reference<Mutex>, Mutex>;

    /// @{
    /// \brief Makes an \ref any_allocator.
    /// \relates any_allocator
    template <typename T, class RawAllocator>
    any_allocator<T> make_any_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    {
        return {std::forward<RawAllocator>(allocator)};
    }

    template <typename T, class Mutex, class RawAllocator>
    any_allocator<T, Mutex> make_any_allocator(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    {
        return {std::forward<RawAllocator>(allocator)};
    }
    /// @}
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
