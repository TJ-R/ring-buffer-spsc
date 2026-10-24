#include <array>
#include <atomic>
#include <thread>

// Decided to try Data-Orented Design
// Pure data and pure behavior

// template <typename T, std::size_t N = 4> struct ring_buffer {
// Want to generalize this so capacity is a power of two only
// but unsure how to make the idx maximum size match that power of two.
template <typename T> struct ring_buffer {
        // Using uint8_t just for size constraint
        // padding is make sure the indexs are on separate cache lines
        // unsure if it makes sense to also do padding between tailIdx and buf
        std::atomic<uint8_t> headIdx = 0;
        char padding[63];
        std::atomic<uint8_t> tailIdx = 0;
        char padding2[63];

        // static constexpr std::size_t capacity = 2 << N;
        std::array<T, 128> buf;
        const uint8_t capacity = 128;
};

// template <typename T, std::size_t N = 64>
template <typename T>
void ring_buffer_push(ring_buffer<T> &rb, T ins, uint8_t &localHead) {
        // write the push logic

        // producer thread can do relaxed read to tail index
        // producer read head acquire
        // producer inserts data if room
        // producer does a release on edit to the tail index update

        // Check if full
        // should handle overflows fine

        if (rb->tailIdx.load(std::memory_order_relaxed) - localHead ==
            rb->capacity) {
                // update headIdx only when full since it is expensive to
                // to do. I.e. let it be stale no risk since tail will always be
                // "ahead" of the head. This is so we can reduce cache misses
                localHead = rb->headIdx.load(std::memory_order_acquire);
                return;
        }

        // Insert value at tail
        rb->buf[rb->tailIdx.load(std::memory_order_relaxed) & rb->capacity] =
            ins;

        // Move tail
        rb->tailIdx.store(rb->tailIdx.load(std::memory_order_relaxed) + 1,
                          std::memory_order_release);

        return;
};

// template <typename T, std::size_t N = 64>
template <typename T>
T ring_buffer_pop(ring_buffer<T> &rb, uint8_t &localTail) {
        // write the pop logic

        // consumer thread can do relaxed read to head index
        // consumer read tail acquire
        // consumer consumes data if not empty
        // consumer does a release on edit to the head index update

        // Check if empty
        if (localTail == rb->headIdx.load(std::memory_order_relaxed)) {
                // Same as push
                localTail = rb->tailIdx.load(std::memory_order_acquire);
                return;
        }

        T val =
            rb->buf[rb->headIdx.load(std::memory_order_relaxed) & rb->capacity];

        rb->headIdx.store(rb->headIdx.load(std::memory_order_relaxed) + 1,
                          std::memory_order_release);

        return val;
};

int main() {
        std::thread t1;
        std::thread t2;

        // x = x + 1;
}
