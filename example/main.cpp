#include "../pool.hpp"

using namespace foonathan::memory;

int main()

{
    memory_pool<> pool(16, 1024);
}
