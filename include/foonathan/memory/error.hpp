// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// \brief Error handling routines.

#ifndef FOONATHAN_MEMORY_ERROR_HPP_INCLUDED
#define FOONATHAN_MEMORY_ERROR_HPP_INCLUDED

#include <cstddef>
#include <new>

#include "config.hpp"

namespace foonathan { namespace memory
{
    /// \brief Information about an allocator.
    /// \details It consists of a name and a pointer to it.<br>
    /// It will be used for logging in error handling.
    /// \ingroup memory
    struct allocator_info
    {
        /// \brief The name of the allocator.
        /// \details Must be a NTBS whose lifetime outlives this object (i.e. a string literal).
        const char *name;

        /// \brief A pointer to the allocator.
        /// \details This may point to a subobject of the allocator,
        /// so it must not be casted to an actual allocator.
        /// It's only purpose is for identifying different allocator objects.<br>
        /// Depending on the context, it might be \c null to represent stateless allocators.
        const void *allocator;

        /// \brief Creates a new allocator info.
        FOONATHAN_CONSTEXPR allocator_info(const char *name,
                                           const void *allocator) FOONATHAN_NOEXCEPT
        : name(name), allocator(allocator) {}

        /// @{
        /// \brief Two allocator info objects are equal if the object pointer is the same.
        friend FOONATHAN_CONSTEXPR
            bool operator==(const allocator_info &a,
                            const allocator_info &b) FOONATHAN_NOEXCEPT
        {
            return a.allocator == b.allocator;
        }

        friend FOONATHAN_CONSTEXPR
        bool operator!=(const allocator_info &a,
                        const allocator_info &b) FOONATHAN_NOEXCEPT
        {
            return a.allocator != b.allocator;
        }
        /// @}
    };

    /// \brief The exception class thrown in case of out of memory condition.
    /// \details This happens when a low level allocation function (e.g. \c std::malloc)
    /// runs out of memory.
    /// It will be thrown when the handler returns.
    /// \ingroup memory
    class out_of_memory : public std::bad_alloc
    {
    public:
        /// \brief The out of memory handler.
        /// \details It will be called prior to throwing the exception.
        /// It can log the error, throw another exception or abort the program.
        /// If it returns, this exception will be thrown.<br>
        /// It gets the \ref allocator_info and the amount of memory last tried to be allocated.<br>
        /// The default handler writes this information to \c stderr and continues execution,
        /// leading to the exception,
        /// unless it is a freestanding implementation where it does nothing.
        /// \note Unlike \c std::new_handler, this function does not get called in a loop or similar,
        /// it will be called only once and is only meant for error reporting.
        using handler = void(*)(const allocator_info &info, std::size_t amount);

        /// \brief Exchanges the \ref handler.
        /// \details This function is thread safe.
        static handler set_handler(handler h);

        /// \brief Returns the \ref handler.
        static handler get_handler();

        /// \brief Creates a new exception object.
        /// \details This also calls the \ref handler.
        /// This might lead to a different exception actually thrown by the program.
        out_of_memory(const allocator_info &info, std::size_t amount);

        /// \brief Returns a message describing the error.
        /// \details It is a static string,
        /// since it cannot do any formatting (it lacks the memory for it).
        const char* what() const FOONATHAN_NOEXCEPT override;

        /// \brief Returns the \ref allocator_info.
        const allocator_info& allocator() const FOONATHAN_NOEXCEPT
        {
            return info_;
        }

        /// \brief Returns the amount of memory that was tried to be allocated.
        std::size_t failed_allocation_size() const FOONATHAN_NOEXCEPT
        {
            return amount_;
        }

    private:
        allocator_info info_;
        std::size_t amount_;
    };

    /// \brief The exception class thrown if an allocation size/alignment exceeds the supported maximum.
    /// \details This applies to the node size (\c max_node_size()),
    /// the array size (\c max_array_size()) or the alignment (\c max_alignment()).<br>
    /// It will be thrown when the handler returns.
    /// \ingroup memory
    class bad_allocation_size : public std::bad_alloc
    {
    public:
        /// \brief The bad allocation handler.
        /// \details It will be called prior to throwing the exception.
        /// It can log the error, throw another exception or abort the program.
        /// If it returns, this exception will be thrown.<br>
        /// It gets the \ref allocator_info,
        /// the size/alignment passed to it and the size/alignment supported maximum.<br>
        /// The default handler writes this information to \c stderr and continues execution,
        /// leading to the exception,
        /// unless it is a freestanding implementation where it does nothing.
        using handler = void(*)(const allocator_info &info,
                                std::size_t passed, std::size_t supported);

        /// \brief Exchanges the \ref handler.
        /// \details This function is thread safe.
        static handler set_handler(handler h);

        /// \brief Returns the \ref handler.
        static handler get_handler();

        /// \brief Creates a new exception object.
        /// \details This also calls the \ref handler.
        /// This might lead to a different exception actually thrown by the program.
        bad_allocation_size(const allocator_info &info,
                            std::size_t passed, std::size_t supported);

        /// \brief Returns a message describing the error.
        /// \details It is a static string,
        /// since it cannot do any formatting.
        const char* what() const FOONATHAN_NOEXCEPT override;

        /// \brief Returns the \ref allocator_info.
        const allocator_info& allocator() const FOONATHAN_NOEXCEPT
        {
            return info_;
        }

        /// @{
        /// \brief Returns the passed/support size/alignment.
        std::size_t passed_value() const FOONATHAN_NOEXCEPT
        {
            return passed_;
        }

        std::size_t supported_value() const FOONATHAN_NOEXCEPT
        {
            return supported_;
        }
        /// @}

    private:
        allocator_info info_;
        std::size_t passed_, supported_;
    };

    namespace detail
    {
        // compatibility implementation for std::get_new_handler()
        std::new_handler get_new_handler();

        // tries to allocate memory by calling the function in a loop
        // if the function returns a non-null pointer, it is returned
        // otherwise the std::new_handler is called, if it exits and the loop continued
        // if it doesn't exist, the out_of_memory_handler is called and std::bad_alloc thrown afterwards
        void* try_allocate(void* (*alloc_func)(std::size_t size), std::size_t size,
                            const allocator_info& info);

        // checks for a valid size
        inline void check_allocation_size(std::size_t passed, std::size_t supported,
                                   const allocator_info &info)
        {
            if (passed > supported)
                FOONATHAN_THROW(bad_allocation_size(info, passed, supported));
        }

        // handles a failed assertion
        void handle_failed_assert(const char *msg, const char *file, int line, const char *fnc) FOONATHAN_NOEXCEPT;

    // note: debug assertion macros don't use fully qualified name
    // because they should only be used in this library, where the whole namespace is available
    // can be override via command line definitions
    #if FOONATHAN_MEMORY_DEBUG_ASSERT && !defined(FOONATHAN_MEMORY_ASSERT)
        #define FOONATHAN_MEMORY_ASSERT(Expr) \
            static_cast<void>((Expr) || (detail::handle_failed_assert("Assertion \"" #Expr "\" failed", \
                                                                      __FILE__, __LINE__, __func__), true))

        #define FOONATHAN_MEMORY_ASSERT_MSG(Expr, Msg) \
            static_cast<void>((Expr) || (detail::handle_failed_assert("Assertion \"" #Expr "\" failed: " Msg, \
                                                                      __FILE__, __LINE__, __func__), true))

        #define FOONATHAN_MEMORY_UNREACHABLE(Msg) \
            detail::handle_failed_assert("Unreachable code reached: " Msg, __FILE__,  __LINE__, __func__)
    #elif !defined(FOONATHAN_MEMORY_ASSERT)
        #define FOONATHAN_MEMORY_ASSERT(Expr) static_cast<void>(Expr)
        #define FOONATHAN_MEMORY_ASSERT_MSG(Expr, Msg) static_cast<void>(Expr)
        #define FOONATHAN_MEMORY_UNREACHABLE(Msg) /* nothing */
    #endif
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ERROR_HPP_INCLUDED
