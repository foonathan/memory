// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// this example provides two implementation of a deep_copy_ptr that performs a copy when copying the pointer
// the first version takes an Allocator the second a RawAllocator
// see https://memory.foonathan.net/md_doc_internal_usage.html for a step-by-step walkthrough
// I know the class is pretty dumb and not that great designed (copy performs deep copy, move invalidates), but that's not that the point

#include <cassert>
#include <memory>
#include <iostream>

#include <foonathan/memory/allocator_storage.hpp> // allocator_reference
#include <foonathan/memory/default_allocator.hpp> // default_allocator

// alias namespace foonathan::memory as memory for easier access
#include <foonathan/memory/namespace_alias.hpp>

namespace using_std_allocator
{
    template <typename T, class Allocator = std::allocator<T>>
    class deep_copy_ptr : Allocator
    {
        using traits = std::allocator_traits<Allocator>;

    public:
        using value_type     = typename traits::value_type;
        using allocator_type = Allocator;

        explicit deep_copy_ptr(const allocator_type& alloc = allocator_type{})
        : allocator_type(alloc), ptr_(nullptr)
        {
        }

        deep_copy_ptr(value_type value, const allocator_type& alloc = allocator_type{})
        : allocator_type(alloc), ptr_(create(*this, std::move(value)))
        {
        }

        deep_copy_ptr(const deep_copy_ptr& other)
        : allocator_type(traits::select_on_container_copy_construction(other)),
          ptr_(create(*this, *other))
        {
        }

        deep_copy_ptr(deep_copy_ptr&& other) noexcept
        : allocator_type(std::move(other)), ptr_(other.ptr_)
        {
            other.ptr_ = nullptr;
        }

        ~deep_copy_ptr() noexcept
        {
            destroy();
        }

        deep_copy_ptr& operator=(const deep_copy_ptr& other)
        {
            if (traits::propagate_on_container_copy_assignment::value
                && static_cast<Allocator&>(*this) != other)
            {
                allocator_type alloc(other);
                auto           ptr = create(alloc, *other);
                destroy();

                Allocator::operator=(std::move(alloc));
                ptr_               = ptr;
            }
            else
            {
                auto ptr = create(*this, *other);
                destroy();
                ptr_ = ptr;
            }
            return *this;
        }

        deep_copy_ptr& operator=(deep_copy_ptr&& other) noexcept(
            traits::propagate_on_container_move_assignment::value)
        {
            if (traits::propagate_on_container_move_assignment::value
                && static_cast<allocator_type&>(*this) != other)
            {
                allocator_type::operator=(std::move(other));
                ptr_                    = other.ptr_;
                other.ptr_              = nullptr;
            }
            else if (static_cast<allocator_type&>(*this) == other)
            {
                ptr_       = other.ptr_;
                other.ptr_ = nullptr;
            }
            else
            {
                auto ptr = create(*this, std::move(*other));
                destroy();
                ptr_ = ptr;
            }
            return *this;
        }

        friend void swap(deep_copy_ptr& a, deep_copy_ptr& b) noexcept
        {
            using std::swap;
            if (traits::propagate_on_container_swap::value)
                swap(static_cast<allocator_type&>(a), static_cast<allocator_type&>(b));
            else
                assert(static_cast<allocator_type&>(a) == b);
            swap(a.ptr_, b.ptr_);
        }

        explicit operator bool() const
        {
            return !!ptr_;
        }

        T& operator*()
        {
            return *ptr_;
        }

        const T& operator*() const
        {
            return *ptr_;
        }

        typename traits::pointer operator->()
        {
            return ptr_;
        }

        typename traits::const_pointer operator->() const
        {
            return ptr_;
        }

    private:
        template <typename... Args>
        typename traits::pointer create(allocator_type& alloc, Args&&... args)
        {
            auto ptr = traits::allocate(alloc, 1);
            try
            {
                traits::construct(alloc, ptr, std::forward<Args>(args)...);
            }
            catch (...)
            {
                traits::deallocate(alloc, ptr, 1);
                throw;
            }
            return ptr;
        }

        void destroy() noexcept
        {
            if (ptr_)
            {
                traits::destroy(*this, ptr_);
                traits::deallocate(*this, ptr_, 1);
            }
        }

        typename traits::pointer ptr_;
    };
} // namespace using_std_allocator

namespace using_raw_allocator
{
    template <typename T,
              class RawAllocator =
                  memory::default_allocator> // default allocator type, usually heap_allocator
    class deep_copy_ptr
    : memory::allocator_reference<
          RawAllocator> // store the allocator by reference to allow sharing and copying
    // for stateless allocators like default_allocator, it does not store an actual reference
    // and can take a temporary, for stateful, the allocator must outlive the reference
    {
        using allocator_ref = memory::allocator_reference<RawAllocator>;

    public:
        using value_type     = T;
        using allocator_type = typename allocator_ref::allocator_type;

        explicit deep_copy_ptr(allocator_ref alloc = allocator_type{})
        : allocator_ref(alloc), ptr_(nullptr)
        {
        }

        deep_copy_ptr(value_type value, allocator_ref alloc = allocator_type{})
        : allocator_ref(alloc), ptr_(create(std::move(value)))
        {
        }

        deep_copy_ptr(const deep_copy_ptr& other) : allocator_ref(other), ptr_(create(*other)) {}

        deep_copy_ptr(deep_copy_ptr&& other) noexcept
        : allocator_ref(std::move(other)), ptr_(other.ptr_)
        {
            other.ptr_ = nullptr;
        }

        ~deep_copy_ptr() noexcept
        {
            destroy();
        }

        // assignment uses straightforward copy/move-and-swap idiom, instead of boilerplate required by Allocator
        deep_copy_ptr& operator=(const deep_copy_ptr& other)
        {
            deep_copy_ptr tmp(other);
            swap(*this, tmp);
            return *this;
        }

        deep_copy_ptr& operator=(deep_copy_ptr&& other) noexcept
        {
            deep_copy_ptr tmp(std::move(other));
            swap(*this, tmp);
            return *this;
        }

        // swap is straightforward too
        friend void swap(deep_copy_ptr& a, deep_copy_ptr& b) noexcept
        {
            using std::swap;
            swap(static_cast<allocator_ref&>(a), static_cast<allocator_ref&>(b));
            swap(a.ptr_, b.ptr_);
        }

        explicit operator bool() const
        {
            return !!ptr_;
        }

        T& operator*()
        {
            return *ptr_;
        }

        const T& operator*() const
        {
            return *ptr_;
        }

        T* operator->()
        {
            return ptr_;
        }

        const T* operator->() const
        {
            return ptr_;
        }

    private:
        template <typename... Args>
        T* create(Args&&... args)
        {
            auto storage =
                this->allocate_node(sizeof(T), alignof(T)); // first allocate storage for the node
            try
            {
                ::new (storage) T(std::forward<Args>(args)...); // then call constructor
            }
            catch (...)
            {
                this->deallocate_node(storage, sizeof(T),
                                      alignof(T)); // if failure, deallocate storage again
                throw;
            }
            return static_cast<T*>(storage);
        }

        void destroy() noexcept
        {
            if (ptr_)
            {
                ptr_->~T();                                         // call destructor
                this->deallocate_node(ptr_, sizeof(T), alignof(T)); // deallocate storage
            }
        }

        T* ptr_;
    };
} // namespace using_raw_allocator

// simple usage functions

template <class Ptr>
void use_ptr()
{
    Ptr a(4), b(3);
    std::cout << *a << ' ' << *b << '\n';
    swap(a, b);

    auto c = a;
    std::cout << *a << ' ' << *c << '\n';

    auto d = std::move(b);
    std::cout << std::boolalpha << *d << ' ' << !!b << '\n';
}

int main()
{
    std::cout << "Allocator\n\n";
    use_ptr<using_std_allocator::deep_copy_ptr<int>>();
    std::cout << "\n\nRawAllocator\n\n";
    use_ptr<using_raw_allocator::deep_copy_ptr<int>>();
}
