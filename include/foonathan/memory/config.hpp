// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// dummy header including the real config file from the CMake binary dir

#ifndef FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED
#define FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED

#define FOONATHAN_MEMORY_IMPL_IN_CONFIG_HPP
#include "config_impl.hpp"
#undef FOONATHAN_MEMORY_IMPL_IN_CONFIG_HPP

#define COMP_IN_PARENT_HEADER
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

// namespace prefix
#if !FOONATHAN_MEMORY_NAMESPACE_PREFIX
    namespace foonathan { namespace memory {}}
    namespace memory = foonathan::memory;

    #define FOONATHAN_MEMORY_IMPL_LOG_PREFIX "memory"
#else
    #define FOONATHAN_MEMORY_IMPL_LOG_PREFIX "foonathan::memory"
#endif

#endif // FOONATHAN_MEMORY_CONFIG_HPP_INCLUDED
