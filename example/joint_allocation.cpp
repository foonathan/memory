// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// this examples shows how to use the joint memory facilities
// they allow sharing the same memory for the dynamic allocation of an object
// and dynamic allocations the member make

#include <iostream>

#include <foonathan/memory/container.hpp>         // string
#include <foonathan/memory/default_allocator.hpp> // default_allocator
#include <foonathan/memory/joint_allocator.hpp> // joint_type, joint_ptr, joint_allocator, joint_array, ...

// alias namespace foonathan::memory as memory for easier access
#include <foonathan/memory/namespace_alias.hpp>

// define our joint type
// we need to inherit from memory::joint_type<T>
// the base class injects two pointers for managing the joint memory
// and also disables default copy/move semantics
// as well as regular creation
struct my_type : memory::joint_type<my_type>
{
    // we can define arbitrary members here
    // in order to access the joint memory
    // we can use the joint_allocator
    // it works best with sequence containers
    // which do not need to grow/shrink

    // for example, a std::string that uses the joint memory for the allocation
    memory::string<memory::joint_allocator> str;
    // again: just an alias for std::basic_string<char, std::char_traits<char>,
    //                      memory::std_allocator<char, memory::joint_allocator>>

    // the joint_allocator has a slight overhead
    // for situations where you just need a dynamic, but fixed-sized array, use:
    memory::joint_array<int> array;
    // this is similar to std::vector<int> but cannot grow
    // it is more efficient than using memory::vector<int, memory::joint_allocator>

    // all constructors must take memory::joint as first parameter
    // as you cannot create the type, you cannot create it by accident
    // it also contains important metadata (like the allocation size)
    // and must be passed to the base class
    // you must pass *this as allocator to members where needed
    // (i.e. the object with the joint memory)
    my_type(memory::joint tag, const char* name)
    : memory::joint_type<my_type>(tag),          // pass metadata
      str(name, memory::joint_allocator(*this)), // create string
      array({1, 2, 3, 4, 5}, *this)              // create array
    {
    }

    // default copy/move constructor are deleted
    // you have to define your own with the memory:;joint as first parameter
    // IMPORTANT: when you have STL containers as member,
    // you must use the copy/move constructors with a special allocator
    // you again have to pass *this as allocator,
    // so that they use the current object for memory,
    // not the other one
    // if you forget it on a copy constructor, your code won't compile
    // but if you forget on a move constructor, this can't be detected!
    // note: joint_array will always not compile
    my_type(memory::joint tag, const my_type& other)
    : memory::joint_type<my_type>(tag), // again: pass metadata
      // note: str(other.str, *this) should work as well,
      // but older GCC don't support it
      str(other.str.c_str(), memory::joint_allocator(*this)), // important: pass *this as allocator
      array(other.array, *this)                               // dito
    {
    }
};

int main()
{
    // in order to create an object with joint memory,
    // you must use the joint_ptr
    // it is similar to std::unique_ptr,
    // but it also manages the additional object

    // to create one, use allocate_joint or the constructor
    // you have to pass the allocator used for the memory allocation,
    // the size of the additional shared memory
    // followed by constructor arguments for the type
    auto ptr = memory::allocate_joint<my_type>(memory::default_allocator{},
                                               // be careful with your size calculations
                                               // and keep alignment buffers in mind
                                               // if your size is too small,
                                               // it will throw an exception
                                               memory::joint_size(20 * sizeof(char)
                                                                  + 10 * sizeof(int) + 10),
                                               "joint allocations!");
    // ptr has the type: memory::joint_ptr<my_type, memory::default_allocator>
    // it points to memory that is big enough for the type
    // followed by the specified number of bytes for the shared memory
    // when ptr goes out of scope, it will destroy the object and deallocate memory
    // note that it is just a single allocation for the entire memory used,
    // instead of three it would have been otherwise

    // ptr behaves like a pointer to my_type
    // the joint memory details are hidden away
    std::cout << ptr->str << '\n';
    for (auto i : ptr->array)
        std::cout << i << ' ';
    std::cout << '\n';

    // if your type provides the joint copy constructor,
    // you can also clone it
    // this will only allocate enough memory as used by the original object
    // so you can, for example, use temporary_allocator with a large joint size
    // to create the object initially,
    // then clone it to get a buffer that fits
    // and destroy the original one
    auto ptr2 = memory::clone_joint(memory::default_allocator{}, *ptr);
    std::cout << ptr2->str << '\n';
}
