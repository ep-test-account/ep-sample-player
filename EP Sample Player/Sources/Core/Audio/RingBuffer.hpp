#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace EP {

/// Lock-free single-producer single-consumer (SPSC) ring buffer.
///
/// Safe for use between exactly one writer thread and one reader thread
/// without any locks or memory allocation. Suitable for real-time audio
/// and MIDI communication.
///
/// Template parameters:
///   T        - element type (must be trivially copyable)
///   Capacity - maximum number of elements (actual usable capacity is Capacity - 1)
///
/// This can indeed be better but it was kept simple for the sake of the test project!
template<typename T, size_t Capacity>
class RingBuffer
{
    static_assert(Capacity > 1, "RingBuffer capacity must be > 1");

public:
    RingBuffer()
        : m_read(0)
        , m_write(0) {}

    /// Push an item into the buffer (producer side).
    /// Returns true on success, false if the buffer is full.
    bool push(const T& item)
    {
        const size_t w = m_write.load(std::memory_order_relaxed);
        const size_t next = (w + 1) % Capacity;

        if (next == m_read.load(std::memory_order_acquire))
        {
            return false; // full
        }

        m_buffer[w] = item;
        m_write.store(next, std::memory_order_release);
        return true;
    }

    /// Pop an item from the buffer (consumer side).
    /// Returns true on success (item written to out), false if empty.
    bool pop(T& out)
    {
        const size_t r = m_read.load(std::memory_order_relaxed);

        if (r == m_write.load(std::memory_order_acquire))
        {
            return false; // empty
        }

        out = m_buffer[r];
        m_read.store((r + 1) % Capacity, std::memory_order_release);
        return true;
    }

private:
    std::atomic<size_t> m_read;
    std::atomic<size_t> m_write;
    std::array<T, Capacity> m_buffer;
};

} // namespace EP
