// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DELETER_HPP_INCLUDED
#define FOONATHAN_MEMORY_DELETER_HPP_INCLUDED

/// \file
/// \c Deleter classes using a \concept{concept_rawallocator,RawAllocator}.

#include "allocator_storage.hpp"
#include "config.hpp"
#include "threading.hpp"

namespace foonathan
{
    namespace memory
    {
        /// A deleter class that deallocates the memory through a specified \concept{concept_rawallocator,RawAllocator}.
        /// It deallocates memory for a specified type but does not call its destructors.
        /// Only a reference to the \c RawAllocator is stored, access to it is synchronized by a given \c Mutex which defaults to \ref default_mutex.
        /// \ingroup memory adapter
        template <typename Type, class RawAllocator, class Mutex = default_mutex>
        class allocator_deallocator : FOONATHAN_EBO(allocator_reference<RawAllocator, Mutex>)
        {
        public:
            using allocator_type =
                typename allocator_reference<RawAllocator, Mutex>::allocator_type;
            using mutex      = Mutex;
            using value_type = Type;

            /// \effects Creates it by passing it an \ref allocator_reference.
            /// It will store the reference to it and uses the referenced allocator object for the deallocation.
            allocator_deallocator(allocator_reference<RawAllocator, mutex> alloc) FOONATHAN_NOEXCEPT
                : allocator_reference<RawAllocator, Mutex>(alloc)
            {
            }

            /// \effects Deallocates the memory given to it.
            /// Calls \c deallocate_node(pointer, sizeof(value_type), alignof(value_type)) on the referenced allocator object.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                this->deallocate_node(pointer, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            auto get_allocator() const FOONATHAN_NOEXCEPT -> decltype(
                std::declval<allocator_reference<allocator_type, mutex>>().get_allocator())
            {
                return this->allocator_reference<allocator_type, mutex>::get_allocator();
            }
        };

        /// Specialization of \ref allocator_deallocator for array types.
        /// Otherwise the same behavior.
        /// \ingroup memory adapter
        template <typename Type, class RawAllocator, class Mutex>
        class allocator_deallocator<Type[], RawAllocator, Mutex>
            : FOONATHAN_EBO(allocator_reference<RawAllocator, Mutex>)
        {
        public:
            using allocator_type =
                typename allocator_reference<RawAllocator, Mutex>::allocator_type;
            using mutex      = Mutex;
            using value_type = Type;

            /// \effects Creates it by passing it an \ref allocator_reference and the size of the array that will be deallocated.
            /// It will store the reference to the allocator and uses the referenced allocator object for the deallocation.
            allocator_deallocator(allocator_reference<RawAllocator, mutex> alloc,
                                  std::size_t size) FOONATHAN_NOEXCEPT
                : allocator_reference<RawAllocator, Mutex>(alloc),
                  size_(size)
            {
            }

            /// \effects Deallocates the memory given to it.
            /// Calls \c deallocate_array(pointer, size, sizeof(value_type), alignof(value_type))
            /// on the referenced allocator object with the size given in the constructor.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                this->deallocate_array(pointer, size_, sizeof(value_type),
                                       FOONATHAN_ALIGNOF(value_type));
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            auto get_allocator() const FOONATHAN_NOEXCEPT -> decltype(
                std::declval<allocator_reference<allocator_type, mutex>>().get_allocator())
            {
                return this->allocator_reference<allocator_type, mutex>::get_allocator();
            }

            /// \returns The size of the array that will be deallocated.
            /// This is the same value as passed in the constructor.
            std::size_t array_size() const FOONATHAN_NOEXCEPT
            {
                return size_;
            }

        private:
            std::size_t size_;
        };

        /// Similar to \ref allocator_deallocator but calls the destructors of the objects.
        /// Otherwise behaves the same.
        /// \ingroup memory adapter
        template <typename Type, class RawAllocator, class Mutex = default_mutex>
        class allocator_deleter : FOONATHAN_EBO(allocator_reference<RawAllocator, Mutex>)
        {
        public:
            using allocator_type =
                typename allocator_reference<RawAllocator, Mutex>::allocator_type;
            using mutex      = Mutex;
            using value_type = Type;

            /// \effects Creates it by passing it an \ref allocator_reference.
            /// It will store the reference to it and uses the referenced allocator object for the deallocation.
            allocator_deleter(allocator_reference<RawAllocator, mutex> alloc) FOONATHAN_NOEXCEPT
                : allocator_reference<RawAllocator, Mutex>(alloc)
            {
            }

            /// \effects Calls the destructor and deallocates the memory given to it.
            /// Calls \c deallocate_node(pointer, sizeof(value_type), alignof(value_type))
            /// on the referenced allocator object for the deallocation.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                pointer->~value_type();
                this->deallocate_node(pointer, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            auto get_allocator() const FOONATHAN_NOEXCEPT -> decltype(
                std::declval<allocator_reference<allocator_type, mutex>>().get_allocator())
            {
                return this->allocator_reference<allocator_type, mutex>::get_allocator();
            }
        };

        /// Specialization of \ref allocator_deleter for array types.
        /// Otherwise the same behavior.
        /// \ingroup memory adapter
        template <typename Type, class RawAllocator, class Mutex>
        class allocator_deleter<Type[], RawAllocator, Mutex>
            : FOONATHAN_EBO(allocator_reference<RawAllocator, Mutex>)
        {
        public:
            using allocator_type =
                typename allocator_reference<RawAllocator, Mutex>::allocator_type;
            using mutex      = Mutex;
            using value_type = Type;

            /// \effects Creates it by passing it an \ref allocator_reference and the size of the array that will be deallocated.
            /// It will store the reference to the allocator and uses the referenced allocator object for the deallocation.
            allocator_deleter(allocator_reference<RawAllocator, mutex> alloc,
                              std::size_t size) FOONATHAN_NOEXCEPT
                : allocator_reference<RawAllocator, Mutex>(alloc),
                  size_(size)
            {
            }

            /// \effects Calls the destructors and deallocates the memory given to it.
            /// Calls \c deallocate_array(pointer, size, sizeof(value_type), alignof(value_type))
            /// on the referenced allocator object with the size given in the constructor for the deallocation.
            void operator()(value_type* pointer) FOONATHAN_NOEXCEPT
            {
                for (auto cur = pointer; cur != pointer + size_; ++cur)
                    cur->~value_type();
                this->deallocate_array(pointer, size_, sizeof(value_type),
                                       FOONATHAN_ALIGNOF(value_type));
            }

            /// \returns The reference to the allocator.
            /// It has the same type as the call to \ref allocator_reference::get_allocator().
            auto get_allocator() const FOONATHAN_NOEXCEPT -> decltype(
                std::declval<allocator_reference<allocator_type, mutex>>().get_allocator())
            {
                return this->allocator_reference<allocator_type, mutex>::get_allocator();
            }

            /// \returns The size of the array that will be deallocated.
            /// This is the same value as passed in the constructor.
            std::size_t array_size() const FOONATHAN_NOEXCEPT
            {
                return size_;
            }

        private:
            std::size_t size_;
        };
    }
} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_DELETER_HPP_INCLUDED
