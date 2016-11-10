// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_JOINT_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_JOINT_ALLOCATOR_HPP_INCLUDED

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
        namespace detail
        {
            // the stack that allocates the joint memory
            struct joinable_stack
            {
                detail::fixed_memory_stack stack;
                char*                      end;

                joinable_stack(char* top, char* end) FOONATHAN_NOEXCEPT : stack(top), end(end)
                {
                }

                void* allocate(std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
                {
                    return stack.allocate(end, size, alignment);
                }

                bool bump(std::size_t offset) FOONATHAN_NOEXCEPT
                {
                    if (offset > std::size_t(end - stack.top()))
                        return false;
                    stack.bump(offset);
                    return true;
                }

                void unwind(void* ptr) FOONATHAN_NOEXCEPT
                {
                    stack.unwind(static_cast<char*>(ptr));
                }
            };

            // manages object + storage for it
            template <typename T>
            class joint_storage
            {
            public:
                explicit joint_storage(std::size_t capacity) FOONATHAN_NOEXCEPT
                    : data_(memory(), memory() + capacity)
                {
                }

                ~joint_storage() FOONATHAN_NOEXCEPT = default;

                joint_storage(joint_storage&&)             = delete;
                joint_storage& operator==(joint_storage&&) = delete;

                char* memory() FOONATHAN_NOEXCEPT
                {
                    auto this_mem = static_cast<void*>(this);
                    return static_cast<char*>(this_mem) + sizeof(joint_storage<T>);
                }

                char* memory_end() FOONATHAN_NOEXCEPT
                {
                    return data_.stack.end;
                }

                char* top() FOONATHAN_NOEXCEPT
                {
                    return data_.stack.stack.top();
                }

                joinable_stack& stack() FOONATHAN_NOEXCEPT
                {
                    return data_.stack;
                }

                std::size_t capacity() FOONATHAN_NOEXCEPT
                {
                    return std::size_t(memory_end() - memory());
                }

                std::size_t capacity_left() FOONATHAN_NOEXCEPT
                {
                    return std::size_t(memory_end() - top());
                }

                std::size_t capacity_used() FOONATHAN_NOEXCEPT
                {
                    return std::size_t(top() - memory());
                }

                void* get_object_storage() FOONATHAN_NOEXCEPT
                {
                    return &data_.storage;
                }

                bool is_valid(T& obj)
                {
                    return get_object_storage() == static_cast<void*>(&obj)
                           && memory() <= memory_end() && capacity_left() <= capacity();
                }

            private:
                struct data_t
                {
                    FOONATHAN_ALIGNAS(T) char storage[sizeof(T)]; // must be first
                    joinable_stack stack;

                    explicit data_t(char* top, char* end) : stack(top, end)
                    {
                    }

                } data_;
            };
        } // namespace detail

        /// A pointer to an object where all allocations are joint.
        ///
        /// It can either own an object or not (be `nullptr`).
        /// When it owns an object, it points to a memory block.
        /// This memory block contains both the actual object (of the type `T`)
        /// and space for allocations of `T`s members.
        ///
        /// You can only access the object through the pointer,
        /// use \ref joint_allocator or \ref joint_array as members of `T`,
        /// to enable the memory sharing.
        ///
        /// The memory block will be managed by the given \concept{concept_rawallocator,RawAllocator},
        /// it is stored in an \ref allocator_reference and not owned by the pointer directly.
        /// \ingroup memory allocator
        template <typename T, class RawAllocator, class Mutex = default_mutex>
        class joint_ptr : FOONATHAN_EBO(allocator_reference<RawAllocator, Mutex>)
        {
        public:
            using joint_type = T;
            using allocator_type =
                typename allocator_reference<RawAllocator, Mutex>::allocator_type;
            using mutex = Mutex;

            //=== constructors/destructor/assignment ===//
            /// @{
            /// \effects Creates it with a \concept{concept_rawallocator,RawAllocator}, but does not own a new object.
            explicit joint_ptr(allocator_type& alloc) FOONATHAN_NOEXCEPT
                : allocator_reference<RawAllocator, Mutex>(alloc),
                  storage_(nullptr)
            {
            }

            explicit joint_ptr(const allocator_type& alloc) FOONATHAN_NOEXCEPT
                : allocator_reference<RawAllocator, Mutex>(alloc),
                  storage_(nullptr)
            {
            }
            /// @}

            /// @{
            /// \effects Reserves memory for the object and the additional size,
            /// and creates the object by forwarding the arguments to its constructor.
            /// The \cocnept{concept_rawallocator,RawAllocator} will be used for the allocation.
            template <typename... Args>
            joint_ptr(allocator_type& alloc, std::size_t additional_size, Args&&... args)
            : joint_ptr(alloc)
            {
                create(additional_size, detail::forward<Args>(args)...);
            }

            template <typename... Args>
            joint_ptr(const allocator_type& alloc, std::size_t additional_size, Args&&... args)
            : joint_ptr(alloc)
            {
                create(additional_size, std::forward<Args>(args)...);
            }
            /// @}

            /// \effects Move-constructs the pointer.
            /// Ownership will be transferred from `other` to the new object.
            joint_ptr(joint_ptr&& other) FOONATHAN_NOEXCEPT
                : allocator_reference<RawAllocator, Mutex>(detail::move(other)),
                  storage_(other.storage_)
            {
                other.storage_ = nullptr;
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
                detail::adl_swap(static_cast<allocator_reference<RawAllocator, Mutex>&>(a),
                                 static_cast<allocator_reference<RawAllocator, Mutex>&>(b));
                detail::adl_swap(a.storage_, b.storage_);
            }

            //=== modifiers ===//
            /// \effects Destroys the object it refers to,
            /// if there is any.
            void reset() FOONATHAN_NOEXCEPT
            {
                if (storage_)
                {
                    (**this).~joint_type();
                    storage_->~joint_storage();
                    this->deallocate_node(storage_, sizeof(*storage_) + storage_->capacity(),
                                          detail::max_alignment);
                    storage_ = nullptr;
                }
            }

            //=== accessors ===//
            /// \returns `true` if the pointer does own an object,
            /// `false` otherwise.
            explicit operator bool() const FOONATHAN_NOEXCEPT
            {
                return storage_ != nullptr;
            }

            /// \returns A reference to the object it owns.
            /// \requires The pointer must own an object,
            /// i.e. `operator bool()` must return `true`.
            joint_type& operator*() const FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_ASSERT(storage_);
                return *static_cast<joint_type*>(storage_->get_object_storage());
            }

            /// \returns A pointer to the object it owns.
            /// \requires The pointer must own an object,
            /// i.e. `operator bool()` must return `true`.
            joint_type* operator->() const FOONATHAN_NOEXCEPT
            {
                return &**this;
            }

            /// \returns A pointer to the object it owns
            /// or `nullptr`, if it does not own any object.
            joint_type* get() const FOONATHAN_NOEXCEPT
            {
                return storage_ ? static_cast<joint_type*>(storage_->get_object_storage()) :
                                  nullptr;
            }

            /// \returns A reference to the allocator it will use for the deallocation.
            auto get_allocator() const FOONATHAN_NOEXCEPT -> decltype(
                std::declval<allocator_reference<allocator_type, mutex>>().get_allocator())
            {
                return this->allocator_reference<allocator_type, mutex>::get_allocator();
            }

        private:
            template <typename... Args>
            void create(std::size_t additional_size, Args&&... args)
            {
                auto mem = this->allocate_node(sizeof(detail::joint_storage<T>) + additional_size,
                                               detail::max_alignment);

                auto storage = ::new (mem) detail::joint_storage<T>(additional_size);
                FOONATHAN_TRY
                {
                    ::new (storage->get_object_storage()) T(detail::forward<Args>(args)...);
                }
                FOONATHAN_CATCH_ALL
                {
                    storage->~joint_storage();
                    this->deallocate_node(mem, sizeof(detail::joint_storage<T>) + additional_size,
                                          detail::max_alignment);
                    FOONATHAN_RETHROW;
                }
                storage_ = storage;

                FOONATHAN_MEMORY_ASSERT(storage_->capacity() == additional_size);
            }

            detail::joint_storage<T>* storage_;

            friend class joint_allocator;
        };

        /// @{
        /// \returns `!ptr`,
        /// i.e. if `ptr` does not own anything.
        /// \relates joint_ptr
        template <typename T, class RawAllocator, class Mutex>
        bool operator==(const joint_ptr<T, RawAllocator, Mutex>& ptr, std::nullptr_t)
        {
            return !ptr;
        }

        template <typename T, class RawAllocator, class Mutex>
        bool operator==(std::nullptr_t, const joint_ptr<T, RawAllocator, Mutex>& ptr)
        {
            return ptr == nullptr;
        }
        /// @}

        /// @{
        /// \returns `ptr.get() == p`,
        /// i.e. if `ptr` ownws the object referred to by `p`.
        /// \relates joint_ptr
        template <typename T, class RawAllocator, class Mutex>
        bool operator==(const joint_ptr<T, RawAllocator, Mutex>& ptr, T* p)
        {
            return ptr.get() == p;
        }

        template <typename T, class RawAllocator, class Mutex>
        bool operator==(T* p, const joint_ptr<T, RawAllocator, Mutex>& ptr)
        {
            return ptr == p;
        }
        /// @}

        /// @{
        /// \returns `!(ptr == nullptr)`,
        /// i.e. if `ptr` does own something.
        /// \relates joint_ptr
        template <typename T, class RawAllocator, class Mutex>
        bool operator!=(const joint_ptr<T, RawAllocator, Mutex>& ptr, std::nullptr_t)
        {
            return !(ptr == nullptr);
        }

        template <typename T, class RawAllocator, class Mutex>
        bool operator!=(std::nullptr_t, const joint_ptr<T, RawAllocator, Mutex>& ptr)
        {
            return ptr != nullptr;
        }
        /// @}

        /// @{
        /// \returns `!(ptr == p)`,
        /// i.e. if `ptr` does not ownw the object referred to by `p`.
        /// \relates joint_ptr
        template <typename T, class RawAllocator, class Mutex>
        bool operator!=(const joint_ptr<T, RawAllocator, Mutex>& ptr, T* p)
        {
            return !(ptr == p);
        }

        template <typename T, class RawAllocator, class Mutex>
        bool operator!=(T* p, const joint_ptr<T, RawAllocator, Mutex>& ptr)
        {
            return ptr != p;
        }
        /// @}

        /// @{
        /// \returns A new \ref joint_ptr as if created with the same arguments passed to the constructor.
        /// \relatesalso joint_ptr
        /// \ingroup memory allocator
        template <typename T, class RawAllocator, typename... Args>
        auto allocate_joint(RawAllocator& alloc, std::size_t additional_size, Args&&... args)
            -> joint_ptr<T, RawAllocator>
        {
            return joint_ptr<T, RawAllocator>(alloc, additional_size,
                                              detail::forward<Args>(args)...);
        }

        template <typename T, class RawAllocator, typename... Args>
        auto allocate_joint(const RawAllocator& alloc, std::size_t additional_size, Args&&... args)
            -> joint_ptr<T, RawAllocator>
        {
            return joint_ptr<T, RawAllocator>(alloc, additional_size,
                                              detail::forward<Args>(args)...);
        }
        /// @}

        /// Tag type to make creation of a type with joint memory more explicit.
        ///
        /// It is used in the constructors of \ref joint_allocator and \ref joint_array.
        /// \ingroup memory allocator
        template <typename T>
        class joint_type
        {
        public:
            /// \effects Creates it from an object with joint memory.
            /// \requires `obj` must be an object managed by \ref joint_ptr,
            /// you cannot pass arbitrary objects to it.
            explicit joint_type(T& obj) FOONATHAN_NOEXCEPT : ptr_(&obj)
            {
            }

            /// \returns A reference to the object with the joint memory.
            T& get() const FOONATHAN_NOEXCEPT
            {
                return *ptr_;
            }

        private:
            T* ptr_;
        };

        /// \returns The \ref joint_type from `T`.
        /// \relates joint_type
        template <typename T>
        joint_type<T> joint(T& t) FOONATHAN_NOEXCEPT
        {
            return joint_type<T>(t);
        }

        template <typename T>
        joint_type<T> joint(const T&) = delete;

        /// @{
        /// \returns The \ref joint_type from the object owned by `ptr`.
        /// \requires `ptr` must not be null.
        /// \relates joint_type
        template <typename T, class RawAllocator, class Mutex>
        joint_type<T> joint(joint_ptr<T, RawAllocator, Mutex>& ptr) FOONATHAN_NOEXCEPT
        {
            return joint_type<T>(*ptr);
        }

        template <typename T, class RawAllocator, class Mutex>
        joint_type<T> joint(const joint_ptr<T, RawAllocator, Mutex>& ptr) FOONATHAN_NOEXCEPT
        {
            return joint_type<T>(*ptr);
        }
        /// @}

        namespace detail
        {
            /// converts object to control block it is contained in
            template <typename T>
            joint_storage<T>& get_storage(joint_type<T> obj) FOONATHAN_NOEXCEPT
            {
                // strict aliasing rules permit converting pointer to first member of standard layout type
                // to standard layout type itself
                static_assert(std::is_standard_layout<joint_storage<T>>::value,
                              "must be standard layout");
                // 1) convert obj to char*, this is always allowed
                auto obj_ptr = reinterpret_cast<char*>(&obj.get());
                // 2) convert pointer to first member of joint_storage::data_t to joint_storage
                // allowed because rule above
                auto joinable_ptr = reinterpret_cast<joint_storage<T>*>(obj_ptr);
                FOONATHAN_MEMORY_ASSERT_MSG(joinable_ptr->is_valid(obj.get()),
                                            "joint_storage not "
                                            "valid, object might not be joint?");
                return *joinable_ptr;
            }
        } // namespace detail

        /// A \concept{concept_rawallocator,RawAllocator} that uses the additional joint memory for its allocation.
        ///
        /// It is somewhat limited and allows only allocation once.
        /// All joint allocators for an object share the joint memory and must not be used in multiple threads.
        /// The memory it returns is owned by a \ref joint_ptr and will be destroyed through it.
        /// \ingroup memory allocator
        class joint_allocator
        {
        public:
            /// \effects Creates it using the joint memory of the given object.
            template <typename T>
            joint_allocator(joint_type<T> j) FOONATHAN_NOEXCEPT
                : stack_(&detail::get_storage(j).stack())
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
                if (!stack_)
                    FOONATHAN_THROW(out_of_fixed_memory(info(), size));
                auto mem = stack_->allocate(size, alignment);
                if (!mem)
                    FOONATHAN_THROW(out_of_fixed_memory(info(), size));
                stack_ = nullptr;
                return mem;
            }

            /// \effects None, deallocation does nothing and is only done when the \ref joint_ptr destroys the memory.
            void deallocate_node(void*, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
            {
            }

        private:
            allocator_info info() const FOONATHAN_NOEXCEPT
            {
                return allocator_info(FOONATHAN_MEMORY_LOG_PREFIX "::joint_allocator", this);
            }

            detail::joinable_stack* stack_;
        };

        /// Specialization of \ref is_shared_allocator to mark \ref joint_allocator as shared.
        /// This allows using it as \ref allocator_reference directly.
        /// \ingroup memory allocator
        template <>
        struct is_shared_allocator<joint_allocator> : std::true_type
        {
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
            joint_array(std::size_t size, joint_type<JointType> j)
            : joint_array(detail::get_storage(j).stack(), size)
            {
            }

            /// \effects Creates with `size` copies of `val`  using the specified joint memory.
            /// \throws \ref out_of_fixed_memory if `size` is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename JointType>
            joint_array(std::size_t size, const value_type& val, joint_type<JointType> j)
            : joint_array(detail::get_storage(j).stack(), size, val)
            {
            }

            /// \effects Creates with the copies of the objects in the initializer list using the specified joint memory.
            /// \throws \ref out_of_fixed_memory if the size is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename JointType>
            joint_array(std::initializer_list<value_type> ilist, joint_type<JointType> j)
            : joint_array(detail::get_storage(j).stack(), ilist)
            {
            }

            /// \effects Creates it by forwarding each element of the range to `T`s constructor  using the specified joint memory.
            /// \throws \ref out_of_fixed_memory if the size is too big
            /// and anything thrown by `T`s constructor.
            /// If an allocation is thrown, the memory will be released directly.
            template <typename InIter, typename JointType,
                      typename = decltype(*std::declval<InIter&>()++)>
            joint_array(InIter begin, InIter end, joint_type<JointType> j)
            : joint_array(detail::get_storage(j).stack(), begin, end)
            {
            }

            /// \effects Destroys all objects,
            /// but does not release the storage.
            ~joint_array() FOONATHAN_NOEXCEPT
            {
                for (std::size_t i = 0u; i != size_; ++i)
                    ptr_[i].~T();
            }

            joint_array(joint_array&&) = delete;
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
            joint_array(allocate_only, detail::joinable_stack& stack, std::size_t size)
            : ptr_(nullptr), size_(0u)
            {
                ptr_ = static_cast<T*>(stack.allocate(size * sizeof(T), FOONATHAN_ALIGNOF(T)));
                if (!ptr_)
                    FOONATHAN_THROW(out_of_fixed_memory(info(), size * sizeof(T)));
            }

            class builder
            {
            public:
                builder(detail::joinable_stack& stack, T* ptr) FOONATHAN_NOEXCEPT : stack_(&stack),
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
                detail::joinable_stack* stack_;
                T*                      objects_;
                std::size_t             size_;
            };

            joint_array(detail::joinable_stack& stack, std::size_t size)
            : joint_array(allocate_only{}, stack, size)
            {
                builder b(stack, ptr_);
                for (auto i = 0u; i != size; ++i)
                    b.create();
                size_ = b.release();
            }

            joint_array(detail::joinable_stack& stack, std::size_t size, const value_type& value)
            : joint_array(allocate_only{}, stack, size)
            {
                builder b(stack, ptr_);
                for (auto i = 0u; i != size; ++i)
                    b.create(value);
                size_ = b.release();
            }

            joint_array(detail::joinable_stack& stack, std::initializer_list<value_type> ilist)
            : joint_array(allocate_only{}, stack, ilist.size())
            {
                builder b(stack, ptr_);
                for (auto& elem : ilist)
                    b.create(elem);
                size_ = b.release();
            }

            template <typename InIter>
            joint_array(detail::joinable_stack& stack, InIter begin, InIter end)
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
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_JOINT_ALLOCATOR_HPP_INCLUDED
