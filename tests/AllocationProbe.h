#pragma once
#include <cstdlib>
#include <new>
// Thread-local probe observes C++ new/new[] on the calling audio test thread.
// This deliberately makes no claim about arbitrary third-party malloc calls.
namespace allocation_probe
{
inline thread_local bool enabled = false;
inline thread_local unsigned count = 0;
} // namespace allocation_probe
void *operator new(std::size_t size)
{
    if (allocation_probe::enabled)
        ++allocation_probe::count;
    if (auto *result = std::malloc(size ? size : 1))
        return result;
    throw std::bad_alloc();
}
void operator delete(void *pointer) noexcept
{
    std::free(pointer);
}
void operator delete(void *pointer, std::size_t) noexcept
{
    std::free(pointer);
}
void *operator new[](std::size_t size)
{
    return ::operator new(size);
}
void operator delete[](void *pointer) noexcept
{
    ::operator delete(pointer);
}
void operator delete[](void *pointer, std::size_t) noexcept
{
    ::operator delete(pointer);
}
