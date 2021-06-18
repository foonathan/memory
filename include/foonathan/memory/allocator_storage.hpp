// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_ALLOCATOR_STORAGE_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_STORAGE_HPP_INCLUDED

/// \file
/// Class template \ref foonathan::memory::allocator_storage, some policies and resulting typedefs.

#include <new>
#include <type_traits>

#include "detail/utility.hpp"
#include "config.hpp"
#include "allocator_traits.hpp"
#include "threading.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            template <class Alloc>
            void* try_allocate_node(std::true_type, Alloc& alloc, std::size_t size,
                                    std::size_t alignment) noexcept
            {
                return composable_allocator_traits<Alloc>::try_allocate_node(alloc, size,
                                                                             alignment);
            }

            template <class Alloc>
            void* try_allocate_array(std::true_type, Alloc& alloc, std::size_t count,
                                     std::size_t size, std::size_t alignment) noexcept
            {
                return composable_allocator_traits<Alloc>::try_allocate_array(alloc, count, size,
                                                                              alignment);
            }

            template <class Alloc>
            bool try_deallocate_node(std::true_type, Alloc& alloc, void* ptr, std::size_t size,
                                     std::size_t alignment) noexcept
            {
                return composable_allocator_traits<Alloc>::try_deallocate_node(alloc, ptr, size,
                                                                               alignment);
            }

            template <class Alloc>
            bool try_deallocate_array(std::true_type, Alloc& alloc, void* ptr, std::size_t count,
                                      std::size_t size, std::size_t alignment) noexcept
            {
                return composable_allocator_traits<Alloc>::try_deallocate_array(alloc, ptr, count,
                                                                                size, alignment);
            }

            template <class Alloc>
            void* try_allocate_node(std::false_type, Alloc&, std::size_t, std::size_t) noexcept
            {
                FOONATHAN_MEMORY_UNREACHABLE("Allocator is not compositioning");
                return nullptr;
            }

            template <class Alloc>
            void* try_allocate_array(std::false_type, Alloc&, std::size_t, std::size_t,
                                     std::size_t) noexcept
            {
                FOONATHAN_MEMORY_UNREACHABLE("Allocator is not compositioning");
                return nullptr;
            }

            template <class Alloc>
            bool try_deallocate_node(std::false_type, Alloc&, void*, std::size_t,
                                     std::size_t) noexcept
            {
                FOONATHAN_MEMORY_UNREACHABLE("Allocator is not compositioning");
                return false;
            }

            template <class Alloc>
            bool try_deallocate_array(std::false_type, Alloc&, void*, std::size_t, std::size_t,
                                      std::size_t) noexcept
            {
                FOONATHAN_MEMORY_UNREACHABLE("Allocator is not compositioning");
                return false;
            }
        } // namespace detail

        /// A \concept{concept_rawallocator,RawAllocator} that stores another allocator.
        /// The \concept{concept_storagepolicy,StoragePolicy} defines the allocator type being stored and how it is stored.
        /// The \c Mutex controls synchronization of the access.
        /// \ingroup storage
        template <class StoragePolicy, class Mutex>
        class allocator_storage
        : FOONATHAN_EBO(StoragePolicy,
                        detail::mutex_storage<
                            detail::mutex_for<typename StoragePolicy::allocator_type, Mutex>>)
        {
            using traits = allocator_traits<typename StoragePolicy::allocator_type>;
            using composable_traits =
                composable_allocator_traits<typename StoragePolicy::allocator_type>;
            using composable   = is_composable_allocator<typename StoragePolicy::allocator_type>;
            using actual_mutex = const detail::mutex_storage<
                detail::mutex_for<typename StoragePolicy::allocator_type, Mutex>>;

        public:
            using allocator_type = typename StoragePolicy::allocator_type;
            using storage_policy = StoragePolicy;
            using mutex          = Mutex;
            using is_stateful    = typename traits::is_stateful;

            /// \effects Creates it by default-constructing the \c StoragePolicy.
            /// \requires The \c StoragePolicy must be default-constructible.
            /// \notes The default constructor may create an invalid allocator storage not associated with any allocator.
            /// If that is the case, it must not be used.
            allocator_storage() = default;

            /// \effects Creates it by passing it an allocator.
            /// The allocator will be forwarded to the \c StoragePolicy, it decides whether it will be moved, its address stored or something else.
            /// \requires The expression <tt>new storage_policy(std::forward<Alloc>(alloc))</tt> must be well-formed,
            /// otherwise this constructor does not participate in overload resolution.
            template <
                class Alloc,
                // MSVC seems to ignore access rights in SFINAE below
                // use this to prevent this constructor being chosen instead of move for types inheriting from it
                FOONATHAN_REQUIRES(
                    (!std::is_base_of<allocator_storage, typename std::decay<Alloc>::type>::value))>
            allocator_storage(Alloc&& alloc,
                              FOONATHAN_SFINAE(new storage_policy(detail::forward<Alloc>(alloc))))
            : storage_policy(detail::forward<Alloc>(alloc))
            {
            }

            /// \effects Creates it by passing it another \c allocator_storage with a different \c StoragePolicy but the same \c Mutex type.
            /// Initializes it with the result of \c other.get_allocator().
            /// \requires The expression <tt>new storage_policy(other.get_allocator())</tt> must be well-formed,
            /// otherwise this constructor does not participate in overload resolution.
            template <class OtherPolicy>
            allocator_storage(const allocator_storage<OtherPolicy, Mutex>& other,
                              FOONATHAN_SFINAE(new storage_policy(other.get_allocator())))
            : storage_policy(other.get_allocator())
            {
            }

            /// @{
            /// \effects Moves the \c allocator_storage object.
            /// A moved-out \c allocator_storage object must still store a valid allocator object.
            allocator_storage(allocator_storage&& other) noexcept
            : storage_policy(detail::move(other)),
              detail::mutex_storage<
                  detail::mutex_for<typename StoragePolicy::allocator_type, Mutex>>(
                  detail::move(other))
            {
            }

            allocator_storage& operator=(allocator_storage&& other) noexcept
            {
                storage_policy::                                 operator=(detail::move(other));
                detail::mutex_storage<detail::mutex_for<typename StoragePolicy::allocator_type,
                                                        Mutex>>::operator=(detail::move(other));
                return *this;
            }
            /// @}

            /// @{
            /// \effects Copies the \c allocator_storage object.
            /// \requires The \c StoragePolicy must be copyable.
            allocator_storage(const allocator_storage&) = default;
            allocator_storage& operator=(const allocator_storage&) = default;
            /// @}

            /// @{
            /// \effects Calls the function on the stored allocator.
            /// The \c Mutex will be locked during the operation.
            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return traits::allocate_node(alloc, size, alignment);
            }

            void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
            {
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return traits::allocate_array(alloc, count, size, alignment);
            }

            void deallocate_node(void* ptr, std::size_t size, std::size_t alignment) noexcept
            {
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                traits::deallocate_node(alloc, ptr, size, alignment);
            }

            void deallocate_array(void* ptr, std::size_t count, std::size_t size,
                                  std::size_t alignment) noexcept
            {
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                traits::deallocate_array(alloc, ptr, count, size, alignment);
            }

            std::size_t max_node_size() const
            {
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return traits::max_node_size(alloc);
            }

            std::size_t max_array_size() const
            {
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return traits::max_array_size(alloc);
            }

            std::size_t max_alignment() const
            {
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return traits::max_alignment(alloc);
            }
            /// @}

            /// @{
            /// \effects Calls the function on the stored composable allocator.
            /// The \c Mutex will be locked during the operation.
            /// \requires The allocator must be composable,
            /// i.e. \ref is_composable() must return `true`.
            /// \note This check is done at compile-time where possible,
            /// and at runtime in the case of type-erased storage.
            FOONATHAN_ENABLE_IF(composable::value)
            void* try_allocate_node(std::size_t size, std::size_t alignment) noexcept
            {
                FOONATHAN_MEMORY_ASSERT(is_composable());
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return composable_traits::try_allocate_node(alloc, size, alignment);
            }

            FOONATHAN_ENABLE_IF(composable::value)
            void* try_allocate_array(std::size_t count, std::size_t size,
                                     std::size_t alignment) noexcept
            {
                FOONATHAN_MEMORY_ASSERT(is_composable());
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return composable_traits::try_allocate_array(alloc, count, size, alignment);
            }

            FOONATHAN_ENABLE_IF(composable::value)
            bool try_deallocate_node(void* ptr, std::size_t size, std::size_t alignment) noexcept
            {
                FOONATHAN_MEMORY_ASSERT(is_composable());
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return composable_traits::try_deallocate_node(alloc, ptr, size, alignment);
            }

            FOONATHAN_ENABLE_IF(composable::value)
            bool try_deallocate_array(void* ptr, std::size_t count, std::size_t size,
                                      std::size_t alignment) noexcept
            {
                FOONATHAN_MEMORY_ASSERT(is_composable());
                std::lock_guard<actual_mutex> lock(*this);
                auto&&                        alloc = get_allocator();
                return composable_traits::try_deallocate_array(alloc, ptr, count, size, alignment);
            }
            /// @}

            /// @{
            /// \effects Forwards to the \c StoragePolicy.
            /// \returns Returns a reference to the stored allocator.
            /// \note This does not lock the \c Mutex.
            auto get_allocator() noexcept
                -> decltype(std::declval<storage_policy>().get_allocator())
            {
                return storage_policy::get_allocator();
            }

            auto get_allocator() const noexcept
                -> decltype(std::declval<const storage_policy>().get_allocator())
            {
                return storage_policy::get_allocator();
            }
            /// @}

            /// @{
            /// \returns A proxy object that acts like a pointer to the stored allocator.
            /// It cannot be reassigned to point to another allocator object and only moving is supported, which is destructive.
            /// As long as the proxy object lives and is not moved from, the \c Mutex will be kept locked.
            auto lock() noexcept -> FOONATHAN_IMPL_DEFINED(decltype(detail::lock_allocator(
                std::declval<storage_policy>().get_allocator(), std::declval<actual_mutex&>())))
            {
                return detail::lock_allocator(get_allocator(), static_cast<actual_mutex&>(*this));
            }

            auto lock() const noexcept -> FOONATHAN_IMPL_DEFINED(decltype(
                detail::lock_allocator(std::declval<const storage_policy>().get_allocator(),
                                       std::declval<actual_mutex&>())))
            {
                return detail::lock_allocator(get_allocator(), static_cast<actual_mutex&>(*this));
            }
            /// @}.

            /// \returns Whether or not the stored allocator is composable,
            /// that is you can use the compositioning functions.
            /// \note Due to type-erased allocators,
            /// this function can not be `constexpr`.
            bool is_composable() const noexcept
            {
                return StoragePolicy::is_composable();
            }
        };

        /// Tag type that enables type-erasure in \ref reference_storage.
        /// It can be used everywhere a \ref allocator_reference is used internally.
        /// \ingroup storage
        struct any_allocator
        {
        };

        /// A \concept{concept_storagepolicy,StoragePolicy} that stores the allocator directly.
        /// It embeds the allocator inside it, i.e. moving the storage policy will move the allocator.
        /// \ingroup storage
        template <class RawAllocator>
        class direct_storage : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
        {
            static_assert(!std::is_same<RawAllocator, any_allocator>::value,
                          "cannot type-erase in direct_storage");

        public:
            using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;

            /// \effects Creates it by default-constructing the allocator.
            /// \requires The \c RawAllcoator must be default constructible.
            direct_storage() = default;

            /// \effects Creates it by moving in an allocator object.
            direct_storage(allocator_type&& allocator) noexcept
            : allocator_type(detail::move(allocator))
            {
            }

            /// @{
            /// \effects Moves the \c direct_storage object.
            /// This will move the stored allocator.
            direct_storage(direct_storage&& other) noexcept : allocator_type(detail::move(other)) {}

            direct_storage& operator=(direct_storage&& other) noexcept
            {
                allocator_type::operator=(detail::move(other));
                return *this;
            }
            /// @}

            /// @{
            /// \returns A (\c const) reference to the stored allocator.
            allocator_type& get_allocator() noexcept
            {
                return *this;
            }

            const allocator_type& get_allocator() const noexcept
            {
                return *this;
            }
            /// @}

        protected:
            ~direct_storage() noexcept = default;

            bool is_composable() const noexcept
            {
                return is_composable_allocator<allocator_type>::value;
            }
        };

        /// An alias template for \ref allocator_storage using the \ref direct_storage policy without a mutex.
        /// It has the effect of giving any \concept{concept_rawallocator,RawAllocator} the interface with all member functions,
        /// avoiding the need to wrap it inside the \ref allocator_traits.
        /// \ingroup storage
        template <class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(allocator_adapter,
                                 allocator_storage<direct_storage<RawAllocator>, no_mutex>);

        /// \returns A new \ref allocator_adapter object created by forwarding to the constructor.
        /// \relates allocator_adapter
        template <class RawAllocator>
        auto make_allocator_adapter(RawAllocator&& allocator) noexcept
            -> allocator_adapter<typename std::decay<RawAllocator>::type>
        {
            return {detail::forward<RawAllocator>(allocator)};
        }

/// An alias template for \ref allocator_storage using the \ref direct_storage policy with a mutex.
/// It has a similar effect as \ref allocator_adapter but performs synchronization.
/// The \c Mutex will default to \c std::mutex if threading is supported,
/// otherwise there is no default.
/// \ingroup storage
#if FOONATHAN_HOSTED_IMPLEMENTATION
        template <class RawAllocator, class Mutex = std::mutex>
        FOONATHAN_ALIAS_TEMPLATE(thread_safe_allocator,
                                 allocator_storage<direct_storage<RawAllocator>, Mutex>);
#else
        template <class RawAllocator, class Mutex>
        FOONATHAN_ALIAS_TEMPLATE(thread_safe_allocator,
                                 allocator_storage<direct_storage<RawAllocator>, Mutex>);
#endif

#if FOONATHAN_HOSTED_IMPLEMENTATION
        /// \returns A new \ref thread_safe_allocator object created by forwarding to the constructor/
        /// \relates thread_safe_allocator
        template <class RawAllocator>
        auto make_thread_safe_allocator(RawAllocator&& allocator)
            -> thread_safe_allocator<typename std::decay<RawAllocator>::type>
        {
            return detail::forward<RawAllocator>(allocator);
        }
#endif

        /// \returns A new \ref thread_safe_allocator object created by forwarding to the constructor,
        /// specifying a certain mutex type.
        /// \requires It requires threading support from the implementation.
        /// \relates thread_safe_allocator
        template <class Mutex, class RawAllocator>
        auto make_thread_safe_allocator(RawAllocator&& allocator)
            -> thread_safe_allocator<typename std::decay<RawAllocator>::type, Mutex>
        {
            return detail::forward<RawAllocator>(allocator);
        }

        namespace detail
        {
            struct reference_stateful
            {
            };
            struct reference_stateless
            {
            };
            struct reference_shared
            {
            };

            reference_stateful  reference_type(std::true_type stateful, std::false_type shared);
            reference_stateless reference_type(std::false_type stateful, std::true_type shared);
            reference_stateless reference_type(std::false_type stateful, std::false_type shared);
            reference_shared    reference_type(std::true_type stateful, std::true_type shared);

            template <class RawAllocator, class Tag>
            class reference_storage_impl;

            // reference to stateful: stores a pointer to an allocator
            template <class RawAllocator>
            class reference_storage_impl<RawAllocator, reference_stateful>
            {
            protected:
                reference_storage_impl() noexcept : alloc_(nullptr) {}

                reference_storage_impl(RawAllocator& allocator) noexcept : alloc_(&allocator) {}

                bool is_valid() const noexcept
                {
                    return alloc_ != nullptr;
                }

                RawAllocator& get_allocator() const noexcept
                {
                    FOONATHAN_MEMORY_ASSERT(alloc_ != nullptr);
                    return *alloc_;
                }

            private:
                RawAllocator* alloc_;
            };

            // reference to stateless: store in static storage
            template <class RawAllocator>
            class reference_storage_impl<RawAllocator, reference_stateless>
            {
            protected:
                reference_storage_impl() noexcept = default;

                reference_storage_impl(const RawAllocator&) noexcept {}

                bool is_valid() const noexcept
                {
                    return true;
                }

                RawAllocator& get_allocator() const noexcept
                {
                    static RawAllocator alloc;
                    return alloc;
                }
            };

            // reference to shared: stores RawAllocator directly
            template <class RawAllocator>
            class reference_storage_impl<RawAllocator, reference_shared>
            {
            protected:
                reference_storage_impl() noexcept = default;

                reference_storage_impl(const RawAllocator& alloc) noexcept : alloc_(alloc) {}

                bool is_valid() const noexcept
                {
                    return true;
                }

                RawAllocator& get_allocator() const noexcept
                {
                    return alloc_;
                }

            private:
                mutable RawAllocator alloc_;
            };
        } // namespace detail

        /// Specifies whether or not a \concept{concept_rawallocator,RawAllocator} has shared semantics.
        /// It is shared, if - like \ref allocator_reference - if multiple objects refer to the same internal allocator and if it can be copied.
        /// This sharing is stateful, however, stateless allocators are not considered shared in the meaning of this traits. <br>
        /// If a \c RawAllocator is shared, it will be directly embedded inside \ref reference_storage since it already provides \ref allocator_reference like semantics, so there is no need to add them manually,<br>
        /// Specialize it for your own types, if they provide sharing semantics and can be copied.
        /// They also must provide an `operator==` to check whether two allocators refer to the same shared one.
        /// \note This makes no guarantees about the lifetime of the shared object, the sharing allocators can either own or refer to a shared object.
        /// \ingroup storage
        template <class RawAllocator>
        struct is_shared_allocator : std::false_type
        {
        };

        /// A \concept{concept_storagepolicy,StoragePolicy} that stores a reference to an allocator.
        /// For stateful allocators it only stores a pointer to an allocator object and copying/moving only copies the pointer.
        /// For stateless allocators it does not store anything, an allocator will be constructed as needed.
        /// For allocators that are already shared (determined through \ref is_shared_allocator) it will store the allocator type directly.
        /// \note It does not take ownership over the allocator in the stateful case, the user has to ensure that the allocator object stays valid.
        /// In the other cases the lifetime does not matter.
        /// \ingroup storage
        template <class RawAllocator>
        class reference_storage
#ifndef DOXYGEN
        : FOONATHAN_EBO(detail::reference_storage_impl<
                        typename allocator_traits<RawAllocator>::allocator_type,
                        decltype(detail::reference_type(
                            typename allocator_traits<RawAllocator>::is_stateful{},
                            is_shared_allocator<RawAllocator>{}))>)
#endif
        {
            using storage = detail::reference_storage_impl<
                typename allocator_traits<RawAllocator>::allocator_type,
                decltype(
                    detail::reference_type(typename allocator_traits<RawAllocator>::is_stateful{},
                                           is_shared_allocator<RawAllocator>{}))>;

        public:
            using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;

            /// Default constructor.
            /// \effects If the allocator is stateless, this has no effect and the object is usable as an allocator.
            /// If the allocator is stateful, creates an invalid reference without any associated allocator.
            /// Then it must not be used.
            /// If the allocator is shared, default constructs the shared allocator.
            /// If the shared allocator does not have a default constructor, this constructor is ill-formed.
            reference_storage() noexcept = default;

            /// \effects Creates it from a stateless or shared allocator.
            /// It will not store anything, only creates the allocator as needed.
            /// \requires The \c RawAllocator is stateless or shared.
            reference_storage(const allocator_type& alloc) noexcept : storage(alloc) {}

            /// \effects Creates it from a reference to a stateful allocator.
            /// It will store a pointer to this allocator object.
            /// \note The user has to take care that the lifetime of the reference does not exceed the allocator lifetime.
            reference_storage(allocator_type& alloc) noexcept : storage(alloc) {}

            /// @{
            /// \effects Copies the \c allocator_reference object.
            /// Only copies the pointer to it in the stateful case.
            reference_storage(const reference_storage&) noexcept = default;
            reference_storage& operator=(const reference_storage&) noexcept = default;
            /// @}

            /// \returns Whether or not the reference is valid.
            /// It is only invalid, if it was created by the default constructor and the allocator is stateful.
            explicit operator bool() const noexcept
            {
                return storage::is_valid();
            }

            /// \returns Returns a reference to the allocator.
            /// \requires The reference must be valid.
            allocator_type& get_allocator() const noexcept
            {
                return storage::get_allocator();
            }

        protected:
            ~reference_storage() noexcept = default;

            bool is_composable() const noexcept
            {
                return is_composable_allocator<allocator_type>::value;
            }
        };

        /// Specialization of the class template \ref reference_storage that is type-erased.
        /// It is triggered by the tag type \ref any_allocator.
        /// The specialization can store a reference to any allocator type.
        /// \ingroup storage
        template <>
        class reference_storage<any_allocator>
        {
            class base_allocator
            {
            public:
                using is_stateful = std::true_type;

                virtual ~base_allocator() = default;

                virtual void clone(void* storage) const noexcept = 0;

                void* allocate_node(std::size_t size, std::size_t alignment)
                {
                    return allocate_impl(1, size, alignment);
                }

                void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
                {
                    return allocate_impl(count, size, alignment);
                }

                void deallocate_node(void* node, std::size_t size, std::size_t alignment) noexcept
                {
                    deallocate_impl(node, 1, size, alignment);
                }

                void deallocate_array(void* array, std::size_t count, std::size_t size,
                                      std::size_t alignment) noexcept
                {
                    deallocate_impl(array, count, size, alignment);
                }

                void* try_allocate_node(std::size_t size, std::size_t alignment) noexcept
                {
                    return try_allocate_impl(1, size, alignment);
                }

                void* try_allocate_array(std::size_t count, std::size_t size,
                                         std::size_t alignment) noexcept
                {
                    return try_allocate_impl(count, size, alignment);
                }

                bool try_deallocate_node(void* node, std::size_t size,
                                         std::size_t alignment) noexcept
                {
                    return try_deallocate_impl(node, 1, size, alignment);
                }

                bool try_deallocate_array(void* array, std::size_t count, std::size_t size,
                                          std::size_t alignment) noexcept
                {
                    return try_deallocate_impl(array, count, size, alignment);
                }

                // count 1 means node
                virtual void* allocate_impl(std::size_t count, std::size_t size,
                                            std::size_t alignment)            = 0;
                virtual void  deallocate_impl(void* ptr, std::size_t count, std::size_t size,
                                              std::size_t alignment) noexcept = 0;

                virtual void* try_allocate_impl(std::size_t count, std::size_t size,
                                                std::size_t alignment) noexcept = 0;

                virtual bool try_deallocate_impl(void* ptr, std::size_t count, std::size_t size,
                                                 std::size_t alignment) noexcept = 0;

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

                virtual bool is_composable() const noexcept = 0;

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
            using allocator_type = FOONATHAN_IMPL_DEFINED(base_allocator);

            /// \effects Creates it from a reference to any stateful \concept{concept_rawallocator,RawAllocator}.
            /// It will store a pointer to this allocator object.
            /// \note The user has to take care that the lifetime of the reference does not exceed the allocator lifetime.
            template <class RawAllocator>
            reference_storage(RawAllocator& alloc) noexcept
            {
                static_assert(sizeof(basic_allocator<RawAllocator>)
                                  <= sizeof(basic_allocator<default_instantiation>),
                              "requires all instantiations to have certain maximum size");
                ::new (static_cast<void*>(&storage_)) basic_allocator<RawAllocator>(alloc);
            }

            // \effects Creates it from any stateless \concept{concept_rawallocator,RawAllocator}.
            /// It will not store anything, only creates the allocator as needed.
            /// \requires The \c RawAllocator is stateless.
            template <class RawAllocator>
            reference_storage(
                const RawAllocator& alloc,
                FOONATHAN_REQUIRES(!allocator_traits<RawAllocator>::is_stateful::value)) noexcept
            {
                static_assert(sizeof(basic_allocator<RawAllocator>)
                                  <= sizeof(basic_allocator<default_instantiation>),
                              "requires all instantiations to have certain maximum size");
                ::new (static_cast<void*>(&storage_)) basic_allocator<RawAllocator>(alloc);
            }

            /// \effects Creates it from the internal base class for the type-erasure.
            /// Has the same effect as if the actual stored allocator were passed to the other constructor overloads.
            /// \note This constructor is used internally to avoid double-nesting.
            reference_storage(const FOONATHAN_IMPL_DEFINED(base_allocator) & alloc) noexcept
            {
                alloc.clone(&storage_);
            }

            /// \effects Creates it from the internal base class for the type-erasure.
            /// Has the same effect as if the actual stored allocator were passed to the other constructor overloads.
            /// \note This constructor is used internally to avoid double-nesting.
            reference_storage(FOONATHAN_IMPL_DEFINED(base_allocator) & alloc) noexcept
            : reference_storage(static_cast<const base_allocator&>(alloc))
            {
            }

            /// @{
            /// \effects Copies the \c reference_storage object.
            /// It only copies the pointer to the allocator.
            reference_storage(const reference_storage& other) noexcept
            {
                other.get_allocator().clone(&storage_);
            }

            reference_storage& operator=(const reference_storage& other) noexcept
            {
                get_allocator().~allocator_type();
                other.get_allocator().clone(&storage_);
                return *this;
            }
            /// @}

            /// \returns A reference to the allocator.
            /// The actual type is implementation-defined since it is the base class used in the type-erasure,
            /// but it provides the full \concept{concept_rawallocator,RawAllocator} member functions.
            /// \note There is no way to access any custom member functions of the allocator type.
            allocator_type& get_allocator() const noexcept
            {
                auto mem = static_cast<void*>(&storage_);
                return *static_cast<base_allocator*>(mem);
            }

        protected:
            ~reference_storage() noexcept
            {
                get_allocator().~allocator_type();
            }

            bool is_composable() const noexcept
            {
                return get_allocator().is_composable();
            }

        private:
            template <class RawAllocator>
            class basic_allocator
            : public base_allocator,
              private detail::reference_storage_impl<
                  typename allocator_traits<RawAllocator>::allocator_type,
                  decltype(
                      detail::reference_type(typename allocator_traits<RawAllocator>::is_stateful{},
                                             is_shared_allocator<RawAllocator>{}))>
            {
                using traits     = allocator_traits<RawAllocator>;
                using composable = is_composable_allocator<typename traits::allocator_type>;
                using storage    = detail::reference_storage_impl<
                    typename allocator_traits<RawAllocator>::allocator_type,
                    decltype(detail::reference_type(typename allocator_traits<
                                                        RawAllocator>::is_stateful{},
                                                    is_shared_allocator<RawAllocator>{}))>;

            public:
                // non stateful
                basic_allocator(const RawAllocator& alloc) noexcept : storage(alloc) {}

                // stateful
                basic_allocator(RawAllocator& alloc) noexcept : storage(alloc) {}

            private:
                typename traits::allocator_type& get() const noexcept
                {
                    return storage::get_allocator();
                }

                void clone(void* storage) const noexcept override
                {
                    ::new (storage) basic_allocator(get());
                }

                void* allocate_impl(std::size_t count, std::size_t size,
                                    std::size_t alignment) override
                {
                    auto&& alloc = get();
                    if (count == 1u)
                        return traits::allocate_node(alloc, size, alignment);
                    else
                        return traits::allocate_array(alloc, count, size, alignment);
                }

                void deallocate_impl(void* ptr, std::size_t count, std::size_t size,
                                     std::size_t alignment) noexcept override
                {
                    auto&& alloc = get();
                    if (count == 1u)
                        traits::deallocate_node(alloc, ptr, size, alignment);
                    else
                        traits::deallocate_array(alloc, ptr, count, size, alignment);
                }

                void* try_allocate_impl(std::size_t count, std::size_t size,
                                        std::size_t alignment) noexcept override
                {
                    auto&& alloc = get();
                    if (count == 1u)
                        return detail::try_allocate_node(composable{}, alloc, size, alignment);
                    else
                        return detail::try_allocate_array(composable{}, alloc, count, size,
                                                          alignment);
                }

                bool try_deallocate_impl(void* ptr, std::size_t count, std::size_t size,
                                         std::size_t alignment) noexcept override
                {
                    auto&& alloc = get();
                    if (count == 1u)
                        return detail::try_deallocate_node(composable{}, alloc, ptr, size,
                                                           alignment);
                    else
                        return detail::try_deallocate_array(composable{}, alloc, ptr, count, size,
                                                            alignment);
                }

                bool is_composable() const noexcept override
                {
                    return composable::value;
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
            alignas(default_instantiation) mutable char storage_[sizeof(default_instantiation)];
        };

        /// An alias template for \ref allocator_storage using the \ref reference_storage policy.
        /// It will store a reference to the given allocator type. The tag type \ref any_allocator enables type-erasure.
        /// Wrap the allocator in a \ref thread_safe_allocator if you want thread safety.
        /// \ingroup storage
        template <class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(allocator_reference,
                                 allocator_storage<reference_storage<RawAllocator>, no_mutex>);

        /// \returns A new \ref allocator_reference object by forwarding the allocator to the constructor.
        /// \relates allocator_reference
        template <class RawAllocator>
        auto make_allocator_reference(RawAllocator&& allocator) noexcept
            -> allocator_reference<typename std::decay<RawAllocator>::type>
        {
            return {detail::forward<RawAllocator>(allocator)};
        }

        /// An alias for the \ref reference_storage specialization using type-erasure.
        /// \ingroup storage
        using any_reference_storage = reference_storage<any_allocator>;

        /// An alias for \ref allocator_storage using the \ref any_reference_storage.
        /// It will store a reference to any \concept{concept_rawallocator,RawAllocator}.
        /// This is the same as passing the tag type \ref any_allocator to the alias \ref allocator_reference.
        /// Wrap the allocator in a \ref thread_safe_allocator if you want thread safety.
        /// \ingroup storage
        using any_allocator_reference = allocator_storage<any_reference_storage, no_mutex>;

        /// \returns A new \ref any_allocator_reference object by forwarding the allocator to the constructor.
        /// \relates any_allocator_reference
        template <class RawAllocator>
        auto make_any_allocator_reference(RawAllocator&& allocator) noexcept
            -> any_allocator_reference
        {
            return {detail::forward<RawAllocator>(allocator)};
        }
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_ALLOCATOR_STORAGE_HPP_INCLUDED
