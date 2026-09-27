#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

// ringbuffer - a lock-free single-producer / single-consumer ring buffer for C++17.
// One writer thread and one reader thread may use it concurrently without locks.

#include <atomic>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace ringbuffer {

class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity)
        : mask_(nextPowerOfTwo(capacity) - 1),
          data_(nextPowerOfTwo(capacity)) {
        if (capacity == 0) throw std::invalid_argument("ringbuffer: capacity must be > 0");
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    std::size_t capacity() const noexcept { return data_.size(); }

    // Writer side.
    bool push(const void* src, std::size_t bytes) noexcept {
        if (bytes == 0 || bytes > data_.size()) return false;
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        if (bytes > data_.size() - (head - tail)) return false;   // not enough space
        std::size_t end = head + bytes;
        std::size_t where = head & mask_;
        std::size_t first = std::min(bytes, data_.size() - where);
        std::memcpy(data_.data() + where, src, first);
        if (first < bytes)
            std::memcpy(data_.data(), static_cast<const char*>(src) + first, bytes - first);
        head_.store(end, std::memory_order_release);
        return true;
    }

    // Reader side. Returns number of bytes actually popped (0 if empty).
    std::size_t pop(void* dest, std::size_t bytes) noexcept {
        if (bytes == 0) return 0;
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t avail = head - tail;
        if (avail == 0) return 0;
        std::size_t take = std::min(bytes, avail);
        std::size_t end = tail + take;
        std::size_t where = tail & mask_;
        std::size_t first = std::min(take, data_.size() - where);
        if (dest) std::memcpy(dest, data_.data() + where, first);
        if (first < take && dest)
            std::memcpy(static_cast<char*>(dest) + first, data_.data(), take - first);
        tail_.store(end, std::memory_order_release);
        return take;
    }

    std::size_t available() const noexcept {
        return head_.load(std::memory_order_acquire) - tail_.load(std::memory_order_acquire);
    }

    bool empty() const noexcept { return available() == 0; }
    bool full() const noexcept { return available() == data_.size(); }

private:
    static std::size_t nextPowerOfTwo(std::size_t n) {
        if (n <= 1) return 2;
        std::size_t v = 1;
        while (v < n) v <<= 1;
        return v;
    }

    std::size_t mask_;
    std::vector<char> data_;
    std::atomic<std::size_t> head_{0};   // producer position
    std::atomic<std::size_t> tail_{0};   // consumer position
};

} // namespace ringbuffer

#endif // RINGBUFFER_HPP