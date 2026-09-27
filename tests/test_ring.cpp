#include <ringbuffer/ring_buffer.hpp>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using ringbuffer::RingBuffer;

static int checks = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { \
    std::cerr << "FAIL at line " << __LINE__ << ": " #cond "\n"; std::exit(1); } } while (0)

int main() {
    using namespace std::chrono_literals;

    // --- basic single-threaded push/pop ---
    {
        RingBuffer rb(64);
        CHECK(rb.capacity() == 64);
        CHECK(rb.empty());
        const char* msg = "hello";
        CHECK(rb.push(msg, 6));
        CHECK(rb.available() == 6);
        char out[8] = {};
        CHECK(rb.pop(out, sizeof out) == 6);
        CHECK(std::string(out) == "hello");
        CHECK(rb.empty());
    }

    // --- wraparound across the buffer boundary ---
    {
        RingBuffer rb(8);
        for (int i = 0; i < 5; ++i) { int v = 7; rb.push(&v, sizeof v); }
        int v = 0;
        for (int i = 0; i < 5; ++i) { rb.pop(&v, sizeof v); CHECK(v == 7); }
        CHECK(rb.empty());
    }

    // --- reject oversized push ---
    {
        RingBuffer rb(4);
        int big[8] = {};
        CHECK(rb.push(big, sizeof big) == false);
    }

    // --- producer / consumer threads ---
    {
        RingBuffer rb(1024);
        const int total = 5000;
        const int chunk = sizeof(int);
        std::atomic<bool> done{false};

        std::thread producer([&] {
            for (int i = 0; i < total;) {
                if (rb.push(&i, chunk)) ++i;
            }
        });
        std::thread consumer([&] {
            int received = 0;
            int expect = 0;
            while (!done.load() || rb.available() > 0) {
                int v = -1;
                if (rb.pop(&v, chunk)) {
                    CHECK(v == expect);
                    ++expect;
                    ++received;
                }
            }
            CHECK(received == total);
        });
        producer.join();
        done = true;
        consumer.join();
    }

    std::cout << "ringbuffer: all " << checks << " checks passed\n";
    return 0;
}