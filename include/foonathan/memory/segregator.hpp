// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_SEGREGATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_SEGREGATOR_HPP_INCLUDED

/// \file
/// Class template \ref foonathan::memory::segregator and related classes.

#include "detail/ebo_storage.hpp"
#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "config.hpp"
#include "error.hpp"

namespace foonathan
{
    namespace memory
    {
        /// A \concept{concept_segregatable,Segregatable} that allocates until a maximum size.
        /// \ingroup memory adapter
        template <class RawAllocator>
        class threshold_segregatable : FOONATHAN_EBO(allocator_traits<RawAllocator>::allocator_type)
        {
        public:
            using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;

            /// \effects Creates it by passing the maximum size it will allocate
            /// and the allocator it uses.
            explicit threshold_segregatable(std::size_t    max_size,
                                            allocator_type alloc = allocator_type())
            : allocator_type(detail::move(alloc)), max_size_(max_size)
            {
            }

            /// \returns `true` if `size` is less then or equal to the maximum size,
            /// `false` otherwise.
            /// \note A return value of `true` means that the allocator will be used for the allocation.
            bool use_allocate_node(std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
            {
                return size <= max_size_;
            }

            /// \returns `true` if `count * size` is less then or equal to the maximum size,
            /// `false` otherwise.
            /// \note A return value of `true` means that the allocator will be used for the allocation.
            bool use_allocate_array(std::size_t count, std::size_t size,
                                    std::size_t) FOONATHAN_NOEXCEPT
            {
                return count * size <= max_size_;
            }

            /// @{
            /// \returns A reference to the allocator it owns.
            allocator_type& get_allocator() FOONATHAN_NOEXCEPT
            {
                return *this;
            }

            const allocator_type& get_allocator() const FOONATHAN_NOEXCEPT
            {
                return *this;
            }
            /// @}

        private:
            std::size_t max_size_;
        };

        /// \returns A \ref threshold_segregatable with the same parameter.
        template <class RawAllocator>
        threshold_segregatable<typename std::decay<RawAllocator>::type> threshold(
            std::size_t max_size, RawAllocator&& alloc)
        {
            return threshold_segregatable<
                typename std::decay<RawAllocator>::type>(max_size,
                                                         std::forward<RawAllocator>(alloc));
        }

        /// A composable \concept{concept_rawallocator,RawAllocator} that will always fail.
        /// This is useful for compositioning or as last resort in \ref binary_segregator.
        /// \ingroup memory allocator
        class null_allocator
        {
        public:
            /// \effects Will always throw.
            /// \throws A \ref out_of_fixed_memory exception.
            void* allocate_node(std::size_t size, std::size_t)
            {
                throw out_of_fixed_memory(info(), size);
            }

            /// \requires Must not be called.
            void deallocate_node(void*, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
            {
                FOONATHAN_MEMORY_UNREACHABLE("cannot be called with proper values");
            }

            /// \effects Does nothing.
            /// \returns Always returns `nullptr`.
            void* try_allocate_node(std::size_t, std::size_t) FOONATHAN_NOEXCEPT
            {
                return nullptr;
            }

            /// \effects Does nothing.
            /// \returns Always returns `false`.
            bool try_deallocate_node(void*, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
            {
                return false;
            }

        private:
            allocator_info info() const FOONATHAN_NOEXCEPT
            {
                return {FOONATHAN_MEMORY_LOG_PREFIX "::null_allocator", this};
            }
        };

        /// A \concept{concept_rawallocator,RawAllocator} that either uses the \concept{concept_segregatable,Segregatable} or the other `RawAllocator`.
        /// It is a faster alternative to \ref fallback_allocator that doesn't require a composable allocator
        /// and decides about the allocator to use purely with the `Segregatable` based on size and alignment.
        /// \ingroup memory adapter
        template <class Segregatable, class RawAllocator>
        class binary_segregator
        : FOONATHAN_EBO(
              detail::ebo_storage<1, typename allocator_traits<RawAllocator>::allocator_type>)
        {
            using segregatable_traits = allocator_traits<typename Segregatable::allocator_type>;
            using fallback_traits     = allocator_traits<RawAllocator>;

        public:
            using segregatable                = Segregatable;
            using segregatable_allocator_type = typename segregatable::allocator_type;
            using fallback_allocator_type = typename allocator_traits<RawAllocator>::allocator_type;

            /// \effects Creates it by giving the \concept{concept_segregatable,Segregatable}
            /// and the \concept{concept_rawallocator,RawAllocator}.
            explicit binary_segregator(segregatable            s,
                                       fallback_allocator_type fallback = fallback_allocator_type())
            : detail::ebo_storage<1, fallback_allocator_type>(detail::move(fallback)),
              s_(detail::move(s))
            {
            }

            /// @{
            /// \effects Uses the \concept{concept_segregatable,Segregatable} to decide which allocator to use.
            /// Then forwards to the chosen allocator.
            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                if (get_segregatable().use_allocate_node(size, alignment))
                    return segregatable_traits::allocate_node(get_segregatable_allocator(), size,
                                                              alignment);
                else
                    return fallback_traits::allocate_node(get_fallback_allocator(), size,
                                                          alignment);
            }

            void deallocate_node(void* ptr, std::size_t size,
                                 std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (get_segregatable().use_allocate_node(size, alignment))
                    segregatable_traits::deallocate_node(get_segregatable_allocator(), ptr, size,
                                                         alignment);
                else
                    fallback_traits::deallocate_node(get_fallback_allocator(), ptr, size,
                                                     alignment);
            }

            void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
            {
                if (get_segregatable().use_allocate_array(count, size, alignment))
                    return segregatable_traits::allocate_array(get_segregatable_allocator(), count,
                                                               size, alignment);
                else
                    return fallback_traits::allocate_array(get_fallback_allocator(), count, size,
                                                           alignment);
            }

            void deallocate_array(void* array, std::size_t count, std::size_t size,
                                  std::size_t alignment) FOONATHAN_NOEXCEPT
            {
                if (get_segregatable().use_allocate_array(count, size, alignment))
                    segregatable_traits::deallocate_array(get_segregatable_allocator(), array,
                                                          count, size, alignment);
                else
                    fallback_traits::deallocate_array(get_fallback_allocator(), array, count, size,
                                                      alignment);
            }
            /// @}

            /// @{
            /// \returns The maximum value of the fallback.
            /// \note It assumes that the fallback will be used for larger allocations,
            /// and the `Segregatable` for smaller ones.
            std::size_t max_node_size() const
            {
                return fallback_traits::max_node_size(get_fallback_allocator());
            }

            std::size_t max_array_size() const
            {
                return fallback_traits::max_array_size(get_fallback_allocator());
            }

            std::size_t max_alignemnt() const
            {
                return fallback_traits::max_alignment(get_fallback_allocator());
            }
            /// @}

            /// @{
            /// \returns A reference to the segregatable allocator.
            /// This is the one primarily used.
            segregatable_allocator_type& get_segregatable_allocator() FOONATHAN_NOEXCEPT
            {
                return get_segregatable().get_allocator();
            }

            const segregatable_allocator_type& get_segregatable_allocator() const FOONATHAN_NOEXCEPT
            {
                return get_segregatable().get_allocator();
            }
            /// @}

            /// @{
            /// \returns A reference to the fallback allocator.
            /// It will be used if the \concept{concept_segregator,Segregator} doesn't want the alloction.
            fallback_allocator_type& get_fallback_allocator() FOONATHAN_NOEXCEPT
            {
                return detail::ebo_storage<1, fallback_allocator_type>::get();
            }

            const fallback_allocator_type& get_fallback_allocator() const FOONATHAN_NOEXCEPT
            {
                return detail::ebo_storage<1, fallback_allocator_type>::get();
            }
            /// @}

        private:
            segregatable& get_segregatable() FOONATHAN_NOEXCEPT
            {
                return s_;
            }

            segregatable s_;
        };

        namespace detail
        {
            template <class... Segregatables>
            struct make_segregator_t;

            template <class Segregatable>
            struct make_segregator_t<Segregatable>
            {
                using type = binary_segregator<Segregatable, null_allocator>;
            };

            template <class Segregatable, class RawAllocator>
            struct make_segregator_t<Segregatable, RawAllocator>
            {
                using type = binary_segregator<Segregatable, RawAllocator>;
            };

            template <class Segregatable, class... Tail>
            struct make_segregator_t<Segregatable, Tail...>
            {
                using type =
                    binary_segregator<Segregatable, typename make_segregator_t<Tail...>::type>;
            };

            template <class Segregator, class Fallback = null_allocator>
            auto make_segregator(Segregator&& seg, Fallback&& f = null_allocator{})
                -> binary_segregator<typename std::decay<Segregator>::type,
                                     typename std::decay<Fallback>::type>
            {
                return binary_segregator<
                    typename std::decay<Segregator>::type,
                    typename std::decay<Fallback>::type>(std::forward<Segregator>(seg),
                                                         std::forward<Fallback>(f));
            }

            template <class Segregator, typename... Rest>
            auto make_segregator(Segregator&& seg, Rest&&... rest)
                -> binary_segregator<typename std::decay<Segregator>::type,
                                     decltype(make_segregator(std::forward<Rest>(rest)...))>
            {
                return binary_segregator<typename std::decay<Segregator>::type,
                                         decltype(make_segregator(std::forward<Rest>(
                                             rest)...))>(std::forward<Segregator>(seg),
                                                         make_segregator(
                                                             std::forward<Rest>(rest)...));
            }

            template <std::size_t I, class Segregator>
            struct segregatable_type;

            template <class Segregator, class Fallback>
            struct segregatable_type<0, binary_segregator<Segregator, Fallback>>
            {
                using type = typename Segregator::allocator_type;

                static type& get(binary_segregator<Segregator, Fallback>& s)
                {
                    return s.get_segregatable_allocator();
                }

                static const type& get(const binary_segregator<Segregator, Fallback>& s)
                {
                    return s.get_segregatable_allocator();
                }
            };

            template <std::size_t I, class Segregator, class Fallback>
            struct segregatable_type<I, binary_segregator<Segregator, Fallback>>
            {
                using base = segregatable_type<I - 1, Fallback>;
                using type = typename base::type;

                static type& get(binary_segregator<Segregator, Fallback>& s)
                {
                    return base::get(s.get_fallback_allocator());
                }

                static const type& get(const binary_segregator<Segregator, Fallback>& s)
                {
                    return base::get(s.get_fallback_allocator());
                }
            };

            template <class Fallback>
            struct fallback_type
            {
                using type = Fallback;

                static const std::size_t size = 0u;

                static type& get(Fallback& f)
                {
                    return f;
                }

                static const type& get(const Fallback& f)
                {
                    return f;
                }
            };

            template <class Segregator, class Fallback>
            struct fallback_type<binary_segregator<Segregator, Fallback>>
            {
                using base = fallback_type<Fallback>;
                using type = typename base::type;

                static const std::size_t size = base::size + 1u;

                static type& get(binary_segregator<Segregator, Fallback>& s)
                {
                    return base::get(s.get_fallback_allocator());
                }

                static const type& get(const binary_segregator<Segregator, Fallback>& s)
                {
                    return base::get(s.get_fallback_allocator());
                }
            };
        } // namespace detail

        /// Creates multiple nested \ref binary_segreagator.
        /// If you pass one type, it must be a \concept{concept_segregatable,Segregatable}.
        /// Then the result is a \ref binary_segregator with that `Segregatable` and \ref null_allocator as fallback.
        /// If you pass two types, the first one must be a `Segregatable`,
        /// the second one a \concept{concept_rawallocator,RawAllocator}.
        /// Then the result is a simple \ref binary_segregator with those arguments.
        /// If you pass more than one, the last one must be a `RawAllocator` all others `Segregatable`,
        /// the result is `binary_segregator<Head, segregator<Tail...>>`.
        /// \note It will result in an allocator that tries each `Segregatable` in the order specified
        /// using the last parameter as final fallback.
        /// \ingroup memory adapter
        template <class... Allocators>
        FOONATHAN_ALIAS_TEMPLATE(segregator,
                                 typename detail::make_segregator_t<Allocators...>::type);

        /// \returns A \ref segregator created from the allocators `args`.
        /// \relates segregator
        template <typename... Args>
        auto make_segregator(Args&&... args) -> segregator<typename std::decay<Args>::type...>
        {
            return detail::make_segregator(std::forward<Args>(args)...);
        }

        /// The number of \concept{concept_segregatable,Segregatable} a \ref segregator has.
        /// \relates segregator
        template <class Segregator>
        struct segregator_size
        {
            static const std::size_t value = detail::fallback_type<Segregator>::size;
        };

        /// The type of the `I`th \concept{concept_segregatable,Segregatable}.
        /// \relates segregator
        template <std::size_t I, class Segregator>
        using segregatable_allocator_type = typename detail::segregatable_type<I, Segregator>::type;

        /// @{
        /// \returns The `I`th \concept{concept_segregatable,Segregatable}.
        /// \relates segregrator
        template <std::size_t I, class Segregator, class Fallback>
        auto get_segregatable_allocator(binary_segregator<Segregator, Fallback>& s)
            -> segregatable_allocator_type<I, binary_segregator<Segregator, Fallback>>&
        {
            return detail::segregatable_type<I, binary_segregator<Segregator, Fallback>>::get(s);
        }

        template <std::size_t I, class Segregator, class Fallback>
        auto get_segregatable_allocator(const binary_segregator<Segregator, Fallback>& s)
            -> const segregatable_allocator_type<I, binary_segregator<Segregator, Fallback>>
        {
            return detail::segregatable_type<I, binary_segregator<Segregator, Fallback>>::get(s);
        }
        /// @}

        /// The type of the final fallback \concept{concept_rawallocator,RawAllocator].
        /// \relates segregator
        template <class Segregator>
        using fallback_allocator_type = typename detail::fallback_type<Segregator>::type;

        /// @{
        /// \returns The final fallback \concept{concept_rawallocator,RawAllocator].
        /// \relates segregator
        template <class Segregator, class Fallback>
        auto get_fallback_allocator(binary_segregator<Segregator, Fallback>& s)
            -> fallback_allocator_type<binary_segregator<Segregator, Fallback>>&
        {
            return detail::fallback_type<binary_segregator<Segregator, Fallback>>::get(s);
        }

        template <class Segregator, class Fallback>
        auto get_fallback_allocator(const binary_segregator<Segregator, Fallback>& s)
            -> const fallback_allocator_type<binary_segregator<Segregator, Fallback>>&
        {
            return detail::fallback_type<binary_segregator<Segregator, Fallback>>::get(s);
        }
        /// @}
    }
} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_SEGREGATOR_HPP_INCLUDED
