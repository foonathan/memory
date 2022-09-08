#include <cstddef>
#include <tuple>

#if FORWARD_LIST_CONTAINER
#include <forward_list>
#endif
#if LIST_CONTAINER
#include <list>
#endif
#if MAP_CONTAINER || MULTIMAP_CONTAINER
#include <map>
#endif
#if SHARED_PTR_STATELESS_CONTAINER || SHARED_PTR_STATEFUL_CONTAINER
#include <memory>
#endif
#if SET_CONTAINER || MULTISET_CONTAINER
#include <set>
#endif
#if UNORDERED_MAP_CONTAINER || UNORDERED_MULTIMAP_CONTAINER
#include <unordered_map>
#endif
#if UNORDERED_SET_CONTAINER || UNORDERED_MULTISET_CONTAINER
#include <unordered_set>
#endif

// This will fail to compile when is_node_size is true, which will
// cause the compiler to print this type with the calculated numbers
// in corresponding parameters.
template<size_t type_align, size_t node_size, bool is_node_size=false>
struct node_size_of
{
    static_assert(!is_node_size, "Expected to fail");
};

struct empty_state {};

template<typename T, typename U>
struct is_same
{
    static constexpr bool value = false;
};

template<typename T>
struct is_same<T,T>
{
    static constexpr bool value = true;
};

// This is a partially implemented allocator type, whose whole purpose
// is to be derived from node_size_of to cause a compiler error when
// this allocator is rebound to the node type.
template<typename T, typename State = empty_state, bool SubtractTSize = true, typename InitialType = T>
struct debug_allocator :
    public node_size_of<alignof(InitialType),
			sizeof(T) - (SubtractTSize ? sizeof(InitialType) : 0),
			!is_same<InitialType,T>::value && (sizeof(InitialType) != sizeof(T))>,
    private State
{
    template<typename U>
    struct rebind
    {
        using other = debug_allocator<U, State, SubtractTSize, InitialType>;
    };

    using value_type = T;

    T* allocate(size_t);
    void deallocate(T*, size_t);
};

// Dummy hash implementation for containers that need it
struct dummy_hash
{
    // note: not noexcept! this leads to a cached hash value
    template <typename T>
    std::size_t operator()(const T&) const
    {
        // quality doesn't matter
        return 0;
    }
};

// Functions to use the debug_allocator for the specified container
// and containee type.  We use the preprocessor to select which one to
// compile to reduce the time this takes.

#if FORWARD_LIST_CONTAINER
template<typename T>
int test_container()
{
    std::forward_list<T, debug_allocator<T>> list = {T()};
    return 0;
}
#endif // FORWARD_LIST_CONTAINER

#if LIST_CONTAINER
template<typename T>
int test_container()
{
    std::list<T, debug_allocator<T>> list = {T()};
    return 0;
}
#endif // LIST_CONTAINER

#if SET_CONTAINER
template<typename T>
int test_container()
{
    std::set<T, std::less<T>, debug_allocator<T>> set = {T()};
    return 0;
}
#endif // SET_CONTAINER

#if MULTISET_CONTAINER
template<typename T>
int test_container()
{
    std::multiset<T, std::less<T>, debug_allocator<T>> set = {T()};
    return 0;
}
#endif // MULTISET_CONTAINER

#if UNORDERED_SET_CONTAINER
template<typename T>
int test_container()
{
    std::unordered_set<T, dummy_hash, std::equal_to<T>, debug_allocator<T>> set = {T()};
    return 0;
}
#endif // UNORDERED_SET_CONTAINER

#if UNORDERED_MULTISET_CONTAINER
template<typename T>
int test_container()
{
    std::unordered_multiset<T, dummy_hash, std::equal_to<T>, debug_allocator<T>> set = {T()};
    return 0;
}
#endif // UNORDERED_MULTISET_CONTAINER

#if MAP_CONTAINER
template<typename T>
int test_container()
{
    using type = std::pair<const T, T>;
    std::map<T, T, std::less<T>, debug_allocator<type>> map = {{T(),T()}};
    return 0;
}
#endif // MAP_CONTAINER

#if MULTIMAP_CONTAINER
template<typename T>
int test_container()
{
    using type = std::pair<const T, T>;
    std::multimap<T, T, std::less<T>, debug_allocator<type>> map = {{T(),T()}};
    return 0;
}
#endif // MULTIMAP_CONTAINER

#if UNORDERED_MAP_CONTAINER
template<typename T>
int test_container()
{
    using type = std::pair<const T, T>;
    std::unordered_map<T, T, dummy_hash, std::equal_to<T>, debug_allocator<type>> map = {{T(),T()}};
    return 0;
}
#endif // UNORDERED_MAP_CONTAINER

#if UNORDERED_MULTIMAP_CONTAINER
template<typename T>
int test_container()
{
    using type = std::pair<const T, T>;
    std::unordered_multimap<T, T, dummy_hash, std::equal_to<T>, debug_allocator<type>> map = {{T(),T()}};
    return 0;
}
#endif // UNORDERED_MULTIMAP_CONTAINER

#if SHARED_PTR_STATELESS_CONTAINER
template<typename T>
int test_container()
{
    auto ptr = std::allocate_shared<T>(debug_allocator<T, empty_state, false>());
    return 0;
}
#endif // SHARED_PTR_STATELESS_CONTAINER

#if SHARED_PTR_STATEFUL_CONTAINER
template<typename T>
int test_container()
{
    struct allocator_reference_payload
    {
	void* ptr;
    };
    
    auto ptr = std::allocate_shared<T>(debug_allocator<T, allocator_reference_payload, false>());
    return 0;
}
#endif // SHARED_PTR_STATEFUL_CONTAINER

// Workaround for environments that have issues passing in type names with spaces
using LONG_LONG = long long;
using LONG_DOUBLE = long double;

#ifdef TEST_TYPES
template<typename... Types>
int test_all(std::tuple<Types...>)
{
    int dummy[] = {(test_container<Types>())...};
    return 0;
}

int foo = test_all(std::tuple<TEST_TYPES>());
#endif

#ifdef TEST_TYPE
int foo = test_container<TEST_TYPE>();
#endif
