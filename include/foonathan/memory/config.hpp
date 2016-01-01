// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// Configuration macros.

/// \defgroup memory memory

/// \namespace foonathan
/// Foonathan namespace.

/// \namespace foonathan::memory
/// Memory namespace.

#ifndef FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED
#define FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED

#if !defined(DOXYGEN)
    #define FOONATHAN_MEMORY_IMPL_IN_CONFIG_HPP
    #include "config_impl.hpp"
    #undef FOONATHAN_MEMORY_IMPL_IN_CONFIG_HPP
#endif

#if !defined(DOXYGEN)
    #define COMP_IN_PARENT_HEADER
    #include "comp/alignas.hpp"
    #include "comp/alignof.hpp"
    #include "comp/constexpr.hpp"
    #include "comp/noexcept.hpp"
    #include "comp/thread_local.hpp"

    #include "comp/get_new_handler.hpp"
    #include "comp/max_align_t.hpp"

    #include "comp/exception_support.hpp"
    #include "comp/hosted_implementation.hpp"
    #include "comp/threading_support.hpp"
    #undef COMP_IN_PARENT_HEADER
#endif

// namespace prefix
#if !FOONATHAN_MEMORY_NAMESPACE_PREFIX
    namespace foonathan { namespace memory {}}
    namespace memory = foonathan::memory;

    #define FOONATHAN_MEMORY_LOG_PREFIX "memory"
#else
    #define FOONATHAN_MEMORY_LOG_PREFIX "foonathan::memory"
#endif

// use this macro to mark implementation-defined types
// gives it more semantics and useful with doxygen
// add PREDEFINED: FOONATHAN_IMPL_DEFINED():=implementation_defined
#ifndef FOONATHAN_IMPL_DEFINED
    #define FOONATHAN_IMPL_DEFINED(...) __VA_ARGS__
#endif

// use this macro to mark base class which only purpose is EBO
// gives it more semantics and useful with doxygen
// add PREDEFINED: FOONATHAN_EBO():=
#ifndef FOONATHAN_EBO
    #define FOONATHAN_EBO(...) __VA_ARGS__
#endif

#ifndef FOONATHAN_ALIAS_TEMPLATE
    // defines a template alias
    // usage:
    // template <typename T>
    // FOONATHAN_ALIAS_TEMPLATE(bar, foo<T, int>);
    // useful for doxygen
    #ifdef DOXYGEN
        #define FOONATHAN_ALIAS_TEMPLATE(Name, ...) \
            class Name : public __VA_ARGS__ {}
    #else
        #define FOONATHAN_ALIAS_TEMPLATE(Name, ...) \
            using Name = __VA_ARGS__
    #endif
#endif

#ifdef DOXYGEN
    // dummy definitions of config macros for doxygen

    /// The major version number.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_VERSION_MAJOR 1

    /// The minor version number.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_VERSION_MINOR 1

    /// The total version number of the form \c Mmm.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_VERSION (FOONATHAN_MEMORY_VERSION_MAJOR * 100 + FOONATHAN_MEMORY_VERSION_MINOR)

    /// Whether or not the \ref foonathan::memory::default_mutex will be \c std::mutex or \ref foonathan::memory::no_mutex.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_THREAD_SAFE_REFERENCE 1

    /// Whether or not internal assertions in the library are enabled.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_DEBUG_ASSERT 1

    /// Whether or not allocated memory will be filled with special values.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_DEBUG_FILL 1

    /// The size of the fence memory, it has no effect if \ref FOONATHAN_MEMORY_DEBUG_FILL is \c false.
    /// \note For most allocators, the actual value doesn't matter and they use appropriate defaults to ensure alignment etc.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_DEBUG_FENCE 1

    /// Whether or not leak checking is enabled.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_DEBUG_LEAK_CHECK 1

    /// Whether or not the deallocation functions will check for pointers that were never allocated by an allocator.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_DEBUG_POINTER_CHECK 1

    /// Whether or not the deallocation functions will check for double free errors.
    /// This option makes no sense if \ref FOONATHAN_MEMORY_DEBUG_POINTER_CHECK is not \c true.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK 1

    /// Whether or not everything is in namespace <tt>foonathan::memory</tt>.
    /// If \c false, a namespace alias <tt>namespace memory = foonathan::memory</tt> is automatically inserted into each header,
    /// allowing to qualify everything with <tt>foonathan::</tt>.
    /// \note This option breaks in combination with using <tt>using namespace foonathan;</tt>.
    /// \ingroup memory
    #define FOONATHAN_MEMORY_NAMESPACE_PREFIX 1
#endif

#endif // FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED
