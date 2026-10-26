#include <array>
#include <atomic>
#include <functional>
#include <iostream>
#include <optional>
#include <thread>

#define MAGIC_WORK_NUMBER 100000000

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
        // Start this on its own cache line as well other wise it shares
        // a line with tailIdx on first 15 values
        alignas(
            std::hardware_destructive_interference_size) std::array<T, 128> buf;

        alignas(std::hardware_destructive_interference_size)
            const uint8_t capacity = 128;
};

// template <typename T, std::size_t N = 64>
template <typename T>
bool ring_buffer_push(ring_buffer<T> &rb, T ins, uint8_t &localHead,
                      int &refreshCounter) {
        // write the push logic

        // producer thread can do relaxed read to tail index
        // producer read head acquire
        // producer inserts data if room
        // producer does a release on edit to the tail index update

        // Check if full
        // should handle overflows fine
        uint8_t tailIdx = rb.tailIdx.load(std::memory_order_relaxed);

        // Update local first to see if actually full
        if (static_cast<uint8_t>(tailIdx - localHead) == rb.capacity - 1) {
                // update headIdx only when full since it is expensive to
                // to do. I.e. let it be stale no risk since tail will always be
                // "ahead" of the head. This is so we can reduce cache misses
                localHead = rb.headIdx.load(std::memory_order_acquire);
                refreshCounter++;
        }

        // Check again
        if (static_cast<uint8_t>(tailIdx - localHead) == rb.capacity - 1) {
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

// template <typename T, std::size_t N = 64>
template <typename T>
std::optional<T> ring_buffer_pop(ring_buffer<T> &rb, uint8_t &localTail,
                                 int &refreshCounter) {
        // write the pop logic

        // consumer thread can do relaxed read to head index
        // consumer read tail acquire
        // consumer consumes data if not empty
        // consumer does a release on edit to the head index update

        // Check if empty and update localTail
        auto headIdx = rb.headIdx.load(std::memory_order_relaxed);
        if (localTail == headIdx) {
                localTail = rb.tailIdx.load(std::memory_order_acquire);
                refreshCounter++;
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

void doSomeConsuming(ring_buffer<int> &rb, int &checker, int &refreshCounter) {
        uint8_t localTail = 0;
        int count = 0;

        while (count < MAGIC_WORK_NUMBER) {
                std::optional<int> val =
                    ring_buffer_pop(rb, localTail, refreshCounter);
                if (val.has_value()) {
                        // Track the amount of numbers read that == the expected
                        if (checker == count) {
                                checker++;
                        }
                        count++;
                }
        }
}

void doSomeProducing(ring_buffer<int> &rb, int &refreshCounter) {
        uint8_t localHead = 0;
        int count = 0;

        while (count < MAGIC_WORK_NUMBER) {
                bool res =
                    ring_buffer_push(rb, count, localHead, refreshCounter);

                if (res) {
                        count++;
                }
        }
}

void checkConsumeTracker(std::vector<int> vec) {
        // Just check to see if every value in order works
        int expectedCount = MAGIC_WORK_NUMBER;
        int count = 0;
        for (std::size_t i = 0; i < vec.size(); i++) {
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

        std::atomic<uint8_t> *headPtr = &rb.headIdx;
        std::atomic<uint8_t> *tailPtr = &rb.tailIdx;
        int *firstIntPtr = &rb.buf[0];

        std::uintptr_t headIntAddress =
            reinterpret_cast<std::uintptr_t>(headPtr);
        std::uintptr_t tailIntAddress =
            reinterpret_cast<std::uintptr_t>(tailPtr);
        std::uintptr_t firstIntAddress =
            reinterpret_cast<std::uintptr_t>(firstIntPtr);

        std::cout << "Head address as int " << headIntAddress << "\n";
        std::cout << "Tail address as int " << tailIntAddress << "\n";
        std::cout << "First Int address as int " << firstIntAddress << "\n";

        int checker = 0;
        for (int i = 0; i < 10; i++) {
                // Start thread work
                int refreshHead = 0;
                int refreshTail = 0;

                auto start = std::chrono::steady_clock::now();
                std::thread producer(doSomeProducing, std::ref(rb),
                                     std::ref(refreshHead));
                std::thread consumer(doSomeConsuming, std::ref(rb),
                                     std::ref(checker), std::ref(refreshTail));

                // Then Join
                std::cout << "Starting threads\n";
                producer.join();
                consumer.join();
                auto end = std::chrono::steady_clock::now();

                std::chrono::duration<double, std::milli> elapsed = end - start;
                std::cout << "Duration of work: " << elapsed.count() << "\n";
                // Does not block the two threads from running at the same time.
                // Need to join or program will crash. C++20 jthread solves this
                // using std::thread to better understand the things that
                // std::jthread solves

                std::cout << "Amount of head refreshes " << refreshHead << "\n";
                std::cout << "Amount of tail refreshes " << refreshTail << "\n";

                if (checker == MAGIC_WORK_NUMBER) {
                        std::cout << "Consumer Result is good.\n";
                }

                checker = 0;
                // Checking to see if ring buffer is working ;
                // std::cout << "Starting result test\n";
                // checkConsumeTracker(consumeTracker);
        }
}
