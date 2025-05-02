// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

/// \file
/// Configuration macros.

#ifndef FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED
#define FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED

#include <cstddef>

#if !defined(DOXYGEN)
#define FOONATHAN_MEMORY_IMPL_IN_CONFIG_HPP
#include "config_impl.hpp"
#undef FOONATHAN_MEMORY_IMPL_IN_CONFIG_HPP
#endif

// exception support
#ifndef FOONATHAN_HAS_EXCEPTION_SUPPORT
#if defined(__GNUC__) && !defined(__EXCEPTIONS)
#define FOONATHAN_HAS_EXCEPTION_SUPPORT 0
#elif defined(_MSC_VER) && !_HAS_EXCEPTIONS
#define FOONATHAN_HAS_EXCEPTION_SUPPORT 0
#else
#define FOONATHAN_HAS_EXCEPTION_SUPPORT 1
#endif
#endif

#if FOONATHAN_HAS_EXCEPTION_SUPPORT
#define FOONATHAN_THROW(Ex) throw(Ex)
#else
#include <cstdlib>
#define FOONATHAN_THROW(Ex) ((Ex), std::abort())
#endif

// hosted implementation
#ifndef FOONATHAN_HOSTED_IMPLEMENTATION
#if !defined(_MSC_VER) && !defined(__STDC_HOSTED__)
#define FOONATHAN_HOSTED_IMPLEMENTATION 0
#else
#define FOONATHAN_HOSTED_IMPLEMENTATION 1
#endif
#endif

// log prefix
#define FOONATHAN_MEMORY_LOG_PREFIX "foonathan::memory"

// version
#define FOONATHAN_MEMORY_VERSION                                                                   \
    (FOONATHAN_MEMORY_VERSION_MAJOR * 100 + FOONATHAN_MEMORY_VERSION_MINOR)

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
#define FOONATHAN_ALIAS_TEMPLATE(Name, ...)                                                        \
    class Name : public __VA_ARGS__                                                                \
    {                                                                                              \
    }
#else
#define FOONATHAN_ALIAS_TEMPLATE(Name, ...) using Name = __VA_ARGS__
#endif
#endif

#ifdef DOXYGEN
// dummy definitions of config macros for doxygen

/// The major version number.
/// \ingroup core
#define FOONATHAN_MEMORY_VERSION_MAJOR 1

/// The minor version number.
/// \ingroup core
#define FOONATHAN_MEMORY_VERSION_MINOR 1

/// The total version number of the form \c Mmm.
/// \ingroup core
#define FOONATHAN_MEMORY_VERSION                                                                   \
    (FOONATHAN_MEMORY_VERSION_MAJOR * 100 + FOONATHAN_MEMORY_VERSION_MINOR)

/// Whether or not the allocation size will be checked,
/// i.e. the \ref foonathan::memory::bad_allocation_size thrown.
/// \ingroup core
#define FOONATHAN_MEMORY_CHECK_ALLOCATION_SIZE 1

/// Whether or not internal assertions in the library are enabled.
/// \ingroup core
#define FOONATHAN_MEMORY_DEBUG_ASSERT 1

/// Whether or not allocated memory will be filled with special values.
/// \ingroup core
#define FOONATHAN_MEMORY_DEBUG_FILL 1

/// The size of the fence memory, it has no effect if \ref FOONATHAN_MEMORY_DEBUG_FILL is \c false.
/// \note For most allocators, the actual value doesn't matter and they use appropriate defaults to ensure alignment etc.
/// \ingroup core
#define FOONATHAN_MEMORY_DEBUG_FENCE 1

/// Whether or not leak checking is enabled.
/// \ingroup core
#define FOONATHAN_MEMORY_DEBUG_LEAK_CHECK 1

/// Whether or not the deallocation functions will check for pointers that were never allocated by an allocator.
/// \ingroup core
#define FOONATHAN_MEMORY_DEBUG_POINTER_CHECK 1

/// Whether or not the deallocation functions will check for double free errors.
/// This option makes no sense if \ref FOONATHAN_MEMORY_DEBUG_POINTER_CHECK is \c false.
/// \ingroup core
#define FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK 1

/// Whether or not everything is in namespace <tt>foonathan::memory</tt>.
/// If \c false, a namespace alias <tt>namespace memory = foonathan::memory</tt> is automatically inserted into each header,
/// allowing to qualify everything with <tt>foonathan::</tt>.
/// \note This option breaks in combination with using <tt>using namespace foonathan;</tt>.
/// \ingroup core
#define FOONATHAN_MEMORY_NAMESPACE_PREFIX 1

/// The mode of the automatic \ref foonathan::memory::temporary_stack creation.
/// Set to `2` to enable automatic lifetime management of the per-thread stack through nifty counter.
/// Then all memory will be freed upon program termination automatically.
/// Set to `1` to disable automatic lifetime managment of the per-thread stack,
/// requires managing it through the \ref foonathan::memory::temporary_stack_initializer.
/// Set to `0` to disable the per-thread stack completely.
/// \ref foonathan::memory::get_temporary_stack() will abort the program upon call.
/// \ingroup allocator
#define FOONATHAN_MEMORY_TEMPORARY_STACK_MODE 2
#endif

#endif // FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED
