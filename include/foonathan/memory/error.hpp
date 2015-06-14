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
    namespace detail
    {
        // compatibility implementation for std::get_new_handler()
        std::new_handler get_new_handler();

        // tries to allocate memory by calling the function in a loop
        // if the function returns a non-null pointer, it is returned
        // otherwise the std::new_handler is called, if it exits and the loop continued
        // if it doesn't exist, the out_of_memory_handler is called and std::bad_alloc thrown afterwards
        void* try_allocate(void* (*alloc_func)(std::size_t), std::size_t size,
                            const char *name, void *allocator);
    } // namespace detail

    /// \brief The out of memory handler.
    /// \details It will be called when a low level allocation function runs out of memory.
    /// It gets a descriptive string of the allocator, a pointer to the allocator instance (\c nullptr for stateless)
    /// and the amount of memory that has been tried to allocate.<br>
    /// If it returns, an exception of type \c std::bad_alloc will be thrown.<br>
    /// The default handler writes the information to \c stderr and continues execution,
    /// leading to to the exception.
    /// \note Unlike \c std::new_handler, this function does not get called in a loop or similar,
    /// it will be called only once and is only meant for error reporting.
    /// \ingroup memory
    using out_of_memory_handler = void(*)(const char *name, void *allocator, std::size_t amount);

    /// \brief Exchanges the \ref out_of_memory_handler.
    /// \details This function is thread safe.
    /// \ingroup memory
    out_of_memory_handler set_out_of_memory_handler(out_of_memory_handler h);

    /// \brief Returns the \ref out_of_memory_handler.
    /// \ingroup memory
    out_of_memory_handler get_out_of_memory_handler();
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ERROR_HPP_INCLUDED
