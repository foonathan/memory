#ifndef FOONATHAN_MEMORY_DETAIL_MEMORY_BLOCK_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_MEMORY_BLOCK_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <utility>

namespace foonathan { namespace memory
{
    // ugly copy-paste due to quick "get-it-working"   
    inline std::size_t align(void* ptr, std::size_t alignment) noexcept
    {
        auto address = reinterpret_cast<uintptr_t>(ptr);
        auto misaligned = address & (alignment - 1);
        if (misaligned == 0)
            return 0;
        return alignment - misaligned;
    }

    /// \cond impl
    namespace detail
    {
        template <class Allocator>
        class memory_block_list : Allocator
        {
            struct node
            {
                node *next;
            };

            static_assert(sizeof(node) <= alignof(std::max_align_t),
                          "can't fit node into aligned memory block;"
                          " contact support");
        public:
            class iterator
            {
            public:
                iterator() noexcept
                : cur_(nullptr) {}

                iterator& operator++() noexcept
                {
                    assert(cur_ && "cannot increment end iterator");
                    cur_ = cur_->next;
                    return *this;
                }

                iterator operator++(int) noexcept
                {
                    assert(cur_ && "cannot increment end iterator");
                    iterator tmp(*this);
                    cur_ = cur_->next;
                    return tmp;
                }

                explicit operator bool() const noexcept
                {
                    return cur_;
                }

                char* memory() const noexcept
                {
                    assert(cur_ && "cannot access end iterator");
                    void* mem = cur_;
                    return static_cast<char*>(mem) + sizeof(node);
                }

                char* memory_end(std::size_t size) const noexcept
                {
                    return memory() + size;
                }

            private:
                iterator(node *cur) noexcept
                : cur_(cur) {}

                node *cur_;

                friend memory_block_list;
            };

            memory_block_list(std::size_t size,
                              Allocator allocator)
            : Allocator(std::move(allocator)),
              first_(nullptr), size_(0), block_size_(size)
            {
                auto mem = this->allocate(block_size_ + alignof(std::max_align_t), alignof(node));
                first_ = ::new(mem) node{nullptr};
            }

            memory_block_list(memory_block_list &&other) noexcept
            : Allocator(std::move(other)),
              first_(other.first_), size_(other.size_),
              block_size_(other.block_size_)
            {
                other.first_ = nullptr;
            }

            ~memory_block_list() noexcept
            {
                erase(first_);
            }

            memory_block_list& operator=(memory_block_list &&other) noexcept
            {
                Allocator::operator=(std::move(other));
                first_ = other.first_;
                size_ = other.size_;
                block_size_ = other.block_size_;
                other.first_ = nullptr;
                return *this;
            }

            // strong exception safety
            void emplace_after(iterator iter)
            {
                auto cur = iter.cur_;
                auto mem = this->allocate(block_size_ + alignof(std::max_align_t), alignof(node));
                auto n = ::new(mem) node{cur->next};
                cur->next = n;
                ++size_;
            }

            void erase_all_after(iterator iter) noexcept
            {
                ++iter;
                erase(iter.cur_);
            }

            iterator begin() const noexcept
            {
                return first_;
            }

            std::size_t size() const noexcept
            {
                return size_;
            }

            std::size_t block_size() const noexcept
            {
                return block_size_;
            }

            Allocator& get_allocator() noexcept
            {
                return *this;
            }

        private:
            void erase(node *n) noexcept
            {
                while (n)
                {
                    auto next = n->next;
                    this->deallocate(n, block_size_ + alignof(std::max_align_t),
                                     alignof(node));
                    n = next;
                }
            }

            node *first_;
            std::size_t size_, block_size_;
        };
    } // namespace detail
    /// \endcond
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_MEMORY_BLOCK_HPP_INCLUDED
