// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_ALLOCATOR_STORAGE_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_STORAGE_HPP_INCLUDED

/// \file
/// \brief Adapters to wrap \ref concept::RawAllocator objects.

#include <new>
#include <type_traits>

#include "detail/utility.hpp"
#include "config.hpp"
#include "allocator_traits.hpp"
#include "threading.hpp"

namespace foonathan { namespace memory
{
    template <class StoragePolicy, class Mutex>
    class allocator_storage;

    namespace detail
    {
        // whether or not a type is an instantiation of a template
        template <template <typename...> class Template, typename T>
        struct is_instantiation_of : std::false_type {};

        template <template <typename...> class Template, typename ... Args>
        struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};

        // whether or not a class is derived from a template
        // (not really right implementation, more is_convertible, but works for the needs here)
        template <template <typename...> class Base, class Derived>
        struct is_base_of_template_impl
        {
            template <typename ... Args>
            static std::true_type check(const Base<Args...>&);

            static std::false_type check(...);
        };

        template <template <typename...> class Base, class Derived>
        using is_base_of_template = decltype(is_base_of_template_impl<Base, Derived>::check(std::declval<Derived>()));

        // whether or not the allocator of the storage policy is a raw allocator itself
        template <class StoragePolicy>
        using is_nested_policy = is_instantiation_of<allocator_storage, typename StoragePolicy::allocator_type>;

        // whether or not derived from allocator_storage template
        template <class C>
        using is_derived_from_allocator_storage = detail::is_base_of_template<allocator_storage, C>;
    } // namespace detail

    /// \brief Stores a raw allocator using a certain storage policy.
    /// \details Accesses are synchronized via a mutex.<br>
    /// The storage policy requires a typedef allocator type actually stored,
    /// a constructor taking the object that is stored,
    /// and a \c get_allocator() function for \c const and \c non-const returning the allocator.
    /// \ingroup memory
    template <class StoragePolicy, class Mutex>
    class allocator_storage
    : FOONATHAN_EBO(StoragePolicy,
        detail::mutex_storage<detail::mutex_for<typename StoragePolicy::allocator_type, Mutex>>)
    {
        static_assert(!detail::is_nested_policy<StoragePolicy>::value,
            "allocator_storage instantiated with another allocator_storage, double wrapping!");

        using traits = allocator_traits<typename StoragePolicy::allocator_type>;
        using actual_mutex = const detail::mutex_storage<
                                detail::mutex_for<typename StoragePolicy::allocator_type, Mutex>>;
    public:
        /// \brief The stored allocator type.
        using allocator_type = typename StoragePolicy::allocator_type;

        /// \brief The used storage policy.
        using storage_policy = StoragePolicy;

        /// \brief The used mutex.
        using mutex = Mutex;

        /// \brief It is stateful, it the traits say so.
        using is_stateful = typename traits::is_stateful;

        /// \brief Default constructor only available if supported by the policy.
        allocator_storage() = default;

        /// \brief Passes it the allocator.
        /// \details Depending on the policy, it will either be moved
        /// or only its address taken.<br>
        /// The constructor is only available if it is valid.
        template <class Alloc,
            // MSVC seems to ignore access rights in SFINAE below
            // use this to prevent this constructor being chosen instead of move for types inheriting from it, e.g. detail::block_list
            FOONATHAN_REQUIRES(!detail::is_derived_from_allocator_storage<typename std::decay<Alloc>::type>::value)>
        allocator_storage(Alloc &&alloc,
            FOONATHAN_SFINAE(new storage_policy(detail::forward<Alloc>(alloc))))
        : storage_policy(detail::forward<Alloc>(alloc)) {}

        /// \brief Passes it another \ref allocator_storage with a different storage policy but the same mutex.
        /// \details Initializes itself with the stored allocator, if it is valid.
        template <class OtherPolicy>
        allocator_storage(const allocator_storage<OtherPolicy, Mutex> &other,
            FOONATHAN_SFINAE(new storage_policy(other.get_allocator())))
        : storage_policy(other.get_allocator()) {}

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
        FOONATHAN_IMPL_DEFINED(detail::locked_allocator<allocator_type, actual_mutex>)
            lock() FOONATHAN_NOEXCEPT
        {
            return {get_allocator(), *this};
        }

        FOONATHAN_IMPL_DEFINED(detail::locked_allocator<const allocator_type, actual_mutex>)
            lock() const FOONATHAN_NOEXCEPT
        {
            return {get_allocator(), *this};
        }
        /// @}.
    };

    /// \brief Tag type to enable type-erasure in \ref reference_storage.
    /// \details It can be used everywhere an \ref allocator_reference is used internally.
    /// \ingroup memory
    struct any_allocator {};

    /// \brief A direct storage policy.
    /// \details Just stores the allocator directly.
    /// \ingroup memory
    template <class RawAllocator>
    class direct_storage : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
    {
        static_assert(!std::is_same<RawAllocator, any_allocator>::value,
                      "cannot type-erase in direct_storage");
    public:
        using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;

        direct_storage() = default;

        direct_storage(allocator_type &&allocator) FOONATHAN_NOEXCEPT
        : allocator_type(detail::move(allocator)) {}

        direct_storage(direct_storage &&other) FOONATHAN_NOEXCEPT
        : allocator_type(detail::move(other)) {}

        direct_storage& operator=(direct_storage &&other) FOONATHAN_NOEXCEPT
        {
            allocator_type::operator=(detail::move(other));
            return *this;
        }

        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const allocator_type& get_allocator() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }

    protected:
        ~direct_storage() FOONATHAN_NOEXCEPT = default;
    };

    /// \brief Wraps any class that has specialized the \ref allocator_traits and gives it the proper interface.
    /// \details It just forwards all function to the traits and makes it easier to use them.<br>
    /// It is implemented via \ref allocator_storage with the \ref direct_storage policy.
    /// It does not use a mutex, since there is no need.
    /// \ingroup memory
    template <class RawAllocator>
    FOONATHAN_ALIAS_TEMPLATE(allocator_adapter,
                             allocator_storage<direct_storage<RawAllocator>,
                                                dummy_mutex>);

    /// \brief Creates an \ref allocator_adapter.
    /// \relates allocator_adapter
    template <class RawAllocator>
    auto make_allocator_adapter(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> allocator_adapter<typename std::decay<RawAllocator>::type>
    {
        return {detail::forward<RawAllocator>(allocator)};
    }

    /// \brief An allocator adapter that uses a mutex for synchronizing.
    /// \details It locks the mutex for each function called.<br>
    /// It is implemented via \ref allocator_storage with the \ref direct_storage policy.
    /// \ingroup memory
#if FOONATHAN_HAS_THREADING_SUPPORT
    template <class RawAllocator, class Mutex = std::mutex>
    FOONATHAN_ALIAS_TEMPLATE(thread_safe_allocator,
                             allocator_storage<direct_storage<RawAllocator>, Mutex>;
#else
    template <class RawAllocator, class Mutex>
    FOONATHAN_ALIAS_TEMPLATE(thread_safe_allocator,
                              allocator_storage<direct_storage<RawAllocator>, Mutex>);
#endif

    /// @{
    /// \brief Creates a \ref thread_safe_allocator.
    /// \relates thread_safe_allocator
#if FOONATHAN_HAS_THREADING_SUPPORT
    template <class RawAllocator>
    auto make_thread_safe_allocator(RawAllocator &&allocator)
    -> thread_safe_allocator<typename std::decay<RawAllocator>::type>
    {
        return detail::forward<RawAllocator>(allocator);
    }
#endif

    template <class Mutex, class RawAllocator>
    auto make_thread_safe_allocator(RawAllocator &&allocator)
    -> thread_safe_allocator<typename std::decay<RawAllocator>::type, Mutex>
    {
        return detail::forward<RawAllocator>(allocator);
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
    : FOONATHAN_EBO(detail::reference_storage_impl<
        typename allocator_traits<RawAllocator>::allocator_type,
        allocator_traits<RawAllocator>::is_stateful::value>)
    {
        using storage = detail::reference_storage_impl<
                            typename allocator_traits<RawAllocator>::allocator_type,
                            allocator_traits<RawAllocator>::is_stateful::value>;
    public:
        using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;

        reference_storage(const allocator_type &alloc) FOONATHAN_NOEXCEPT
        : storage(alloc) {}

        reference_storage(allocator_type &alloc) FOONATHAN_NOEXCEPT
        : storage(alloc) {}

        reference_storage(const reference_storage &) FOONATHAN_NOEXCEPT = default;
        reference_storage& operator=(const reference_storage &)FOONATHAN_NOEXCEPT = default;

        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> typename storage::reference_type
        {
            return storage::get_allocator();
        }

    protected:
        ~reference_storage() FOONATHAN_NOEXCEPT = default;
    };

    /// \brief Specialization of \ref reference_storage that is type-erased.
    /// \details It uses type erasure via virtual functions to store the allocator.
    /// \ingroup
    template <>
    class reference_storage<any_allocator>
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

        protected:
            enum class query
            {
                node_size,
                array_size,
                alignment
            };

            virtual std::size_t max(query q) const = 0;
        };

    public:
        using allocator_type = base_allocator;

        // create from lvalue
        template <class RawAllocator>
        reference_storage(RawAllocator &alloc) FOONATHAN_NOEXCEPT
        {
            static_assert(sizeof(basic_allocator<RawAllocator>)
                          <= sizeof(basic_allocator<default_instantiation>),
                          "requires all instantiations to have certain maximum size");
            ::new(static_cast<void*>(&storage_)) basic_allocator<RawAllocator>(alloc);
        }

        // create from rvalue
        template <class RawAllocator>
        reference_storage(const RawAllocator &alloc,
                    FOONATHAN_REQUIRES(!allocator_traits<RawAllocator>::is_stateful::value)) FOONATHAN_NOEXCEPT
        {
            static_assert(sizeof(basic_allocator<RawAllocator>)
                          <= sizeof(basic_allocator<default_instantiation>),
                          "requires all instantiations to have certain maximum size");
            ::new(static_cast<void*>(&storage_)) basic_allocator<RawAllocator>(alloc);
        }

        // create from base_allocator
        reference_storage(const base_allocator &alloc) FOONATHAN_NOEXCEPT
        {
            alloc.clone(&storage_);
        }

        // copy
        reference_storage(const reference_storage &other) FOONATHAN_NOEXCEPT
        {
            other.get_allocator().clone(&storage_);
        }

        reference_storage& operator=(const reference_storage &other) FOONATHAN_NOEXCEPT
        {
            // no cleanup necessary
            other.get_allocator().clone(&storage_);
            return *this;
        }

        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            auto mem = static_cast<void*>(&storage_);
            return *static_cast<base_allocator*>(mem);
        }

        const allocator_type& get_allocator() const FOONATHAN_NOEXCEPT
        {
            auto mem = static_cast<const void*>(&storage_);
            return *static_cast<const base_allocator*>(mem);
        }

    protected:
        // basic_allocator is trivially destructible
        ~reference_storage() FOONATHAN_NOEXCEPT = default;

    private:
        template <class RawAllocator>
        class basic_allocator
        : public base_allocator,
          private detail::reference_storage_impl<
                  typename allocator_traits<RawAllocator>::allocator_type,
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
        // base_allocator is stateful
        using default_instantiation = basic_allocator<base_allocator>;
        using storage = std::aligned_storage<sizeof(default_instantiation),
                FOONATHAN_ALIGNOF(default_instantiation)>::type;
        storage storage_;
    };

    /// \brief A \ref concept::RawAllocator storing a pointer to an allocator, thus making it copyable.
    /// \details It adapts any class by forwarding all requests to the stored allocator via the \ref allocator_traits.<br>
    /// A mutex or \ref dummy_mutex can be specified that is locked prior to accessing the allocator.<br>
    /// For stateless allocators there is no locking or storing overhead whatsover,
    /// they are just created as needed on the fly.<br>
    /// The tag allocator type \ref any_allocator can be used to enable type-erasure.
    /// \ingroup memory
    template <class RawAllocator, class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(allocator_reference,
                             allocator_storage<reference_storage<RawAllocator>, Mutex>);

    /// @{
    /// \brief Creates a \ref allocator_reference.
    /// \relates allocator_reference
    template <class RawAllocator>
    auto make_allocator_reference(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> allocator_reference<typename std::decay<RawAllocator>::type>
    {
        return {detail::forward<RawAllocator>(allocator)};
    }

    template <class Mutex, class RawAllocator>
    auto make_allocator_reference(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> allocator_reference<typename std::decay<RawAllocator>::type, Mutex>
    {
        return {detail::forward<RawAllocator>(allocator)};
    }
    /// @}

    /// \brief A storage policy storing a reference to any allocator.
    /// \details It is a typedef for the type-erased reference storage
    /// \ingroup memory
    using any_reference_storage = reference_storage<any_allocator>;

    /// \brief Similar to \ref allocator_reference but can store a reference to any allocator type.
    /// \details A mutex can be specfied to lock any accesses.<br>
    /// It is an instantiation of \ref allocator_reference with the tag \ref any_allocator to enable type-erasure
    /// \ingroup memory
    template <class Mutex = default_mutex>
    FOONATHAN_ALIAS_TEMPLATE(any_allocator_reference,
                             allocator_storage<any_reference_storage, Mutex>);

    /// @{
    /// \brief Creates a \ref any_allocator_reference.
    /// \relates any_allocator_reference
    template <class RawAllocator>
    auto make_any_allocator_reference(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> any_allocator_reference<>
    {
        return {detail::forward<RawAllocator>(allocator)};
    }

    template <class Mutex, class RawAllocator>
    auto make_any_allocator_reference(RawAllocator &&allocator) FOONATHAN_NOEXCEPT
    -> any_allocator_reference<Mutex>
    {
        return {detail::forward<RawAllocator>(allocator)};
    }
    /// @}
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_STORAGE_HPP_INCLUDED
