# ringbuffer

A **lock-free single-producer / single-consumer (SPSC) ring buffer for C++17**.
Designed for real-time audio, logging pipelines and streaming between two threads.

## Highlights

- **Lock-free** — uses only `std::atomic` acquire/release ordering.
- **Exactly one writer + one reader** thread may push/pop concurrently.
- **Header-only**, no dependencies.
- Wraparound-safe: data can be read/written across the buffer boundary transparently.
- Capacity is rounded up to a power of two (mask-based indexing).

## Usage

```cpp
#include <ringbuffer/ring_buffer.hpp>

ringbuffer::RingBuffer rb(1024);
int sample = 42;
rb.push(&sample, sizeof(sample));        // producer

int out = 0;
std::size_t got = rb.pop(&out, sizeof(out));   // consumer
```

See `tests/test_ring.cpp` for a threaded producer/consumer example.

## Build & test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## License

MIT