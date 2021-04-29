// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_ASSERT_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_ASSERT_HPP_INCLUDED

#include <cstdlib>

#include "../config.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            // handles a failed assertion
            void handle_failed_assert(const char* msg, const char* file, int line,
                                      const char* fnc) noexcept;

            void handle_warning(const char* msg, const char* file, int line,
                                const char* fnc) noexcept;

// note: debug assertion macros don't use fully qualified name
// because they should only be used in this library, where the whole namespace is available
// can be override via command line definitions
#if FOONATHAN_MEMORY_DEBUG_ASSERT && !defined(FOONATHAN_MEMORY_ASSERT)
#define FOONATHAN_MEMORY_ASSERT(Expr)                                                              \
    static_cast<void>((Expr)                                                                       \
                      || (detail::handle_failed_assert("Assertion \"" #Expr "\" failed", __FILE__, \
                                                       __LINE__, __func__),                        \
                          true))

#define FOONATHAN_MEMORY_ASSERT_MSG(Expr, Msg)                                                     \
    static_cast<void>((Expr)                                                                       \
                      || (detail::handle_failed_assert("Assertion \"" #Expr "\" failed: " Msg,     \
                                                       __FILE__, __LINE__, __func__),              \
                          true))

#define FOONATHAN_MEMORY_UNREACHABLE(Msg)                                                          \
    detail::handle_failed_assert("Unreachable code reached: " Msg, __FILE__, __LINE__, __func__)

#define FOONATHAN_MEMORY_WARNING(Msg) detail::handle_warning(Msg, __FILE__, __LINE__, __func__)

#elif !defined(FOONATHAN_MEMORY_ASSERT)
#define FOONATHAN_MEMORY_ASSERT(Expr)
#define FOONATHAN_MEMORY_ASSERT_MSG(Expr, Msg)
#define FOONATHAN_MEMORY_UNREACHABLE(Msg) std::abort()
#define FOONATHAN_MEMORY_WARNING(Msg)
#endif
        } // namespace detail
    }     // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_DETAIL_ASSERT_HPP_INCLUDED

