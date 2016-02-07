// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_ERROR_HELPERS_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_ERROR_HELPERS_HPP_INCLUDED

#include "../config.hpp"

namespace foonathan { namespace memory
{
    struct allocator_info;

    namespace detail
    {
        // checks if passed <= supported and throws bad_allocation_size if needed
        // useful because it does not require including error.hpp
        void check_allocation_size(std::size_t passed, std::size_t supported,
                                   const allocator_info &info);

        // handles an out of memory condition by throwing the exception
        // useful because it does not require including error.hpp
        void handle_out_of_memory(const allocator_info &info, std::size_t amount);

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

#endif // FOONATHAN_MEMORY_DETAIL_ERROR_HELPERS_HPP_INCLUDED
