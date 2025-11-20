#pragma once
#include <memory_resource>
#include <map>
#include <vector>
#include <mutex>
#include <cstddef>
#include <cstdint>

class MapMemoryResource : public std::pmr::memory_resource {
public:
    explicit MapMemoryResource(std::size_t total_size);
    ~MapMemoryResource();

    MapMemoryResource(const MapMemoryResource&) = delete;
    MapMemoryResource& operator=(const MapMemoryResource&) = delete;

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override;
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

private:
    void* allocate_from_buffer(std::size_t bytes, std::size_t alignment);
    static std::uintptr_t align_up(std::uintptr_t addr, std::size_t alignment);

    std::byte* buffer_;
    std::size_t size_;
    std::size_t offset_;
    std::map<std::size_t, std::vector<void*>> free_blocks_;
    std::mutex lock_;
};
