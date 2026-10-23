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
template <typename T> struct ring_buffer {
        // static constexpr std::size_t capacity = 2 << N;
        std::array<T, 256> buf;
        const uint16_t capacity = 256;

        // Using uint8_t just for size constraint
        std::uint8_t headIdx = 0;
        std::uint8_t tailIdx = 0;
};

template <typename T, std::size_t N = 64>
void ring_buffer_push(ring_buffer<T> &buf, T ins) {
        // write the push logic

        // Check if full
        // What if u8 buffer overflows?
        //
}

template <typename T, std::size_t N = 64>
T ring_buffer_pop(ring_buffer<T> &buf) {
        // write the pop logic

        // Check if empty
        if (buf->tailIdx == buf->headIdx) {
                return;
        }
}
