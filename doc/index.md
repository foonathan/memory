This is the documentation of `foonathan/memory`.

For a quick start, read the [Tutorial] or skim the examples at the [Github page].
The concepts of this library are defined are [here](md_doc_concepts.html).

## Short example

This is a short example using a some allocators.

~~~{.cpp}
#include <algorithm>
#include <iostream>
#include <iterator>

#include <memory/container.hpp>
#include <memory/memory_pool.hpp>
#include <memory/smart_ptr.hpp>
#include <memory/temporary_allocator.hpp>

template <typename BiIter>
void merge_sort(BiIter begin, BiIter end)
{
    using value_type = typename std::iterator_traits<BiIter>::value_type;

    auto distance = std::distance(begin, end);
    if (distance <= 1)
        return;

    auto mid = begin;
    std::advance(mid, distance / 2);

    auto alloc = memory::make_temporary_allocator();

    // alias for std::vector<value_type, memory::std_allocator<value_type, memory::temporary_allocator>>
    memory::vector<value_type, memory::temporary_allocator> first(begin, mid, alloc),
                                                            second(mid, end, alloc);

    merge_sort(first.begin(), first.end());
    merge_sort(second.begin(), second.end());
    std::merge(first.begin(), first.end(), second.begin(), second.end(), begin);
}

int main()
{
    memory::memory_pool<> pool(memory::list_node_size<int>::value, 4096u);

    // alias for std::list<int, memory::std_allocator<int, memory::memory_pool<>>
    memory::list<int, memory::memory_pool<>> list(pool);
    list.push_back(3);
    list.push_back(2);
    list.push_back(1);

    merge_sort(list.begin(), list.end());

    // a unique_ptr using the pool
    auto ptr = memory::allocate_unique<int>(pool, *list.begin());
    std::cout << *ptr << '\n';
}
~~~

## About this documentation

This documentation is written in a similar way as the C++ standard itself, although not that formal.

Concepts are documented using the names of the template parameters, for example the following class:

~~~{.cpp}
template <class Tracker, class RawAllocator>
class tracked_allocator;
~~~

It takes two template parameters, the first must model the [Tracker] concept, the second the [RawAllocator] concept.

Unless explicitly stated otherwise, it is not allowed to call a function that modifies state from two different threads.
Functions that modify state are non-`const` member functions, functions taking a non-`const` reference to objects
or functions where it is explictly documented that they change some hidden state.

If a function is documented as `noexcept`, it does not throw anything.
Otherwise it has a *Throws:* clause specifying what it throws, or if it is a forwarding function, the information can be found there (see below).

If a class is described as [RawAllocator] it automatically has certain semantically information which are not explictly mentioned.
This is especially true for the member functions of an [allocator_traits] specialization.

If a function is described as returning the value of another function or forwarding to it,
it implicitly has the requirements and effects from the called function and can also throw the same things.

[Tutorial]: md_doc_tutorial.html
[Github page]: https://github.com/foonathan/memory/
[Tracker]: md_doc_concepts.html#concept_tracker
[RawAllocator]: md_doc_concepts.html#concept_rawallocator
[allocator_traits]: \ref foonathan::memory::allocator_traits
