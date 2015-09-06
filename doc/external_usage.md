# Using RawAllocators in STL containers, smart pointers, etc.

## 1. Using Containers with RawAllocators

The following listing shows how to use a [memory_pool] with a `std::list` container:

~~~{.cpp}
#include <memory/container.hpp> // for list, list_node_size
#include <memory/memory_pool.hpp> // for memory_pool

int main()
{
    memory::memory_pool<> pool(memory::list_node_size<int>::value, 1024);
    memory::list<int, decltype(pool)> list(pool);
    // use list as normal
}
~~~

The first line in `main()` declares a [memory_pool].
A memory pool is a special kind of allocator that takes a big memory block and separates it into many smaller
[nodes] of a given size. Free nodes are put onto a list and can be retrieved in constant time.
This structures allows (de-)allocations in any order, but only for a fixed size.
But this is exactly the memory footprint for a node based container, like `std::list`:
Each element has the same size and they can be created and destroyed at any time.

[memory_pool] is templated but the default parameters are just fine for the most use cases so they are used.
Its constructor takes two parameters: The first one is the fixed size of each node.
The pool will be used two allocate the nodes for a list of int, but `sizeof(int)` isn't enough,
since each node also stores the two pointers to the next and previous node in the list.
To avoid guessing its size which also varies between STL implementations,
they are automatically obtained on building and stored in integral constants of the from `<container>_node_size<T>`.
In this case it is a list of `int` and thus `list_node_size<int>::value` is the node size we need.
The second parameter is simply the size of big block that will be seperated.
All allocators that are working on bigger memory blocks can grow, if their initial capacity is exhausted,
but it is better to use a big size at the beginning.

The second line then actually declares the list.
Since [RawAllocator] provides a conceptually very different interface than `Allocator`,
they cannot be used directly, but need to be wrapped in the class [std_allocator].
It takes a `RawAllocator` and provides the interface of an `Allocator` forwarding to the underlying raw allocator
and taking care of rebinding, container copying and threading.
The raw allocator itself is only stored as reference, not directly embedded.
This is required by the `Allocator` model which wants to copy allocators, but `RawAllocator` objects are only moveable.
In addition, the `get_allocator()` function of containers only return a copy of the allocator,
access to the direct allocator isn't possible.
By storing a reference (actually a pointer) inside the `std_allocator`, copying is enabled
and the raw allocator can be accessed either directly or via getter function of the `std_allocator` object.

For simplicity, template aliases are provided in `container.hpp` that do the wrapping.
The above `memory::list<...>` is equivalent to `std::list<int, memory::std_allocator<int, decltype(pool)>>`.
Due to the nature of the `Allocator` model, the `value_type` has to be repeated twice,
also the `Allocator` is the last template parameter, leading to a very verbose
`std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, memory::std_allocator<std::pair<const Key, Value>, RawAllocator>>`
as opposed to `memory::unordered_map<Key, Value, RawAllocator>`.
But of course the verbose form can be used, in this case `std_allocator.hpp` has to be included to get `std_allocator`.

Since the type of `list` is a normal `std::list`, just with a special allocator,
it provides all the normal functions. The constructor used is the one taking the allocator object,
an automatic conversion to `std_allocator` allows to pass it the pool directly.
Then the object can be used as normal, passed to algorithms and freely copied, moved or swapped.
`std_allocator` ensures that each copy uses the same [memory_pool] as its allocator.

The same procedure - create a [RawAllocator], wrap it inside a [std_allocator] and pass it to a container,
optionally the last two steps combined - can be used for all other STL containers, `basic_string` or any class taking an `Allocator` object.
Node size values are provided for all node based STL containers (lists, sets and maps).

## 2. RawAllocators as Deleter classes

But not all STL classes require the full `Allocator`, some only need a `Deleter`.
A `Deleter` is just a function object that can be called with a pointer and should free it.
Like [std_allocator] is a wrapper to provide the `Allocator` interface,
there are two kinds of deleter wrappers defined in `deleter.hpp`:
[allocator_deallocator] and [allocator_deleter].
The former just deallocates the memory without calling destructors, the latter does call destructors.
They also store a reference instead of the actual allocator for the same reason as in `std_allocator`
and take care of synchronization.
And like the container typedefs, there is an easier way to handle the most common use case of deleters: smart pointers.

The following excerpt shows the handling of smart pointers:

~~~.cpp
#include <memory/smart_ptr.hpp> // for allocate_XXX
...
// assume we have a RawAllocator alloc
auto unique_ptr = memory::allocate_unique<int>(alloc, 5); // (1)
auto array_unique_ptr = memory::allocate_unique<int[]>(alloc, 10u) // (2)
auto shared_ptr = memory::allocate_shared<int>(alloc, 7) // (3)
~~~

At (1) a `std::unique_ptr` is created storing a dynamically allocated `int` of value `5` via a `RawAllocator` `alloc`.
It is another great use case for C++11's `auto`,
the actual type would be `std::unique_ptr<int, memory::allocator_deleter<int, RawAllocator>>`.
The deleter and function also works with arrays, of course, as (2) shows:
It creates an array of `10` value-initialized integers.
A similar function is provided for `std::shared_ptr` used in (3).
It uses the `std::allocate_shared` function internally and thus guarantees the efficient allocation.
Like in the standard library, there is no array version for shared pointers.
And since the results are instantiations of the actual standard library pointers,
they can be used as usual.
Especially `std::shared_ptr` can be very easily integrated,
since the actual allocator or deleter is type erased.

## 3. Temporary allocations

The third big use case for allocators besides containers or single objects
are temporary allocations.
Sometimes an algorithm needs a temporary buffer to store some results.
Variable length arrays - although currently not part of the C++ standard - are a common solution.
There are either compiler extensions allowing normal variables to be used as array size directly
or the more low level approach via `alloca()`.
`alloca()` allocates memory by simply adjusting the top pointer of the stack.
The resulting memory is thus available directly on the stack and will be automatically freed on function exit.
The allocation is also much faster than heap allocation directly.

But although `alloca()` is available on many platforms, it is not portable.
In addition, out of memory cannot be reported, since it leads to a stack overflow
and nothing can be done then.
Thus it is not recommended to use it.
Instead, use the [temporary_allocator] class available in `temporary_allocator.hpp`.
It does not use the real program stack for the allocation,
but its own, separate stack for each thread obtained from the heap.

Below is a simple implementation of a merge sort that uses a temporary buffer:

~~~.cpp
#include <algorithm>
#include <iterator>

#include <memory/container.hpp> // for memory::vector
#include <memory/temporary_allocator.hpp> // for memory::temporary_allocator

template <typename RAIter>
void merge_sort(RAIter begin, RAIter end)
{
    using value_type = typename std::iterator_traits<RAIter>::value_type;

    if (end - begin <= 1)
        return;
    auto mid = begin + (end - begin) / 2;

    auto alloc = memory::make_temporary_allocator(); // (1)
    memory::vector<value_type, memory::temporary_allocator> first(begin, mid, alloc),
                                                            second(mid, end, alloc); // (2)

    merge_sort(first.begin(), first.end());
    merge_sort(second.begin(), second.end());
    std::merge(first.begin(), first.end(), second.begin(), second.end(), begin);
}
~~~

The usage of [temporary_allocator] is just as usual:
At (1), the allocator is created.
It must be created on the stack, so there is no public constructor,
only the `make_temporary_allocator()` function that returns a new allocator object.
Then it can be used to create the vectors as usual in (2).

Temporary memory allocations are extremely fast, as they only need to adopt the pointer in the internal stack.
Deallocations are a no-op, since they are done automatically when the scope of the allocator object is left.
This is ensured by the constructor, who saves the current top of the stack, and the destructor, who simply sets the pointer back.
For that reason, allocations must not be made from any object other than the last created one.
Each thread has its own seperate, internal stack.
It is only created, however, on the first call to `make_temporary_allocator()` to avoid the huge memory allocation when not needed.
The size of the internal stack, can also be specified in the make function,
if it is not the first one, it will be ignored.
The stack can grow if the initial size is exhausted, although it may lead to a slow heap allocation.


[allocator_deallocator]: \ref foonathan::memory::allocator_deallocator
[allocator_deleter]: \ref foonathan::memory::allocator_deleter
[memory_pool]: \ref foonathan::memory::memory_pool
[std_allocator]: \ref foonathan::memory::std_allocator
[temporary_allocator]: \ref foonathan::memory::temporary_allocator
[nodes]: md_doc_concepts.html#concept_node
[RawAllocator]: md_doc_concepts.html#concept_rawallocator
