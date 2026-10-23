#include <atomic>
#include <iostream>

int main() {
        std::atomic<int> x{0};
        x++;

        // x = x + 1;
}

// Decided to try Data-Orented Design
// Pure data and pure behavior

template <typename T, std::size_t N = 64> struct ring_buffer {
        std::array<T, N> buf;

        int headIdx = 0;
        int tailIdx = 0;
};

template <typename T, std::size_t N = 64>
void ring_buffer_push(ring_buffer<T, N> &buf, T ins) {
        // write the push logic
}

template <typename T, std::size_t N = 64>
T ring_buffer_pop(ring_buffer<T, N> &buf) {
        // write the pop logic
}
