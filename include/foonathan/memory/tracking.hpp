// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
#define FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::tracked_allocator and related classes and functions.

#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "memory_arena.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            template <class Allocator, class Tracker>
            auto set_tracker(int, Allocator& allocator, Tracker* tracker) noexcept
                -> decltype(allocator.get_allocator().set_tracker(tracker))
            {
                return allocator.get_allocator().set_tracker(tracker);
            }
            template <class Allocator, class Tracker>
            void set_tracker(short, Allocator&, Tracker*) noexcept
            {
            }

            // used with deeply_tracked_allocator
            template <class Tracker, class BlockAllocator>
            class deeply_tracked_block_allocator : FOONATHAN_EBO(BlockAllocator)
            {
            public:
                template <typename... Args>
                deeply_tracked_block_allocator(std::size_t block_size, Args&&... args)
                : BlockAllocator(block_size, detail::forward<Args>(args)...), tracker_(nullptr)
                {
                }

                memory_block allocate_block()
                {
                    auto block = BlockAllocator::allocate_block();
                    if (tracker_) // on first call tracker_ is nullptr
                        tracker_->on_allocator_growth(block.memory, block.size);
                    return block;
                }

                void deallocate_block(memory_block block) noexcept
                {
                    if (tracker_) // on last call tracker_ is nullptr again
                        tracker_->on_allocator_shrinking(block.memory, block.size);
                    BlockAllocator::deallocate_block(block);
                }

                std::size_t next_block_size() const noexcept
                {
                    return BlockAllocator::next_block_size();
                }

                void set_tracker(Tracker* tracker) noexcept
                {
                    tracker_ = tracker;
                }

            private:
                Tracker* tracker_;
            };
        } // namespace detail

        /// A \concept{concept_blockallocator,BlockAllocator} adapter that tracks another allocator using a \concept{concept_tracker,tracker}.
        /// It wraps another \concept{concept_blockallocator,BlockAllocator} and calls the tracker function before forwarding to it.
        /// The class can then be used anywhere a \concept{concept_blockallocator,BlockAllocator} is required and the memory usage will be tracked.<br>
        /// It will only call the <tt>on_allocator_growth()</tt> and <tt>on_allocator_shrinking()</tt> tracking functions,
        /// since a \concept{concept_blockallocator,BlockAllocator} is normally used inside higher allocators only.
        /// \ingroup adapter
        template <class Tracker, class BlockOrRawAllocator>
        class tracked_block_allocator
        : FOONATHAN_EBO(Tracker, make_block_allocator_t<BlockOrRawAllocator>)
        {
        public:
            using allocator_type = make_block_allocator_t<BlockOrRawAllocator>;
            using tracker        = Tracker;

            /// @{
            /// \effects Creates it by giving it a \concept{concept_tracker,tracker} and the tracked \concept{concept_rawallocator,RawAllocator}.
            /// It will embed both objects.
            explicit tracked_block_allocator(tracker t = {}) noexcept : tracker(detail::move(t)) {}

            tracked_block_allocator(tracker t, allocator_type&& alloc) noexcept
            : tracker(detail::move(t)), allocator_type(detail::move(alloc))
            {
            }
            /// @}

            /// \effects Creates it in the form required by the concept.
            /// The allocator will be constructed using \c block_size and \c args.
            template <typename... Args>
            tracked_block_allocator(std::size_t block_size, tracker t, Args&&... args)
            : tracker(detail::move(t)), allocator_type(block_size, detail::forward<Args>(args)...)
            {
            }

            /// \effects Calls <tt>Tracker::on_allocator_growth()</tt> after forwarding to the allocator.
            /// \returns The block as the returned by the allocator.
            memory_block allocate_block()
            {
                auto block = allocator_type::allocate_block();
                this->on_allocator_growth(block.memory, block.size);
                return block;
            }

            /// \effects Calls <tt>Tracker::on_allocator_shrinking()</tt> and forwards to the allocator.
            void deallocate_block(memory_block block) noexcept
            {
                this->on_allocator_shrinking(block.memory, block.size);
                allocator_type::deallocate_block(block);
            }

            /// \returns The next block size as returned by the allocator.
            std::size_t next_block_size() const noexcept
            {
                return allocator_type::next_block_size();
            }

            /// @{
            /// \returns A (const) reference to the used allocator.
            allocator_type& get_allocator() noexcept
            {
                return *this;
            }

            const allocator_type& get_allocator() const noexcept
            {
                return *this;
            }
            /// @}

            /// @{
            /// \returns A (const) reference to the tracker.
            tracker& get_tracker() noexcept
            {
                return *this;
            }

            const tracker& get_tracker() const noexcept
            {
                return *this;
            }
            /// @}
        };

        /// Similar to \ref tracked_block_allocator, but shares the tracker with the higher level allocator.
        /// This allows tracking both (de-)allocations and growth with one tracker.
        /// \note Due to implementation reasons, it cannot track growth and shrinking in the constructor/destructor of the higher level allocator.
        /// \ingroup adapter
        template <class Tracker, class BlockOrRawAllocator>
        using deeply_tracked_block_allocator = FOONATHAN_IMPL_DEFINED(
            detail::deeply_tracked_block_allocator<Tracker,
                                                   make_block_allocator_t<BlockOrRawAllocator>>);

        /// A \concept{concept_rawallocator,RawAllocator} adapter that tracks another allocator using a \concept{concept_tracker,tracker}.
        /// It wraps another \concept{concept_rawallocator,RawAllocator} and calls the tracker function before forwarding to it.
        /// The class can then be used anywhere a \concept{concept_rawallocator,RawAllocator} is required and the memory usage will be tracked.<br>
        /// If the \concept{concept_rawallocator,RawAllocator} uses \ref deeply_tracked_block_allocator as \concept{concept_blockallocator,BlockAllocator},
        /// it will also track growth and shrinking of the allocator.
        /// \ingroup adapter
        template <class Tracker, class RawAllocator>
        class tracked_allocator
        : FOONATHAN_EBO(Tracker, allocator_traits<RawAllocator>::allocator_type)
        {
            using traits            = allocator_traits<RawAllocator>;
            using composable_traits = composable_allocator_traits<RawAllocator>;

        public:
            using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;
            using tracker        = Tracker;

            using is_stateful = std::integral_constant<bool, traits::is_stateful::value
                                                                 || !std::is_empty<Tracker>::value>;

            /// @{
            /// \effects Creates it by giving it a \concept{concept_tracker,tracker} and the tracked \concept{concept_rawallocator,RawAllocator}.
            /// It will embed both objects.
            /// \note This will never call the <tt>Tracker::on_allocator_growth()</tt> function.
            explicit tracked_allocator(tracker t = {}) noexcept
            : tracked_allocator(detail::move(t), allocator_type{})
            {
            }

            tracked_allocator(tracker t, allocator_type&& allocator) noexcept
            : tracker(detail::move(t)), allocator_type(detail::move(allocator))
            {
                detail::set_tracker(0, get_allocator(), &get_tracker());
            }
            /// @}

            /// \effects Destroys both tracker and allocator.
            /// \note This will never call the <tt>Tracker::on_allocator_shrinking()</tt> function.
            ~tracked_allocator() noexcept
            {
                detail::set_tracker(0, get_allocator(), static_cast<tracker*>(nullptr));
            }

            /// @{
            /// \effects Moving moves both the tracker and the allocator.
            tracked_allocator(tracked_allocator&& other) noexcept
            : tracker(detail::move(other)), allocator_type(detail::move(other))
            {
                detail::set_tracker(0, get_allocator(), &get_tracker());
            }

            tracked_allocator& operator=(tracked_allocator&& other) noexcept
            {
                tracker::       operator=(detail::move(other));
                allocator_type::operator=(detail::move(other));
                detail::set_tracker(0, get_allocator(), &get_tracker());
                return *this;
            }
            /// @}

            /// \effects Calls <tt>Tracker::on_node_allocation()</tt> and forwards to the allocator.
            /// If a growth occurs and the allocator is deeply tracked, also calls <tt>Tracker::on_allocator_growth()</tt>.
            /// \returns The result of <tt>allocate_node()</tt>
            void* allocate_node(std::size_t size, std::size_t alignment)
            {
                auto mem = traits::allocate_node(get_allocator(), size, alignment);
                this->on_node_allocation(mem, size, alignment);
                return mem;
            }

            /// \effects Calls the composable node allocation function.
            /// If allocation was successful, also calls `Tracker::on_node_allocation()`.
            /// \returns The result of `try_allocate_node()`.
            void* try_allocate_node(std::size_t size, std::size_t alignment) noexcept
            {
                auto mem = composable_traits::try_allocate_node(get_allocator(), size, alignment);
                if (mem)
                    this->on_node_allocation(mem, size, alignment);
                return mem;
            }

            /// \effects Calls <tt>Tracker::on_array_allocation()</tt> and forwards to the allocator.
            /// If a growth occurs and the allocator is deeply tracked, also calls <tt>Tracker::on_allocator_growth()</tt>.
            /// \returns The result of <tt>allocate_array()</tt>
            void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
            {
                auto mem = traits::allocate_array(get_allocator(), count, size, alignment);
                this->on_array_allocation(mem, count, size, alignment);
                return mem;
            }

            /// \effects Calls the composable array allocation function.
            /// If allocation was succesful, also calls `Tracker::on_array_allocation()`.
            /// \returns The result of `try_allocate_array()`.
            void* try_allocate_array(std::size_t count, std::size_t size,
                                     std::size_t alignment) noexcept
            {
                auto mem =
                    composable_traits::try_allocate_array(get_allocator(), count, size, alignment);
                if (mem)
                    this->on_array_allocation(mem, count, size, alignment);
                return mem;
            }

            /// \effects Calls <tt>Tracker::on_node_deallocation()</tt> and forwards to the allocator's <tt>deallocate_node()</tt>.
            /// If shrinking occurs and the allocator is deeply tracked, also calls <tt>Tracker::on_allocator_shrinking()</tt>.
            void deallocate_node(void* ptr, std::size_t size, std::size_t alignment) noexcept
            {
                this->on_node_deallocation(ptr, size, alignment);
                traits::deallocate_node(get_allocator(), ptr, size, alignment);
            }

            /// \effects Calls the composable node deallocation function.
            /// If it was succesful, also calls `Tracker::on_node_deallocation()`.
            /// \returns The result of `try_deallocate_node()`.
            bool try_deallocate_node(void* ptr, std::size_t size, std::size_t alignment) noexcept
            {
                auto res =
                    composable_traits::try_deallocate_node(get_allocator(), ptr, size, alignment);
                if (res)
                    this->on_node_deallocation(ptr, size, alignment);
                return res;
            }

            /// \effects Calls <tt>Tracker::on_array_deallocation()</tt> and forwards to the allocator's <tt>deallocate_array()</tt>.
            /// If shrinking occurs and the allocator is deeply tracked, also calls <tt>Tracker::on_allocator_shrinking()</tt>.
            void deallocate_array(void* ptr, std::size_t count, std::size_t size,
                                  std::size_t alignment) noexcept
            {
                this->on_array_deallocation(ptr, count, size, alignment);
                traits::deallocate_array(get_allocator(), ptr, count, size, alignment);
            }

            /// \effects Calls the composable array deallocation function.
            /// If it was succesful, also calls `Tracker::on_array_deallocation()`.
            /// \returns The result of `try_deallocate_array()`.
            bool try_deallocate_array(void* ptr, std::size_t count, std::size_t size,
                                      std::size_t alignment) noexcept
            {
                auto res = composable_traits::try_deallocate_array(ptr, count, size, alignment);
                if (res)
                    this->on_array_deallocation(ptr, count, size, alignment);
                return res;
            }

            /// @{
            /// \returns The result of the corresponding function on the wrapped allocator.
            std::size_t max_node_size() const
            {
                return traits::max_node_size(get_allocator());
            }

            std::size_t max_array_size() const
            {
                return traits::max_array_size(get_allocator());
            }

            std::size_t max_alignment() const
            {
                return traits::max_alignment(get_allocator());
            }
            /// @}

            /// @{
            /// \returns A (\c const) reference to the wrapped allocator.
            allocator_type& get_allocator() noexcept
            {
                return *this;
            }

            const allocator_type& get_allocator() const noexcept
            {
                return *this;
            }
            /// @}

            /// @{
            /// \returns A (\c const) reference to the tracker.
            tracker& get_tracker() noexcept
            {
                return *this;
            }

            const tracker& get_tracker() const noexcept
            {
                return *this;
            }
            /// @}
        };

        /// \effects Takes a \concept{concept_rawallocator,RawAllocator} and wraps it with a \concept{concept_tracker,tracker}.
        /// \returns A \ref tracked_allocator with the corresponding parameters forwarded to the constructor.
        /// \relates tracked_allocator
        template <class Tracker, class RawAllocator>
        auto make_tracked_allocator(Tracker t, RawAllocator&& alloc)
            -> tracked_allocator<Tracker, typename std::decay<RawAllocator>::type>
        {
            return tracked_allocator<Tracker, typename std::decay<RawAllocator>::type>{detail::move(
                                                                                           t),
                                                                                       detail::move(
                                                                                           alloc)};
        }

        namespace detail
        {
            template <typename T, bool Block>
            struct is_block_or_raw_allocator_impl : std::true_type
            {
            };

            template <typename T>
            struct is_block_or_raw_allocator_impl<T, false> : memory::is_raw_allocator<T>
            {
            };

            template <typename T>
            struct is_block_or_raw_allocator
            : is_block_or_raw_allocator_impl<T, memory::is_block_allocator<T>::value>
            {
            };

            template <class RawAllocator, class BlockAllocator>
            struct rebind_block_allocator;

            template <template <typename...> class RawAllocator, typename... Args,
                      class OtherBlockAllocator>
            struct rebind_block_allocator<RawAllocator<Args...>, OtherBlockAllocator>
            {
                using type =
                    RawAllocator<typename std::conditional<is_block_or_raw_allocator<Args>::value,
                                                           OtherBlockAllocator, Args>::type...>;
            };

            template <class Tracker, class RawAllocator>
            using deeply_tracked_block_allocator_for =
                memory::deeply_tracked_block_allocator<Tracker,
                                                       typename RawAllocator::allocator_type>;

            template <class Tracker, class RawAllocator>
            using rebound_allocator = typename rebind_block_allocator<
                RawAllocator, deeply_tracked_block_allocator_for<Tracker, RawAllocator>>::type;
        } // namespace detail

        /// A \ref tracked_allocator that has rebound any \concept{concept_blockallocator,BlockAllocator} to the corresponding \ref deeply_tracked_block_allocator.
        /// This makes it a deeply tracked allocator.<br>
        /// It replaces each template argument of the given \concept{concept_rawallocator,RawAllocator} for which \ref is_block_allocator or \ref is_raw_allocator is \c true with a \ref deeply_tracked_block_allocator.
        /// \ingroup adapter
        template <class Tracker, class RawAllocator>
        FOONATHAN_ALIAS_TEMPLATE(
            deeply_tracked_allocator,
            tracked_allocator<Tracker, detail::rebound_allocator<Tracker, RawAllocator>>);

        /// \effects Takes a \concept{concept_rawallocator,RawAllocator} and deeply wraps it with a \concept{concept_tracker,tracker}.
        /// \returns A \ref deeply_tracked_allocator with the corresponding parameters forwarded to the constructor.
        /// \relates deeply_tracked_allocator
        template <class RawAllocator, class Tracker, typename... Args>
        auto make_deeply_tracked_allocator(Tracker t, Args&&... args)
            -> deeply_tracked_allocator<Tracker, RawAllocator>
        {
            return deeply_tracked_allocator<Tracker, RawAllocator>(detail::move(t),
                                                                   {detail::forward<Args>(
                                                                       args)...});
        }
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
