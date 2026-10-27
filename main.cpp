#include <array>
#include <atomic>
#include <functional>
#include <iostream>
#include <optional>
#include <thread>

#define MAGIC_WORK_NUMBER 100000000

// Decided to try Data-Orented Design
// Pure data and pure behavior

template <typename T, std::size_t N> struct ring_buffer {
        static_assert(N > 0 && (N & (N - 1)) == 0,
                      "N must be power of 2 smaller than size_t.");

        static_assert(N < (1ULL << 32), "N < (1ULL << 32)");

        // Might as well use size_t since cache lines are 64 bytes apart
        // both counters are on separate cache lines
        // Starts on cacheline as per hardware descripiton
        alignas(std::hardware_destructive_interference_size)
            std::atomic<size_t> headIdx = 0;

        // Starts on cacheline as per hardware descripiton
        alignas(std::hardware_destructive_interference_size)
            std::atomic<size_t> tailIdx = 0;

        // static constexpr std::size_t capacity = 2 << N;
        // Start this on its own cache line as well other wise it shares
        // a line with tailIdx on first 15 values
        alignas(
            std::hardware_destructive_interference_size) std::array<T, N> buf;

        alignas(std::hardware_destructive_interference_size)
            const size_t capacity = N;
};

// template <typename T, std::size_t N = 64>
template <typename T, std::size_t N>
bool ring_buffer_push(ring_buffer<T, N> &rb, T ins, size_t &localHead) {
        // write the push logic

        // producer thread can do relaxed read to tail index
        // producer read head acquire
        // producer inserts data if room
        // producer does a release on edit to the tail index update

        // Check if full
        // should handle overflows fine
        size_t tailIdx = rb.tailIdx.load(std::memory_order_relaxed);

        // Update local first to see if actually full
        if (static_cast<size_t>(tailIdx - localHead) == rb.capacity) {
                // update headIdx only when full since it is expensive to
                // to do. I.e. let it be stale no risk since tail will always be
                // "ahead" of the head. This is so we can reduce cache misses
                localHead = rb.headIdx.load(std::memory_order_acquire);
                // refreshCounter++;
        }

        // Check again
        if (static_cast<size_t>(tailIdx - localHead) == rb.capacity) {
                // update headIdx only when full since it is expensive to
                // to do. I.e. let it be stale no risk since tail will always be
                // "ahead" of the head. This is so we can reduce cache misses
                return false;
        }

        // Insert value at tail
        rb.buf[tailIdx & (rb.capacity - 1)] = ins;

        // Move tail
        rb.tailIdx.store(tailIdx + 1, std::memory_order_release);

        return true;
};

template <typename T, std::size_t N>
std::optional<T> ring_buffer_pop(ring_buffer<T, N> &rb, size_t &localTail) {
        // write the pop logic

        // consumer thread can do relaxed read to head index
        // consumer read tail acquire
        // consumer consumes data if not empty
        // consumer does a release on edit to the head index update

        // Check if empty and update localTail
        auto headIdx = rb.headIdx.load(std::memory_order_relaxed);
        if (localTail == headIdx) {
                localTail = rb.tailIdx.load(std::memory_order_acquire);
        }

        // If truly empty then return
        if (localTail == headIdx) {
                // Same as push
                return std::nullopt;
        }

        T val = rb.buf[headIdx & (rb.capacity - 1)];

        rb.headIdx.store(headIdx + 1, std::memory_order_release);

        return val;
};

template <typename T, std::size_t N>
void doSomeConsuming(ring_buffer<T, N> &rb, int &checker) {
        size_t localTail = rb.tailIdx.load(std::memory_order_acquire);
        int count = 0;

        while (count < MAGIC_WORK_NUMBER) {
                std::optional<int> val = ring_buffer_pop(rb, localTail);
                if (val.has_value()) {
                        // Track the amount of numbers read that == the expected
                        if (checker == val.value()) {
                                checker++;
                        }
                        count++;
                }
        }
}

template <typename T, std::size_t N>
void doSomeProducing(ring_buffer<T, N> &rb) {
        size_t localHead = rb.headIdx.load(std::memory_order_acquire);
        int count = 0;

        while (count < MAGIC_WORK_NUMBER) {
                bool res = ring_buffer_push(rb, count, localHead);

                if (res) {
                        count++;
                }
        }
}

int main() {
        ring_buffer<int, 1ULL << 7> rb;

        alignas(std::hardware_destructive_interference_size) int checker = 0;
        for (int i = 0; i < 10; i++) {
                // Start thread work
                auto start = std::chrono::steady_clock::now();
                // Unsigned Long Long 1 shifted left 63 times, 2^63
                std::thread producer(doSomeProducing<int, 1ULL << 7>,
                                     std::ref(rb));
                std::thread consumer(doSomeConsuming<int, 1ULL << 7>,
                                     std::ref(rb), std::ref(checker));

                // Then Join
                producer.join();
                consumer.join();
                auto end = std::chrono::steady_clock::now();

                std::chrono::duration<double, std::milli> elapsed = end - start;
                std::cout << "Duration of work: " << elapsed.count() << "\n";

                if (checker == MAGIC_WORK_NUMBER) {
                        std::cout << "Consumer Result is good.\n";
                }

                checker = 0;

                // Does not block the two threads from running at the same time.
                // Need to join or program will crash. C++20 jthread solves this
                // using std::thread to better understand the things that
                // std::jthread solves
        }
}
