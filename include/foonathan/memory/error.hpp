// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// The exception classes.

#ifndef FOONATHAN_MEMORY_ERROR_HPP_INCLUDED
#define FOONATHAN_MEMORY_ERROR_HPP_INCLUDED

#include <cstddef>
#include <new>

#include "config.hpp"

namespace foonathan
{
    namespace memory
    {
        /// Contains information about an allocator.
        /// It can be used for logging in the various handler functions.
        /// \ingroup core
        struct allocator_info
        {
            /// The name of the allocator.
            /// It is a NTBS whose lifetime is not managed by this object,
            /// it must be stored elsewhere or be a string literal.
            const char* name;

            /// A pointer representing an allocator.
            /// It does not necessarily point to the beginning of the allocator object,
            /// the only guarantee is that different allocator objects result in a different pointer value.
            /// For stateless allocators it is sometimes \c nullptr.
            /// \note The pointer must not be cast back to any allocator type.
            const void* allocator;

            /// \effects Creates it by giving it the name of the allocator and a pointer.
            constexpr allocator_info(const char* n, const void* alloc) noexcept
            : name(n), allocator(alloc)
            {
            }

            /// @{
            /// \effects Compares two \ref allocator_info objects, they are equal, if the \ref allocator is the same.
            /// \returns The result of the comparision.
            friend constexpr bool operator==(const allocator_info& a,
                                             const allocator_info& b) noexcept
            {
                return a.allocator == b.allocator;
            }

            friend constexpr bool operator!=(const allocator_info& a,
                                             const allocator_info& b) noexcept
            {
                return a.allocator != b.allocator;
            }
            /// @}
        };

        /// The exception class thrown when a low level allocator runs out of memory.
        /// It is derived from \c std::bad_alloc.
        /// This can happen if a low level allocation function like \c std::malloc() runs out of memory.
        /// Throwing can be prohibited by the handler function.
        /// \ingroup core
        class out_of_memory : public std::bad_alloc
        {
        public:
            /// The type of the handler called in the constructor of \ref out_of_memory.
            /// When an out of memory situation is encountered and the exception class created,
            /// this handler gets called.
            /// It is especially useful if exception support is disabled.
            /// It gets the \ref allocator_info and the amount of memory that was tried to be allocated.
            /// \requiredbe It can log the error, throw a different exception derived from \c std::bad_alloc or abort the program.
            /// If it returns, this exception object will be created and thrown.
            /// \defaultbe On a hosted implementation it logs the error on \c stderr and continues execution,
            /// leading to this exception being thrown.
            /// On a freestanding implementation it does nothing.
            /// \note It is different from \c std::new_handler; it will not be called in a loop trying to allocate memory
            /// or something like that. Its only job is to report the error.
            using handler = void (*)(const allocator_info& info, std::size_t amount);

            /// \effects Sets \c h as the new \ref handler in an atomic operation.
            /// A \c nullptr sets the default \ref handler.
            /// \returns The previous \ref handler. This is never \c nullptr.
            static handler set_handler(handler h);

            /// \returns The current \ref handler. This is never \c nullptr.
            static handler get_handler();

            /// \effects Creates it by passing it the \ref allocator_info and the amount of memory failed to be allocated.
            /// It also calls the \ref handler to control whether or not it will be thrown.
            out_of_memory(const allocator_info& info, std::size_t amount);

            /// \returns A static NTBS that describes the error.
            /// It does not contain any specific information since there is no memory for formatting.
            const char* what() const noexcept override;

            /// \returns The \ref allocator_info passed to it in the constructor.
            const allocator_info& allocator() const noexcept
            {
                return info_;
            }

            /// \returns The amount of memory that was tried to be allocated.
            /// This is the value passed in the constructor.
            std::size_t failed_allocation_size() const noexcept
            {
                return amount_;
            }

        private:
            allocator_info info_;
            std::size_t    amount_;
        };

        /// A special case of \ref out_of_memory errors
        /// thrown when a low-level allocator with a fixed size runs out of memory.
        /// For example, thrown by \ref fixed_block_allocator or \ref static_allocator.<br>
        /// It is derived from \ref out_of_memory but does not provide its own handler.
        /// \ingroup core
        class out_of_fixed_memory : public out_of_memory
        {
        public:
            /// \effects Just forwards to \ref out_of_memory.
            out_of_fixed_memory(const allocator_info& info, std::size_t amount)
            : out_of_memory(info, amount)
            {
            }

            /// \returns A static NTBS that describes the error.
            /// It does not contain any specific information since there is no memory for formatting.
            const char* what() const noexcept override;
        };

        /// The exception class thrown when an allocation size is bigger than the supported maximum.
        /// This size is either the node, array or alignment parameter in a call to an allocation function.
        /// If those exceed the supported maximum returned by \c max_node_size(), \c max_array_size() or \c max_alignment(),
        /// one of its derived classes will be thrown or this class if in a situation where the type is unknown.
        /// It is derived from \c std::bad_alloc.
        /// Throwing can be prohibited by the handler function.
        /// \note Even if all parameters are less than the maximum, \ref out_of_memory or a similar exception can be thrown,
        /// because the maximum functions return an upper bound and not the actual supported maximum size,
        /// since it always depends on fence memory, alignment buffer and the like.
        /// \note A user should only \c catch for \c bad_allocation_size, not the derived classes.
        /// \note Most checks will only be done if \ref FOONATHAN_MEMORY_CHECK_ALLOCATION_SIZE is \c true.
        /// \ingroup core
        class bad_allocation_size : public std::bad_alloc
        {
        public:
            /// The type of the handler called in the constructor of \ref bad_allocation_size.
            /// When a bad allocation size is detected and the exception object created,
            /// this handler gets called.
            /// It is especially useful if exception support is disabled.
            /// It gets the \ref allocator_info, the size passed to the function and the supported size
            /// (the latter is still an upper bound).
            /// \requiredbe It can log the error, throw a different exception derived from \c std::bad_alloc or abort the program.
            /// If it returns, this exception object will be created and thrown.
            /// \defaultbe On a hosted implementation it logs the error on \c stderr and continues execution,
            /// leading to this exception being thrown.
            /// On a freestanding implementation it does nothing.
            using handler = void (*)(const allocator_info& info, std::size_t passed,
                                     std::size_t supported);

            /// \effects Sets \c h as the new \ref handler in an atomic operation.
            /// A \c nullptr sets the default \ref handler.
            /// \returns The previous \ref handler. This is never \c nullptr.
            static handler set_handler(handler h);

            /// \returns The current \ref handler. This is never \c nullptr.
            static handler get_handler();

            /// \effects Creates it by passing it the \ref allocator_info, the size passed to the allocation function
            /// and an upper bound on the supported size.
            /// It also calls the \ref handler to control whether or not it will be thrown.
            bad_allocation_size(const allocator_info& info, std::size_t passed,
                                std::size_t supported);

            /// \returns A static NTBS that describes the error.
            /// It does not contain any specific information since there is no memory for formatting.
            const char* what() const noexcept override;

            /// \returns The \ref allocator_info passed to it in the constructor.
            const allocator_info& allocator() const noexcept
            {
                return info_;
            }

            /// \returns The size or alignment value that was passed to the allocation function
            /// which was too big. This is the same value passed to the constructor.
            std::size_t passed_value() const noexcept
            {
                return passed_;
            }

            /// \returns An upper bound on the maximum supported size/alignment.
            /// It is only an upper bound, values below can fail, but values above will always fail.
            std::size_t supported_value() const noexcept
            {
                return supported_;
            }

        private:
            allocator_info info_;
            std::size_t    passed_, supported_;
        };

        /// The exception class thrown when the node size exceeds the supported maximum,
        /// i.e. it is bigger than \c max_node_size().
        /// It is derived from \ref bad_allocation_size but does not override the handler.
        /// \ingroup core
        class bad_node_size : public bad_allocation_size
        {
        public:
            /// \effects Just forwards to \ref bad_allocation_size.
            bad_node_size(const allocator_info& info, std::size_t passed, std::size_t supported)
            : bad_allocation_size(info, passed, supported)
            {
            }

            /// \returns A static NTBS that describes the error.
            /// It does not contain any specific information since there is no memory for formatting.
            const char* what() const noexcept override;
        };

        /// The exception class thrown when the array size exceeds the supported maximum,
        /// i.e. it is bigger than \c max_array_size().
        /// It is derived from \ref bad_allocation_size but does not override the handler.
        /// \ingroup core
        class bad_array_size : public bad_allocation_size
        {
        public:
            /// \effects Just forwards to \ref bad_allocation_size.
            bad_array_size(const allocator_info& info, std::size_t passed, std::size_t supported)
            : bad_allocation_size(info, passed, supported)
            {
            }

            /// \returns A static NTBS that describes the error.
            /// It does not contain any specific information since there is no memory for formatting.
            const char* what() const noexcept override;
        };

        /// The exception class thrown when the alignment exceeds the supported maximum,
        /// i.e. it is bigger than \c max_alignment().
        /// It is derived from \ref bad_allocation_size but does not override the handler.
        /// \ingroup core
        class bad_alignment : public bad_allocation_size
        {
        public:
            /// \effects Just forwards to \ref bad_allocation_size.
            /// \c passed is <tt>count * size</tt>, \c supported the size in bytes.
            bad_alignment(const allocator_info& info, std::size_t passed, std::size_t supported)
            : bad_allocation_size(info, passed, supported)
            {
            }

            /// \returns A static NTBS that describes the error.
            /// It does not contain any specific information since there is no memory for formatting.
            const char* what() const noexcept override;
        };

        namespace detail
        {
            template <class Ex, typename Func>
            void check_allocation_size(std::size_t passed, Func f, const allocator_info& info)
            {
#if FOONATHAN_MEMORY_CHECK_ALLOCATION_SIZE
                auto supported = f();
                if (passed > supported)
                    FOONATHAN_THROW(Ex(info, passed, supported));
#else
                (void)passed;
                (void)f;
                (void)info;
#endif
            }

            template <class Ex>
            void check_allocation_size(std::size_t passed, std::size_t supported,
                                       const allocator_info& info)
            {
                check_allocation_size<Ex>(
                    passed, [&] { return supported; }, info);
            }
        } // namespace detail
    }     // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_ERROR_HPP_INCLUDED
