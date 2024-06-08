#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdlib>
#include <iostream>

class MemoryCounter {
public:
    static int allocationCount;
    static int deallocationCount;

    static void printCounts() {
        std::cout << "---------------------\nAllocations: " << allocationCount << std::endl;
        std::cout << "Deallocations: " << deallocationCount << std::endl;
    }
};

inline int MemoryCounter::allocationCount = 0;
inline int MemoryCounter::deallocationCount = 0;

void* operator new(size_t size) noexcept{
    MemoryCounter::allocationCount++;
    return malloc(size);
}

void operator delete(void* ptr) noexcept{
    MemoryCounter::deallocationCount++;
    free(ptr);
}

#endif // MEMORY_HPP
