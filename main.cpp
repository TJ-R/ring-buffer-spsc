#include <array>
#include <atomic>
#include <functional>
#include <iostream>
#include <optional>
#include <thread>

// Decided to try Data-Orented Design
// Pure data and pure behavior

// template <typename T, std::size_t N = 4> struct ring_buffer {
// Want to generalize this so capacity is a power of two only
// but unsure how to make the idx maximum size match that power of two.
template <typename T> struct ring_buffer {
        // Using uint8_t just for size constraint
        //
        // both counters are on separate cache lines

        // Starts on cacheline as per hardware descripiton
        alignas(std::hardware_destructive_interference_size)
            std::atomic<uint8_t> headIdx = 0;

        // Starts on cacheline as per hardware descripiton
        alignas(std::hardware_destructive_interference_size)
            std::atomic<uint8_t> tailIdx = 0;

        // static constexpr std::size_t capacity = 2 << N;
        std::array<T, 128> buf;
        const uint8_t capacity = 128;
};

// template <typename T, std::size_t N = 64>
template <typename T>
bool ring_buffer_push(ring_buffer<T> &rb, T ins, uint8_t &localHead) {
        // write the push logic

        // producer thread can do relaxed read to tail index
        // producer read head acquire
        // producer inserts data if room
        // producer does a release on edit to the tail index update

        // Check if full
        // should handle overflows fine
        uint8_t tailIdx = rb.tailIdx.load(std::memory_order_relaxed);

        if (static_cast<uint8_t>(tailIdx - localHead) == rb.capacity - 1) {
                // update headIdx only when full since it is expensive to
                // to do. I.e. let it be stale no risk since tail will always be
                // "ahead" of the head. This is so we can reduce cache misses
                localHead = rb.headIdx.load(std::memory_order_acquire);
                return false;
        }

        // Insert value at tail
        rb.buf[tailIdx & (rb.capacity - 1)] = ins;

        // Move tail
        rb.tailIdx.store(tailIdx + 1, std::memory_order_release);

        return true;
};

// template <typename T, std::size_t N = 64>
template <typename T>
std::optional<T> ring_buffer_pop(ring_buffer<T> &rb, uint8_t &localTail) {
        // write the pop logic

        // consumer thread can do relaxed read to head index
        // consumer read tail acquire
        // consumer consumes data if not empty
        // consumer does a release on edit to the head index update

        // Check if empty
        if (localTail == rb.headIdx.load(std::memory_order_relaxed)) {
                // Same as push
                localTail = rb.tailIdx.load(std::memory_order_acquire);
                return std::nullopt;
        }

        T val = rb.buf[rb.headIdx.load(std::memory_order_relaxed) &
                       (rb.capacity - 1)];

        rb.headIdx.store(rb.headIdx.load(std::memory_order_relaxed) + 1,
                         std::memory_order_release);

        return val;
};

void doSomeConsuming(ring_buffer<int> &rb, std::vector<int> &tracking) {
        uint8_t localTail = 0;
        int count = 0;

        while (count < 100000000) {
                std::optional<int> val = ring_buffer_pop(rb, localTail);
                if (val.has_value()) {
                        tracking.push_back(val.value());
                        count++;
                }
        }
}

void doSomeProducing(ring_buffer<int> &rb) {
        uint8_t localHead = 0;
        int count = 0;

        while (count < 100000000) {
                bool res = ring_buffer_push(rb, count, localHead);

                if (res) {
                        count++;
                }
        }
}

void checkConsumeTracker(std::vector<int> vec) {
        // Just check to see if every value in order works
        int expectedCount = 100000000;
        int count = 0;
        for (int i = 0; i < vec.size(); i++) {
                if (i != vec[i]) {
                        std::cout << "--------------------\n";
                        std::cout << "Values did not match\n";
                        std::cout << "Expected " << i << "\n";
                        std::cout << "Found " << vec[i] << "\n";
                        std::cout << "--------------------\n";
                }

                count++;
        }

        if (count != expectedCount) {
                std::cout
                    << "Amount of values found did not match expectations\n";
                std::cout << "Expected " << expectedCount << "\n"
                          << "Found " << count << "\n";
        }
}

int main() {
        ring_buffer<int> rb;

        std::vector<int> consumeTracker;
        // Start thread work
        std::thread producer(doSomeProducing, std::ref(rb));
        std::thread consumer(doSomeConsuming, std::ref(rb),
                             std::ref(consumeTracker));

        // Then Join
        std::cout << "Starting threads\n";
        producer.join();
        consumer.join();

        // Does not block the two threads from running at the same time.
        // Need to join or program will crash. C++20 jthread solves this
        // using std::thread to better understand the things that
        // std::jthread solves

        // Checking to see if ring buffer is working ;
        std::cout << "Starting result test\n";
        checkConsumeTracker(consumeTracker);
}

// TODO WRITE SOME TESTING CODE TO MAKE SURE THIS WORKS PROPERLY
// NO USING COUT
//
// ALSO WRITE THE ATOMIC READS TO A LOCAL INSTEAD OF CALLING MULTIPLE ATOMIC
// READS
