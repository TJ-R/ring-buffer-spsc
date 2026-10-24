#include <atomic>
#include <iostream>

int main() {
        std::atomic<int> x{0};
        x++;

        // x = x + 1;
}

// Decided to try Data-Orented Design
// Pure data and pure behavior

// template <typename T, std::size_t N = 4> struct ring_buffer {
// Want to generalize this so capacity is a power of two only
// but unsure how to make the idx maximum size match that power of two.
template <typename T> struct ring_buffer {
        // static constexpr std::size_t capacity = 2 << N;
        std::array<T, 128> buf;
        const uint8_t capacity = 128;

        // Using uint8_t just for size constraint
        std::atomic<uint8_t> headIdx = 0;
        std::atomic<uint8_t> tailIdx = 0;
};

// template <typename T, std::size_t N = 64>
template <typename T> void ring_buffer_push(ring_buffer<T> &buf, T ins) {
        // write the push logic

        // producer thread can do relaxed read to tail index
        // producer read head acquire
        // producer inserts data if room
        // producer does a release on edit to the tail index update

        // Check if full
        // should handle overflows fine

        if (buf->tailIdx.load(std::memory_order_relaxed) -
                buf->headIdx.load(std::memory_order_acquire) ==
            buf->capacity) {
                return;
        }
};

// template <typename T, std::size_t N = 64>
template <typename T> T ring_buffer_pop(ring_buffer<T> &buf) {
        // write the pop logic

        // consumer thread can do relaxed read to head index
        // consumer read tail acquire
        // consumer consumes data if not empty
        // consumer does a release on edit to the head index update

        // Check if empty
        if (buf->tailIdx.load(std::memory_order_acquire) ==
            buf->headIdx.load(std::memory_order_relaxed)) {
                return;
        }
};
