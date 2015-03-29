#include "block_list.hpp"

#include <cassert>
#include <new>

using namespace foonathan::memory;
using namespace detail;

struct block_list_impl::node
{
    node *prev;
    std::size_t size;
    
    node(node *prev, std::size_t size) noexcept
    : prev(prev), size(size) {}
};

std::size_t block_list_impl::impl_offset()
{
    return sizeof(node);
}

std::size_t block_list_impl::push(void* &memory, std::size_t size) noexcept
{
    auto ptr = ::new(memory) node(head_, size);
    head_ = ptr;
    memory = static_cast<char*>(memory) + sizeof(node);
    return sizeof(node);
}

block_info block_list_impl::push(block_list_impl &other) noexcept
{
    assert(other.head_ && "stack underflow");
    auto top = other.head_;
    other.head_ = top->prev;
    top->prev = head_;
    head_ = top;
    return {top, top->size - sizeof(node)};
}

block_info block_list_impl::pop() noexcept
{
    assert(head_ && "stack underflow");
    auto top = head_;
    head_ = top->prev;
    return {top, top->size};
}
