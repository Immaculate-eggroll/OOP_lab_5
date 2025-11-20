#include "MapMemoryResource.h"
#include <new>

MapMemoryResource::MapMemoryResource(std::size_t total_size)
    : buffer_(nullptr), size_(total_size), offset_(0)
{
    buffer_ = static_cast<std::byte*>(::operator new(total_size));
}

MapMemoryResource::~MapMemoryResource() {
    ::operator delete(buffer_);
}

std::uintptr_t MapMemoryResource::align_up(std::uintptr_t addr, std::size_t alignment) {
    std::uintptr_t mod = addr % alignment;
    return mod == 0 ? addr : addr + (alignment - mod);
}

void* MapMemoryResource::allocate_from_buffer(std::size_t bytes, std::size_t alignment) {
    std::uintptr_t base = reinterpret_cast<std::uintptr_t>(buffer_);
    std::uintptr_t aligned = align_up(base + offset_, alignment);
    std::size_t new_offset = aligned - base + bytes;

    if (new_offset > size_)
        throw std::bad_alloc();

    offset_ = new_offset;
    return reinterpret_cast<void*>(aligned);
}

void* MapMemoryResource::do_allocate(std::size_t bytes, std::size_t alignment) {
    std::lock_guard<std::mutex> guard(lock_);

    auto it = free_blocks_.lower_bound(bytes);
    if (it != free_blocks_.end() && !it->second.empty()) {
        void* ptr = it->second.back();
        it->second.pop_back();
        return ptr;
    }

    return allocate_from_buffer(bytes, alignment);
}

void MapMemoryResource::do_deallocate(void* ptr, std::size_t bytes, std::size_t) {
    std::lock_guard<std::mutex> guard(lock_);
    free_blocks_[bytes].push_back(ptr);
}

bool MapMemoryResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
    return this == &other;
}
