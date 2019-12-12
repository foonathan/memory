// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DELETER_HPP_INCLUDED
#define FOONATHAN_MEMORY_DELETER_HPP_INCLUDED

/// \file
/// \c Deleter classes using a \concept{concept_rawallocator,RawAllocator}.

#include <type_traits>

#include "allocator_storage.hpp"
#include "config.hpp"
#include "threading.hpp"

namespace foonathan
{
    namespace memory
    {
        /// A deleter class that deallocates the memory through a specified \concept{concept_rawallocator,RawAllocator}.
        ///
        /// It deallocates memory for a specified type but does not call its destructors.
        /// \ingroup memory adapter
        template <typename Type, class RawAllocator>
        class allocator_deallocator : FOONATHAN_EBO(allocator_reference<RawAllocator>)
        {
            static_assert(!std::is_abstract<Type>::value,
                          "use allocator_polymorphic_deallocator for storing base classes");

        public:
            using allocator_type = typename allocator_reference<RawAllocator>::allocator_type;
            using value_type     = Type;

            /// \effects Creates it without any associated allocator.
            /// The deallocator must not be used if that is the case.
            /// \notes This functions is useful if you have want to create an empty smart pointer without giving it an allocator.
            allocator_deallocator() FOONATHAN_NOEXCEPT = default;

            /// \effects Creates it by passing it an \ref allocator_reference.
            /// It will store the reference to it and uses the referenced allocator object for the deallocation.
            allocator_deallocator(allocator_reference<RawAllocator> alloc) FOONATHAN_NOEXCEPT
            : allocator_reference<RawAllocator>(alloc)
            {
            }

            /// \effects Deallocates the memory given to it.
            /// Calls \c deallocate_node(pointer, sizeof(value_type), alignof(value_type)) on the referenced allocator object.
            /// \requires The deallocator must not have been created by the default constructor.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                this->deallocate_node(pointer, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            /// \requires The deallocator must not be created by the default constructor.
            auto get_allocator() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<allocator_reference<allocator_type>>().get_allocator())
            {
                return this->allocator_reference<allocator_type>::get_allocator();
            }
        };

        /// Specialization of \ref allocator_deallocator for array types.
        /// Otherwise the same behavior.
        /// \ingroup memory adapter
        template <typename Type, class RawAllocator>
        class allocator_deallocator<Type[], RawAllocator>
        : FOONATHAN_EBO(allocator_reference<RawAllocator>)
        {
            static_assert(!std::is_abstract<Type>::value, "must not create polymorphic arrays");

        public:
            using allocator_type = typename allocator_reference<RawAllocator>::allocator_type;
            using value_type     = Type;

            /// \effects Creates it without any associated allocator.
            /// The deallocator must not be used if that is the case.
            /// \notes This functions is useful if you have want to create an empty smart pointer without giving it an allocator.
            allocator_deallocator() FOONATHAN_NOEXCEPT : size_(0u) {}

            /// \effects Creates it by passing it an \ref allocator_reference and the size of the array that will be deallocated.
            /// It will store the reference to the allocator and uses the referenced allocator object for the deallocation.
            allocator_deallocator(allocator_reference<RawAllocator> alloc,
                                  std::size_t                       size) FOONATHAN_NOEXCEPT
            : allocator_reference<RawAllocator>(alloc),
              size_(size)
            {
            }

            /// \effects Deallocates the memory given to it.
            /// Calls \c deallocate_array(pointer, size, sizeof(value_type), alignof(value_type))
            /// on the referenced allocator object with the size given in the constructor.
            /// \requires The deallocator must not have been created by the default constructor.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                this->deallocate_array(pointer, size_, sizeof(value_type),
                                       FOONATHAN_ALIGNOF(value_type));
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            /// \requires The deallocator must not have been created by the default constructor.
            auto get_allocator() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<allocator_reference<allocator_type>>().get_allocator())
            {
                return this->allocator_reference<allocator_type>::get_allocator();
            }

            /// \returns The size of the array that will be deallocated.
            /// This is the same value as passed in the constructor, or `0` if it was created by the default constructor.
            std::size_t array_size() const FOONATHAN_NOEXCEPT
            {
                return size_;
            }

        private:
            std::size_t size_;
        };

        /// A deleter class that deallocates the memory of a derived type through a specified \concept{concept_rawallocator,RawAllocator}.
        ///
        /// It can only be created from a \ref allocator_deallocator and thus must only be used for smart pointers initialized by derived-to-base conversion of the pointer.
        /// \ingroup memory adapter
        template <typename BaseType, class RawAllocator>
        class allocator_polymorphic_deallocator : FOONATHAN_EBO(allocator_reference<RawAllocator>)
        {
        public:
            using allocator_type = typename allocator_reference<RawAllocator>::allocator_type;
            using value_type     = BaseType;

            /// \effects Creates it from a deallocator for a derived type.
            /// It will deallocate the memory as if done by the derived type.
            template <typename T, FOONATHAN_REQUIRES((std::is_base_of<BaseType, T>::value))>
            allocator_polymorphic_deallocator(allocator_deallocator<T, RawAllocator> dealloc)
            : allocator_reference<RawAllocator>(dealloc.get_allocator()),
              derived_size_(sizeof(T)),
              derived_alignment_(FOONATHAN_ALIGNOF(T))
            {
            }

            /// \effects Deallocates the memory given to it.
            /// Calls \c deallocate_node(pointer, size, alignment) on the referenced allocator object,
            /// where \c size and \c alignment are the values of the type it was created with.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                this->deallocate_node(pointer, derived_size_, derived_alignment_);
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            auto get_allocator() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<allocator_reference<allocator_type>>().get_allocator())
            {
                return this->allocator_reference<allocator_type>::get_allocator();
            }

        private:
            std::size_t derived_size_, derived_alignment_;
        };

        /// Similar to \ref allocator_deallocator but calls the destructors of the object.
        /// Otherwise behaves the same.
        /// \ingroup memory adapter
        template <typename Type, class RawAllocator>
        class allocator_deleter : FOONATHAN_EBO(allocator_reference<RawAllocator>)
        {
            static_assert(!std::is_abstract<Type>::value,
                          "use allocator_polymorphic_deleter for storing base classes");

        public:
            using allocator_type = typename allocator_reference<RawAllocator>::allocator_type;
            using value_type     = Type;

            /// \effects Creates it without any associated allocator.
            /// The deleter must not be used if that is the case.
            /// \notes This functions is useful if you have want to create an empty smart pointer without giving it an allocator.
            allocator_deleter() FOONATHAN_NOEXCEPT = default;

            /// \effects Creates it by passing it an \ref allocator_reference.
            /// It will store the reference to it and uses the referenced allocator object for the deallocation.
            allocator_deleter(allocator_reference<RawAllocator> alloc) FOONATHAN_NOEXCEPT
            : allocator_reference<RawAllocator>(alloc)
            {
            }

            /// \effects Calls the destructor and deallocates the memory given to it.
            /// Calls \c deallocate_node(pointer, sizeof(value_type), alignof(value_type))
            /// on the referenced allocator object for the deallocation.
            /// \requires The deleter must not have been created by the default constructor.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                pointer->~value_type();
                this->deallocate_node(pointer, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            auto get_allocator() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<allocator_reference<allocator_type>>().get_allocator())
            {
                return this->allocator_reference<allocator_type>::get_allocator();
            }
        };

        /// Specialization of \ref allocator_deleter for array types.
        /// Otherwise the same behavior.
        /// \ingroup memory adapter
        template <typename Type, class RawAllocator>
        class allocator_deleter<Type[], RawAllocator>
        : FOONATHAN_EBO(allocator_reference<RawAllocator>)
        {
            static_assert(!std::is_abstract<Type>::value, "must not create polymorphic arrays");

        public:
            using allocator_type = typename allocator_reference<RawAllocator>::allocator_type;
            using value_type     = Type;

            /// \effects Creates it without any associated allocator.
            /// The deleter must not be used if that is the case.
            /// \notes This functions is useful if you have want to create an empty smart pointer without giving it an allocator.
            allocator_deleter() FOONATHAN_NOEXCEPT : size_(0u) {}

            /// \effects Creates it by passing it an \ref allocator_reference and the size of the array that will be deallocated.
            /// It will store the reference to the allocator and uses the referenced allocator object for the deallocation.
            allocator_deleter(allocator_reference<RawAllocator> alloc,
                              std::size_t                       size) FOONATHAN_NOEXCEPT
            : allocator_reference<RawAllocator>(alloc),
              size_(size)
            {
            }

            /// \effects Calls the destructors and deallocates the memory given to it.
            /// Calls \c deallocate_array(pointer, size, sizeof(value_type), alignof(value_type))
            /// on the referenced allocator object with the size given in the constructor for the deallocation.
            /// \requires The deleter must not have been created by the default constructor.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                for (auto cur = pointer; cur != pointer + size_; ++cur)
                    cur->~value_type();
                this->deallocate_array(pointer, size_, sizeof(value_type),
                                       FOONATHAN_ALIGNOF(value_type));
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            /// \requires The deleter must not be created by the default constructor.
            auto get_allocator() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<allocator_reference<allocator_type>>().get_allocator())
            {
                return this->allocator_reference<allocator_type>::get_allocator();
            }

            /// \returns The size of the array that will be deallocated.
            /// This is the same value as passed in the constructor, or `0` if it was created by the default constructor.
            std::size_t array_size() const FOONATHAN_NOEXCEPT
            {
                return size_;
            }

        private:
            std::size_t size_;
        };

        /// Similar to \ref allocator_polymorphic_deallocator but calls the destructors of the object.
        /// Otherwise behaves the same.
        /// \note It has a relatively high space overhead, so only use it if you have to.
        /// \ingroup memory adapter
        template <typename BaseType, class RawAllocator>
        class allocator_polymorphic_deleter : FOONATHAN_EBO(allocator_reference<RawAllocator>)
        {
        public:
            using allocator_type = typename allocator_reference<RawAllocator>::allocator_type;
            using value_type     = BaseType;

            /// \effects Creates it from a deleter for a derived type.
            /// It will deallocate the memory as if done by the derived type.
            template <typename T, FOONATHAN_REQUIRES((std::is_base_of<BaseType, T>::value))>
            allocator_polymorphic_deleter(allocator_deleter<T, RawAllocator> deleter)
            : allocator_reference<RawAllocator>(deleter.get_allocator()),
              derived_size_(sizeof(T)),
              derived_alignment_(FOONATHAN_ALIGNOF(T))
            {
                FOONATHAN_MEMORY_ASSERT(std::size_t(derived_size_) == sizeof(T)
                                        && std::size_t(derived_alignment_) == FOONATHAN_ALIGNOF(T));
            }

            /// \effects Deallocates the memory given to it.
            /// Calls \c deallocate_node(pointer, size, alignment) on the referenced allocator object,
            /// where \c size and \c alignment are the values of the type it was created with.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                pointer->~value_type();
                this->deallocate_node(pointer, derived_size_, derived_alignment_);
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            auto get_allocator() const FOONATHAN_NOEXCEPT
                -> decltype(std::declval<allocator_reference<allocator_type>>().get_allocator())
            {
                return this->allocator_reference<allocator_type>::get_allocator();
            }

        private:
            unsigned short derived_size_,
                derived_alignment_; // use unsigned short here to save space
        };
    } // namespace memory
} // namespace foonathan

#endif //FOONATHAN_MEMORY_DELETER_HPP_INCLUDED
