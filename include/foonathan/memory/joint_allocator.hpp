// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_JOINT_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_JOINT_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class template \ref foonathan::memory::joint_ptr, \ref foonathan::memory::joint_allocator and related.

#include <initializer_list>
#include <new>

#include "detail/align.hpp"
#include "detail/memory_stack.hpp"
#include "detail/utility.hpp"
#include "allocator_storage.hpp"
#include "config.hpp"
#include "default_allocator.hpp"
#include "error.hpp"

namespace foonathan
{
    namespace memory
    {
        template <typename T, class RawAllocator>
        class joint_ptr;

        template <typename T>
        class joint_type;

        namespace detail
        {
            // the stack that allocates the joint memory
            class joint_stack
            {
            public:
                joint_stack(void* mem, std::size_t cap) FOONATHAN_NOEXCEPT
                : stack_(static_cast<char*>(mem)),
                  end_(static_cast<char*>(mem) + cap)
                {
                }

                void* allocate(std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
                {
                    return stack_.allocate(end_, size, alignment, 0u);
                }

                bool bump(std::size_t offset) FOONATHAN_NOEXCEPT
                {
                    if (offset > std::size_t(end_ - stack_.top()))
                        return false;
                    stack_.bump(offset);
                    return true;
                }

                char* top() FOONATHAN_NOEXCEPT
                {
                    return stack_.top();
                }

                const char* top() const FOONATHAN_NOEXCEPT
                {
                    return stack_.top();
                }

                void unwind(void* ptr) FOONATHAN_NOEXCEPT
                {
                    stack_.unwind(static_cast<char*>(ptr));
                }

                std::size_t capacity(const char* mem) const FOONATHAN_NOEXCEPT
                {
                    return std::size_t(end_ - mem);
                }

                std::size_t capacity_left() const FOONATHAN_NOEXCEPT
                {
                    return std::size_t(end_ - top());
                }

                std::size_t capacity_used(const char* mem) const FOONATHAN_NOEXCEPT
                {
                    return std::size_t(top() - mem);
                }

            private:
                detail::fixed_memory_stack stack_;
                char*                      end_;
            };

            template <typename T>
            detail::joint_stack& get_stack(joint_type<T>& obj) FOONATHAN_NOEXCEPT;

            template <typename T>
            const detail::joint_stack& get_stack(const joint_type<T>& obj) FOONATHAN_NOEXCEPT;
        } // namespace detail

        /// Tag type that can't be created.
        ///
        /// It isued by \ref joint_ptr.
        /// \ingroup memory allocator
        class joint
        {
            joint(std::size_t cap) FOONATHAN_NOEXCEPT : capacity(cap) {}

            std::size_t capacity;

            template <typename T, class RawAllocator>
            friend class joint_ptr;
            template <typename T>
            friend class joint_type;
        };

        /// Tag type to make the joint size more explicit.
        ///
        /// It is used by \ref joint_ptr.
        /// \ingroup memory allocator
        struct joint_size
        {
            std::size_t size;

            explicit joint_size(std::size_t s) FOONATHAN_NOEXCEPT : size(s) {}
        };

        /// CRTP base class for all objects that want to use joint memory.
        ///
        /// This will disable default copy/move operations
        /// and inserts additional members for the joint memory management.
        /// \ingroup memory allocator
        template <typename T>
        class joint_type
        {
        protected:
            /// \effects Creates the base class,
            /// the tag type cannot be created by the user.
            /// \note This ensures that you cannot create joint types yourself.
            joint_type(joint j) FOONATHAN_NOEXCEPT;

            joint_type(const joint_type&) = delete;
            joint_type(joint_type&&)      = delete;

        private:
            detail::joint_stack stack_;

            template <typename U>
            friend detail::joint_stack& detail::get_stack(joint_type<U>& obj) FOONATHAN_NOEXCEPT;
            template <typename U>
            friend const detail::joint_stack& detail::get_stack(const joint_type<U>& obj)
                FOONATHAN_NOEXCEPT;
        };

        namespace detail
        {
            template <typename T>
            detail::joint_stack& get_stack(joint_type<T>& obj) FOONATHAN_NOEXCEPT
            {
                return obj.stack_;
            }

            template <typename T>
            const detail::joint_stack& get_stack(const joint_type<T>& obj) FOONATHAN_NOEXCEPT
            {
                return obj.stack_;
            }

            template <typename T>
            char* get_memory(joint_type<T>& obj) FOONATHAN_NOEXCEPT
            {
                auto mem = static_cast<void*>(&obj);
                return static_cast<char*>(mem) + sizeof(T);
            }

            template <typename T>
            const char* get_memory(const joint_type<T>& obj) FOONATHAN_NOEXCEPT
            {
                auto mem = static_cast<const void*>(&obj);
                return static_cast<const char*>(mem) + sizeof(T);
            }

        } // namespace detail

        template <typename T>
        joint_type<T>::joint_type(joint j) FOONATHAN_NOEXCEPT
        : stack_(detail::get_memory(*this), j.capacity)
        {
            FOONATHAN_MEMORY_ASSERT(stack_.top() == detail::get_memory(*this));
            FOONATHAN_MEMORY_ASSERT(stack_.capacity_left() == j.capacity);
        }

        /// A pointer to an object where all allocations are joint.
        ///
        /// It can either own an object or not (be `nullptr`).
        /// When it owns an object, it points to a memory block.
        /// This memory block contains both the actual object (of the type `T`)
        /// and space for allocations of `T`s members.
        ///
        /// The type `T` must be derived from \ref joint_type and every constructor must take \ref joint
        /// as first parameter.
        /// This prevents that you create joint objects yourself,
        /// without the additional storage.
        /// The default copy and move constructors are also deleted,
        /// you need to write them yourself.
        ///
        /// You can only access the object through the pointer,
        /// use \ref joint_allocator or \ref joint_array as members of `T`,
        /// to enable the memory sharing.
        /// If you are using \ref joint_allocator inside STL containers,
        /// make sure that you do not call their regular copy/move constructors,
        /// but instead the version where you pass an allocator.
        ///
        /// The memory block will be managed by the given \concept{concept_rawallocator,RawAllocator},
        /// it is stored in an \ref allocator_reference and not owned by the pointer directly.
        /// \ingroup memory allocator
        template <typename T, class RawAllocator>
        class joint_ptr : FOONATHAN_EBO(allocator_reference<RawAllocator>)
        {
            static_assert(std::is_base_of<joint_type<T>, T>::value,
                          "T must be derived of joint_type<T>");

        public:
            using element_type   = T;
            using allocator_type = typename allocator_reference<RawAllocator>::allocator_type;

            //=== constructors/destructor/assignment ===//
            /// @{
            /// \effects Creates it with a \concept{concept_rawallocator,RawAllocator}, but does not own a new object.
            explicit joint_ptr(allocator_type& alloc) FOONATHAN_NOEXCEPT
            : allocator_reference<RawAllocator>(alloc),
              ptr_(nullptr)
            {
            }

            explicit joint_ptr(const allocator_type& alloc) FOONATHAN_NOEXCEPT
            : allocator_reference<RawAllocator>(alloc),
              ptr_(nullptr)
            {
            }
            /// @}

            /// @{
            /// \effects Reserves memory for the object and the additional size,
            /// and creates the object by forwarding the arguments to its constructor.
            /// The \concept{concept_rawallocator,RawAllocator} will be used for the allocation.
            template <typename... Args>
            joint_ptr(allocator_type& alloc, joint_size additional_size, Args&&... args)
            : joint_ptr(alloc)
            {
                create(additional_size.size, detail::forward<Args>(args)...);
            }

            template <typename... Args>
            joint_ptr(const allocator_type& alloc, joint_size additional_size, Args&&... args)
            : joint_ptr(alloc)
            {
                create(additional_size.size, detail::forward<Args>(args)...);
            }
            /// @}

            /// \effects Move-constructs the pointer.
            /// Ownership will be transferred from `other` to the new object.
            joint_ptr(joint_ptr&& other) FOONATHAN_NOEXCEPT
            : allocator_reference<RawAllocator>(detail::move(other)),
              ptr_(other.ptr_)
            {
                other.ptr_ = nullptr;
            }

            /// \effects Destroys the object and deallocates its storage.
            ~joint_ptr() FOONATHAN_NOEXCEPT
            {
                reset();
            }

            /// \effects Move-assings the pointer.
            /// The previously owned object will be destroyed,
            /// and ownership of `other` transferred.
            joint_ptr& operator=(joint_ptr&& other) FOONATHAN_NOEXCEPT
            {
                joint_ptr tmp(detail::move(other));
                swap(*this, tmp);
                return *this;
            }

            /// \effects Same as `reset()`.
            joint_ptr& operator=(std::nullptr_t) FOONATHAN_NOEXCEPT
            {
                reset();
                return *this;
            }

            /// \effects Swaps to pointers and their ownership and allocator.
            friend void swap(joint_ptr& a, joint_ptr& b) FOONATHAN_NOEXCEPT
            {
                detail::adl_swap(static_cast<allocator_reference<RawAllocator>&>(a),
                                 static_cast<allocator_reference<RawAllocator>&>(b));
                detail::adl_swap(a.ptr_, b.ptr_);
            }

            //=== modifiers ===//
            /// \effects Destroys the object it refers to,
            /// if there is any.
            void reset() FOONATHAN_NOEXCEPT
            {
                if (ptr_)
                {
                    (**this).~element_type();
                    this->deallocate_node(ptr_,
                                          sizeof(element_type)
                                              + detail::get_stack(*ptr_).capacity(
                                                  detail::get_memory(*ptr_)),
                                          FOONATHAN_ALIGNOF(element_type));
                    ptr_ = nullptr;
                }
            }

            //=== accessors ===//
            /// \returns `true` if the pointer does own an object,
            /// `false` otherwise.
            explicit operator bool() const FOONATHAN_NOEXCEPT
            {
                return ptr_ != nullptr;
            }

            /// \returns A reference to the object it owns.
            /// \requires The pointer must own an object,
            /// i.e. `operator bool()` must return `true`.
            element_type& operator*() const FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_ASSERT(ptr_);
                return *get();
            }

            /// \returns A pointer to the object it owns.
            /// \requires The pointer must own an object,
            /// i.e. `operator bool()` must return `true`.
            element_type* operator->() const FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_ASSERT(ptr_);
                return get();
            }

            /// \returns A pointer to the object it owns
            /// or `nullptr`, if it does not own any object.
            element_type* get() const FOONATHAN_NOEXCEPT
            {
                return static_cast<element_type*>(ptr_);
            }

            /// \returns A reference to the allocator it will use for the deallocation.
            auto get_allocator() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<allocator_reference<allocator_type>>().get_allocator())
            {
                return this->allocator_reference<allocator_type>::get_allocator();
            }

        private:
            template <typename... Args>
            void create(std::size_t additional_size, Args&&... args)
            {
                auto mem = this->allocate_node(sizeof(element_type) + additional_size,
                                               FOONATHAN_ALIGNOF(element_type));

                element_type* ptr = nullptr;
                FOONATHAN_TRY
                {
                    ptr = ::new (mem)
                        element_type(joint(additional_size), detail::forward<Args>(args)...);
                }
                FOONATHAN_CATCH_ALL
                {
                    this->deallocate_node(mem, sizeof(element_type) + additional_size,
                                          FOONATHAN_ALIGNOF(element_type));
                    FOONATHAN_RETHROW;
                }
                ptr_ = ptr;
            }

            joint_type<T>* ptr_;

            friend class joint_allocator;
        };

        /// @{
        /// \returns `!ptr`,
        /// i.e. if `ptr` does not own anything.
        /// \relates joint_ptr
        template <typename T, class RawAllocator>
        bool operator==(const joint_ptr<T, RawAllocator>& ptr, std::nullptr_t)
        {
            return !ptr;
        }

        template <typename T, class RawAllocator>
        bool operator==(std::nullptr_t, const joint_ptr<T, RawAllocator>& ptr)
        {
            return ptr == nullptr;
        }
        /// @}

        /// @{
        /// \returns `ptr.get() == p`,
        /// i.e. if `ptr` ownws the object referred to by `p`.
        /// \relates joint_ptr
        template <typename T, class RawAllocator>
        bool operator==(const joint_ptr<T, RawAllocator>& ptr, T* p)
        {
            return ptr.get() == p;
        }

        template <typename T, class RawAllocator>
        bool operator==(T* p, const joint_ptr<T, RawAllocator>& ptr)
        {
            return ptr == p;
        }
        /// @}

        /// @{
        /// \returns `!(ptr == nullptr)`,
        /// i.e. if `ptr` does own something.
        /// \relates joint_ptr
        template <typename T, class RawAllocator>
        bool operator!=(const joint_ptr<T, RawAllocator>& ptr, std::nullptr_t)
        {
            return !(ptr == nullptr);
        }

        template <typename T, class RawAllocator>
        bool operator!=(std::nullptr_t, const joint_ptr<T, RawAllocator>& ptr)
        {
            return ptr != nullptr;
        }
        /// @}

        /// @{
        /// \returns `!(ptr == p)`,
        /// i.e. if `ptr` does not ownw the object referred to by `p`.
        /// \relates joint_ptr
        template <typename T, class RawAllocator>
        bool operator!=(const joint_ptr<T, RawAllocator>& ptr, T* p)
        {
            return !(ptr == p);
        }

        template <typename T, class RawAllocator>
        bool operator!=(T* p, const joint_ptr<T, RawAllocator>& ptr)
        {
            return ptr != p;
        }
        /// @}

        /// @{
        /// \returns A new \ref joint_ptr as if created with the same arguments passed to the constructor.
        /// \relatesalso joint_ptr
        /// \ingroup memory allocator
        template <typename T, class RawAllocator, typename... Args>
        auto allocate_joint(RawAllocator& alloc, joint_size additional_size, Args&&... args)
            -> joint_ptr<T, RawAllocator>
        {
            return joint_ptr<T, RawAllocator>(alloc, additional_size,
                                              detail::forward<Args>(args)...);
        }

        template <typename T, class RawAllocator, typename... Args>
        auto allocate_joint(const RawAllocator& alloc, joint_size additional_size, Args&&... args)
            -> joint_ptr<T, RawAllocator>
        {
            return joint_ptr<T, RawAllocator>(alloc, additional_size,
                                              detail::forward<Args>(args)...);
        }
        /// @}

        /// @{
        /// \returns A new \ref joint_ptr that points to a copy of `joint`.
        /// It will allocate as much memory as needed and forward to the copy constructor.
        /// \ingroup memory allocator
        template <class RawAllocator, typename T>
        auto clone_joint(RawAllocator& alloc, const joint_type<T>& joint)
            -> joint_ptr<T, RawAllocator>
        {
            return joint_ptr<T, RawAllocator>(alloc,
                                              joint_size(detail::get_stack(joint).capacity_used(
                                                  detail::get_memory(joint))),
                                              static_cast<const T&>(joint));
        }

        template <class RawAllocator, typename T>
        auto clone_joint(const RawAllocator& alloc, const joint_type<T>& joint)
            -> joint_ptr<T, RawAllocator>
        {
            return joint_ptr<T, RawAllocator>(alloc,
                                              joint_size(detail::get_stack(joint).capacity_used(
                                                  detail::get_memory(joint))),
                                              static_cast<const T&>(joint));
        }
        /// @}

        /// A \concept{concept_rawallocator,RawAllocator} that uses the additional joint memory for its allocation.
        ///
        /// It is somewhat limited and allows only allocation once.
        /// All joint allocators for an object share the joint memory and must not be used in multiple threads.
        /// The memory it returns is owned by a \ref joint_ptr and will be destroyed through it.
        /// \ingroup memory allocator
        class joint_allocator
        {
        public:
#if defined(__GNUC__) && (!defined(_GLIBCXX_USE_CXX11_ABI) || _GLIBCXX_USE_CXX11_ABI == 0)
            // std::string requires default constructor for the small string optimization when using gcc's old ABI
            // so add one, but it must never be used for allocation
            joint_allocator() FOONATHAN_NOEXCEPT : stack_(nullptr) {}
#endif

            /// \effects Creates it using the joint memory of the given object.
            template <typename T>
            joint_allocator(joint_type<T>& j) FOONATHAN_NOEXCEPT : stack_(&detail::get_stack(j))
            {
            }

            joint_allocator(const joint_allocator& other) FOONATHAN_NOEXCEPT = default;
            joint_allocator& operator=(const joint_allocator& other) FOONATHAN_NOEXCEPT = default;

            /// \effects Allocates a node with given properties.
            /// \returns A pointer to the new node.
            /// \throws \ref out_of_fixed_memory exception if this function has been called for a second time
            /// or the joint memory block is exhausted.
            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                FOONATHAN_MEMORY_ASSERT(stack_);
                auto mem = stack_->allocate(size, alignment);
                if (!mem)
                    FOONATHAN_THROW(out_of_fixed_memory(info(), size));
                return mem;
            }

            /// \effects Deallocates the node, if possible.
            /// \note It is only possible if it was the last allocation.
            void deallocate_node(void* ptr, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_ASSERT(stack_);
                auto end = static_cast<char*>(ptr) + size;
                if (end == stack_->top())
                    stack_->unwind(ptr);
            }

        private:
            allocator_info info() const FOONATHAN_NOEXCEPT
            {
                return allocator_info(FOONATHAN_MEMORY_LOG_PREFIX "::joint_allocator", this);
            }

            detail::joint_stack* stack_;

            friend bool operator==(const joint_allocator& lhs,
                                   const joint_allocator& rhs) FOONATHAN_NOEXCEPT;
        };

        /// @{
        /// \returns Whether `lhs` and `rhs` use the same joint memory for the allocation.
        /// \relates joint_allocator
        inline bool operator==(const joint_allocator& lhs,
                               const joint_allocator& rhs) FOONATHAN_NOEXCEPT
        {
            return lhs.stack_ == rhs.stack_;
        }

        inline bool operator!=(const joint_allocator& lhs,
                               const joint_allocator& rhs) FOONATHAN_NOEXCEPT
        {
            return !(lhs == rhs);
        }
        /// @}

        /// Specialization of \ref is_shared_allocator to mark \ref joint_allocator as shared.
        /// This allows using it as \ref allocator_reference directly.
        /// \ingroup memory allocator
        template <>
        struct is_shared_allocator<joint_allocator> : std::true_type
        {
        };

        /// Specialization of \ref is_thread_safe_allocator to mark \ref joint_allocator as thread safe.
        /// This is an optimization to get rid of the mutex in \ref allocator_storage,
        /// as joint allocator must not be shared between threads.
        /// \note The allocator is *not* thread safe, it just must not be shared.
        template <>
        struct is_thread_safe_allocator<joint_allocator> : std::true_type
        {
        };

#if !defined(DOXYGEN)
        template <class RawAllocator>
        struct propagation_traits;
#endif

        /// Specialization of the \ref propagation_traits for the \ref joint_allocator.
        /// A joint allocator does not propagate on assignment
        /// and it is not allowed to use the regular copy/move constructor of allocator aware containers,
        /// instead it needs the copy/move constructor with allocator.
        /// \note This is required because the container constructor will end up copying/moving the allocator.
        /// But this is not allowed as you need the allocator with the correct joined memory.
        /// Copying can be customized (i.e. forbidden), but sadly not move, so keep that in mind.
        /// \ingroup memory allocator
        template <>
        struct propagation_traits<joint_allocator>
        {
            using propagate_on_container_swap            = std::false_type;
            using propagate_on_container_move_assignment = std::false_type;
            using propagate_on_container_copy_assignment = std::false_type;

            template <class AllocReference>
            static AllocReference select_on_container_copy_construction(const AllocReference&)
            {
                static_assert(always_false<AllocReference>::value,
                              "you must not use the regular copy constructor");
            }

        private:
            template <typename T>
            struct always_false : std::false_type
            {
            };
        };

        /// A zero overhead dynamic array using joint memory.
        ///
        /// If you use, e.g. `std::vector` with \ref joint_allocator,
        /// this has a slight additional overhead.
        /// This type is joint memory aware and has no overhead.
        ///
        /// It has a dynamic, but fixed size,
        /// it cannot grow after it has been created.
        /// \ingroup memory allocator
        template <typename T>
        class joint_array
        {
        public:
            using value_type     = T;
            using iterator       = value_type*;
            using const_iterator = const value_type*;

            //=== constructors ===//
            /// \effects Creates with `size` default-constructed objects using the specified joint memory.
            /// \throws \ref out_of_fixed_memory if `size` is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename JointType>
            joint_array(std::size_t size, joint_type<JointType>& j)
            : joint_array(detail::get_stack(j), size)
            {
            }

            /// \effects Creates with `size` copies of `val`  using the specified joint memory.
            /// \throws \ref out_of_fixed_memory if `size` is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename JointType>
            joint_array(std::size_t size, const value_type& val, joint_type<JointType>& j)
            : joint_array(detail::get_stack(j), size, val)
            {
            }

            /// \effects Creates with the copies of the objects in the initializer list using the specified joint memory.
            /// \throws \ref out_of_fixed_memory if the size is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename JointType>
            joint_array(std::initializer_list<value_type> ilist, joint_type<JointType>& j)
            : joint_array(detail::get_stack(j), ilist)
            {
            }

            /// \effects Creates it by forwarding each element of the range to `T`s constructor  using the specified joint memory.
            /// \throws \ref out_of_fixed_memory if the size is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename InIter, typename JointType,
                      typename = decltype(*std::declval<InIter&>()++)>
            joint_array(InIter begin, InIter end, joint_type<JointType>& j)
            : joint_array(detail::get_stack(j), begin, end)
            {
            }

            joint_array(const joint_array&) = delete;

            /// \effects Copy constructs each element from `other` into the storage of the specified joint memory.
            /// \throws \ref out_of_fixed_memory if the size is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename JointType>
            joint_array(const joint_array& other, joint_type<JointType>& j)
            : joint_array(detail::get_stack(j), other)
            {
            }

            joint_array(joint_array&&) = delete;

            /// \effects Move constructs each element from `other` into the storage of the specified joint memory.
            /// \throws \ref out_of_fixed_memory if the size is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename JointType>
            joint_array(joint_array&& other, joint_type<JointType>& j)
            : joint_array(detail::get_stack(j), detail::move(other))
            {
            }

            /// \effects Destroys all objects,
            /// but does not release the storage.
            ~joint_array() FOONATHAN_NOEXCEPT
            {
                for (std::size_t i = 0u; i != size_; ++i)
                    ptr_[i].~T();
            }

            joint_array& operator=(const joint_array&) = delete;
            joint_array& operator=(joint_array&&) = delete;

            //=== accessors ===//
            /// @{
            /// \returns A reference to the `i`th object.
            /// \requires `i < size()`.
            value_type& operator[](std::size_t i) FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_ASSERT(i < size_);
                return ptr_[i];
            }

            const value_type& operator[](std::size_t i) const FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_ASSERT(i < size_);
                return ptr_[i];
            }
            /// @}

            /// @{
            /// \returns A pointer to the first object.
            /// It points to contiguous memory and can be used to access the objects directly.
            value_type* data() FOONATHAN_NOEXCEPT
            {
                return ptr_;
            }

            const value_type* data() const FOONATHAN_NOEXCEPT
            {
                return ptr_;
            }
            /// @}

            /// @{
            /// \returns A random access iterator to the first element.
            iterator begin() FOONATHAN_NOEXCEPT
            {
                return ptr_;
            }

            const_iterator begin() const FOONATHAN_NOEXCEPT
            {
                return ptr_;
            }
            /// @}

            /// @{
            /// \returns A random access iterator one past the last element.
            iterator end() FOONATHAN_NOEXCEPT
            {
                return ptr_ + size_;
            }

            const_iterator end() const FOONATHAN_NOEXCEPT
            {
                return ptr_ + size_;
            }
            /// @}

            /// \returns The number of elements in the array.
            std::size_t size() const FOONATHAN_NOEXCEPT
            {
                return size_;
            }

            /// \returns `true` if the array is empty, `false` otherwise.
            bool empty() const FOONATHAN_NOEXCEPT
            {
                return size_ == 0u;
            }

        private:
            // allocate only
            struct allocate_only
            {
            };
            joint_array(allocate_only, detail::joint_stack& stack, std::size_t size)
            : ptr_(nullptr), size_(0u)
            {
                ptr_ = static_cast<T*>(stack.allocate(size * sizeof(T), FOONATHAN_ALIGNOF(T)));
                if (!ptr_)
                    FOONATHAN_THROW(out_of_fixed_memory(info(), size * sizeof(T)));
            }

            class builder
            {
            public:
                builder(detail::joint_stack& stack, T* ptr) FOONATHAN_NOEXCEPT : stack_(&stack),
                                                                                 objects_(ptr),
                                                                                 size_(0u)
                {
                }

                ~builder() FOONATHAN_NOEXCEPT
                {
                    for (std::size_t i = 0u; i != size_; ++i)
                        objects_[i].~T();

                    if (size_)
                        stack_->unwind(objects_);
                }

                builder(builder&&) = delete;
                builder& operator=(builder&&) = delete;

                template <typename... Args>
                T* create(Args&&... args)
                {
                    auto ptr = ::new (static_cast<void*>(&objects_[size_]))
                        T(detail::forward<Args>(args)...);
                    ++size_;
                    return ptr;
                }

                std::size_t size() const FOONATHAN_NOEXCEPT
                {
                    return size_;
                }

                std::size_t release() FOONATHAN_NOEXCEPT
                {
                    auto res = size_;
                    size_    = 0u;
                    return res;
                }

            private:
                detail::joint_stack* stack_;
                T*                   objects_;
                std::size_t          size_;
            };

            joint_array(detail::joint_stack& stack, std::size_t size)
            : joint_array(allocate_only{}, stack, size)
            {
                builder b(stack, ptr_);
                for (auto i = 0u; i != size; ++i)
                    b.create();
                size_ = b.release();
            }

            joint_array(detail::joint_stack& stack, std::size_t size, const value_type& value)
            : joint_array(allocate_only{}, stack, size)
            {
                builder b(stack, ptr_);
                for (auto i = 0u; i != size; ++i)
                    b.create(value);
                size_ = b.release();
            }

            joint_array(detail::joint_stack& stack, std::initializer_list<value_type> ilist)
            : joint_array(allocate_only{}, stack, ilist.size())
            {
                builder b(stack, ptr_);
                for (auto& elem : ilist)
                    b.create(elem);
                size_ = b.release();
            }

            joint_array(detail::joint_stack& stack, const joint_array& other)
            : joint_array(allocate_only{}, stack, other.size())
            {
                builder b(stack, ptr_);
                for (auto& elem : other)
                    b.create(elem);
                size_ = b.release();
            }

            joint_array(detail::joint_stack& stack, joint_array&& other)
            : joint_array(allocate_only{}, stack, other.size())
            {
                builder b(stack, ptr_);
                for (auto& elem : other)
                    b.create(detail::move(elem));
                size_ = b.release();
            }

            template <typename InIter>
            joint_array(detail::joint_stack& stack, InIter begin, InIter end)
            : ptr_(nullptr), size_(0u)
            {
                if (begin == end)
                    return;

                ptr_ = static_cast<T*>(stack.allocate(sizeof(T), FOONATHAN_ALIGNOF(T)));
                if (!ptr_)
                    FOONATHAN_THROW(out_of_fixed_memory(info(), sizeof(T)));

                builder b(stack, ptr_);
                b.create(*begin++);

                for (auto last = ptr_; begin != end; ++begin)
                {
                    // just bump stack to get more memory
                    if (!stack.bump(sizeof(T)))
                        FOONATHAN_THROW(out_of_fixed_memory(info(), b.size() * sizeof(T)));

                    auto cur = b.create(*begin);
                    FOONATHAN_MEMORY_ASSERT(last + 1 == cur);
                    last = cur;
                }

                size_ = b.release();
            }

            allocator_info info() const FOONATHAN_NOEXCEPT
            {
                return {FOONATHAN_MEMORY_LOG_PREFIX "::joint_array", this};
            }

            value_type* ptr_;
            std::size_t size_;
        };
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_JOINT_ALLOCATOR_HPP_INCLUDED
