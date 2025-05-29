# Writing classes using a RawAllocator {#md_doc_internal_usage}

Compared to the requirements an `AllocatorAwareContainer` has to fulfill,
it is very easy to use a `RawAllocator` in a container.
There is no need to worry about comparing allocators, `select_on_container_copy_construction()`,
`propagate_on_container_move_assignment` or the undefined behavior that sometimes happens if you `swap()` a container.

## The Allocator version

To demonstrate this, consider a simple `deep_copy_ptr`. `deep_copy_ptr` is like `std::unique_ptr` but provides a copy constructor
which will perform a copy of the object.
Unlike `std::unique_ptr` it will take a full-blown `Allocator`. Then it will be transformed to use a [RawAllocator].
It is only meant to demonstrate the use of allocator classes and not to be a real use smart pointer class
(it is pretty dumb, it copies the pointee on copy but invalidates on move...).
So, this is it:

```cpp
template <typename T, class Allocator = std::allocator<T>>
class deep_copy_ptr
: Allocator
{
    using traits = std::allocator_traits<Allocator>;
public:
    using value_type = typename traits::value_type;
    using allocator_type = Allocator;

    explicit deep_copy_ptr(const allocator_type &alloc = allocator_type{})
    : allocator_type(alloc), ptr_(nullptr) {}

    deep_copy_ptr(value_type value, const allocator_type &alloc = allocator_type{})
    : allocator_type(alloc), ptr_(create(*this, std::move(value))) {}

    deep_copy_ptr(const deep_copy_ptr &other)
    : allocator_type(traits::select_on_container_copy_construction(other)),
      ptr_(create(*this, *other))
    {}

    deep_copy_ptr(deep_copy_ptr &&other) noexcept
    : allocator_type(std::move(other)),
      ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }

    ~deep_copy_ptr() noexcept
    {
        destroy();
    }

    deep_copy_ptr& operator=(const deep_copy_ptr &other)
    {
        if (traits::propagate_on_container_copy_assignment::value && static_cast<Allocator&>(*this) != other)
        {
            allocator_type alloc(other);
            auto ptr = create(alloc, *other);
            destroy();

            Allocator::operator=(std::move(alloc));
            ptr_ = ptr;
        }
        else
        {
            auto ptr = create(*this, *other);
            destroy();
            ptr_ = ptr;
        }
        return *this;
    }

    deep_copy_ptr& operator=(deep_copy_ptr &&other) noexcept(traits::propagate_on_container_move_assignment::value)
    {
        if (traits::propagate_on_container_move_assignment::value && static_cast<allocator_type&>(*this) != other)
        {
            allocator_type::operator=(std::move(other));
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        else if (static_cast<allocator_type&>(*this) == other)
        {
            ptr_ = other.ptr_;
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

    friend void swap(deep_copy_ptr &a, deep_copy_ptr &b) noexcept
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
    template <typename ... Args>
    typename traits::pointer create(allocator_type &alloc, Args&&... args)
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
```

I am not going to go into much detail about this code, since it is just to demonstrate the complexity involved with the `Allocator` model.
To note is the following:

* The `Allocator` is inherited privately to use the empty base optimization if it is an empty type.
Also the allocator is *owned* by the pointer.
* All access to the `Allocator` is done through the `std::allocator_traits` class.
In addition, the actual `value_type` and pointer must be obtained from the traits class and its appropriate functions called to construct/destroy the object.
* The copy constructor must call `traits::select_on_container_copy_construction()`, the move constructor can just move the allocator.
* Copy and Move assignment and `swap()` only exchange the container if the appropriate `traits::propagate_on_container_XXX` is `true`.
This involves a lot of complexity since if it is `false` - which is the default! - the old memory has to be deallocated on the old allocator
and the new memory allocated on the new allocator if the allocators aren't *equal* - even for move!
Also note the `assert()` in `swap()`: Since `swap()` must not throw, it cannot do the reallocation if the propagation is `false`.

## The RawAllocator version

This is now a step-by-step review of the changes in the version that uses a [RawAllocator].

```cpp
template <typename T, class RawAllocator = memory::default_allocator>
class deep_copy_ptr
: memory::allocator_reference<RawAllocator>
```
The default allocator is now [default_allocator]. Its actual type can be changed when building this library,
but it is similar to `std::allocator`.
Also the allocator is stored in a [allocator_reference].
This is recommended for three reasons:

a) Usage requirement: `RawAllocator` classes are only required to be moveable. [allocator_reference] is copyable, this allows copying the `deep_copy_ptr`.

b) Simplicity: [allocator_reference] provides the full interface without using the [allocator_traits] class.
It has already done the wrapping for you.

c) Ownership: The `deep_copy_ptr` doesn't *own* the allocator, it can be shared with other classes or objects.
This is a useful semantic change which is often required anyway.
*Note: The passed allocator object must now live as long as the container object, except for stateless allocators!*

The reference is inherited too for the same reason:
It is empty for stateless allocators. They are constructed on-the-fly.
This also means that they can be passed in as a temporary.
For stateful allocators it stores a pointer. The user has to ensure that the referenced allocator object then outlives the `deep_copy_ptr` object.

```cpp
    using allocator_ref = memory::allocator_reference<RawAllocator>;
public:
    using value_type = T;
    using allocator_type = typename allocator_ref::allocator_type;

    explicit deep_copy_ptr(allocator_ref alloc = allocator_type{})
    : allocator_ref(alloc), ptr_(nullptr) {}

    deep_copy_ptr(value_type value, allocator_ref alloc = allocator_type{})
    : allocator_ref(alloc), ptr_(create(std::move(value))) {}

    deep_copy_ptr(const deep_copy_ptr &other)
    : allocator_ref(other),
      ptr_(create(*other))
    {}

    deep_copy_ptr(deep_copy_ptr &&other) noexcept
    : allocator_ref(std::move(other)),
      ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }
```

Not much changed with the typedefs: The traits typedef can be removed, instead there is one for the reference.
The `value_type` is now the template parameter directly but the `allocator_type` is defined in the reference through the traits.
This allows rebinding to support `Allocator` classes.

The constructors now take an `allocator_ref` instead of the `allocator_type` directly but otherwise are left unchanged.
Note that the assignment of a default constructed `allocator_type` only compiles for stateless allocators,
since the reference does not actual store a reference to them. For stateful it wil not compile.
Since only the reference is copied and not the allocator there is no need for a special treatment in copying.
`create()` no longer needs to take an allocator as reference so this argument can be omitted.

The destructor has not changed at all, it still only calls the helper function `destroy()`.

Copy and move assignment operators can now use the copy(move)-and-swap-idiom and do not need to worry about all the propagation stuff
since the allocator is held by reference. Same goes for `swap()` which just swaps the reference and pointer.

The accessor functions have not changed, except that the actual pointer type is now simply `T*` and no longer defined in the traits.

```cpp
template <typename ... Args>
T* create(Args&&... args)
{
    auto storage = this->allocate_node(sizeof(T), alignof(T));
    try
    {
        ::new(storage) T(std::forward<Args>(args)...);
    }
    catch (...)
    {
        this->deallocate_node(storage, sizeof(T), alignof(T));
        throw;
    }
    return static_cast<T*>(storage);
}

void destroy() noexcept
{
    if (ptr_)
    {
        ptr_->~T();
        this->deallocate_node(ptr_, sizeof(T), alignof(T));
    }
}
```

The helper functions `create()` and `destroy()` no only perform the (de-)allocation through the allocator,
constructor/destructor call is done manually. Note that the pointer returned by `allocate_node()` is `void*`
and that you have to explicitly specify `this->` due to the template name lookup rules.

This is now the full `RawAllocator` version of `deep_copy_ptr`:

```cpp
template <typename T, class RawAllocator = memory::default_allocator>
class deep_copy_ptr
: memory::allocator_reference<RawAllocator>
{
    using allocator_ref = memory::allocator_reference<RawAllocator>;
public:
    using value_type = T;
    using allocator_type = typename allocator_ref::allocator_type;

    explicit deep_copy_ptr(allocator_ref alloc = allocator_type{})
    : allocator_ref(alloc), ptr_(nullptr) {}

    deep_copy_ptr(value_type value, allocator_ref alloc = allocator_type{})
    : allocator_ref(alloc), ptr_(create(std::move(value))) {}

    deep_copy_ptr(const deep_copy_ptr &other)
    : allocator_ref(other),
      ptr_(create(*other))
    {}

    deep_copy_ptr(deep_copy_ptr &&other) noexcept
    : allocator_ref(std::move(other)),
      ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }

    ~deep_copy_ptr() noexcept
    {
        destroy();
    }

    deep_copy_ptr& operator=(const deep_copy_ptr &other)
    {
        deep_copy_ptr tmp(other);
        swap(*this, tmp);
        return *this;
    }

    deep_copy_ptr& operator=(deep_copy_ptr &&other) noexcept
    {
        deep_copy_ptr tmp(std::move(other));
        swap(*this, tmp);
        return *this;
    }

    friend void swap(deep_copy_ptr &a, deep_copy_ptr &b) noexcept
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
    template <typename ... Args>
    T* create(Args&&... args)
    {
        auto storage = this->allocate_node(sizeof(T), alignof(T));
        try
        {
            ::new(storage) T(std::forward<Args>(args)...);
        }
        catch (...)
        {
            this->deallocate_node(storage, sizeof(T), alignof(T));
            throw;
        }
        return static_cast<T*>(storage);
    }

    void destroy() noexcept
    {
        if (ptr_)
        {
            ptr_->~T();
            this->deallocate_node(ptr_, sizeof(T), alignof(T));
        }
    }

    T *ptr_;
};
```

[default_allocator]: \ref foonathan::memory::default_allocator
[allocator_reference]: \ref foonathan::memory::allocator_reference
[allocator_traits]: \ref foonathan::memory::allocator_traits
[allocator_deallocator]: \ref foonathan::memory::allocator_deallocator
[RawAllocator]: md_doc_concepts.html#concept_rawallocator
