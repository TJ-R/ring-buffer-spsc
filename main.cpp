#include <array>
#include <atomic>
#include <functional>
#include <iostream>
#include <optional>
#include <thread>

#define MAGIC_WORK_NUMBER 100000000

template <typename T> class ring_buffer {

      public:
        ring_buffer(std::size_t capacity) {
                if (!(capacity > 0 && (capacity & (capacity - 1)) == 0)) {
                        throw std::invalid_argument(
                            "Capacity is not a multiple of zero");
                }

                if (capacity > (1ULL << 63)) {
                        throw std::invalid_argument(
                            "Capacity must 2^63 or lower");
                }

                m_capacity = capacity;
                m_mask = capacity - 1;
                void *ptr = std::aligned_alloc(64, sizeof(T) * capacity);

                if (!ptr) {
                        throw std::bad_alloc();
                }

                m_buf = static_cast<T *>(ptr);
        }

        // Freeing the buffer
        ~ring_buffer() noexcept {
                if (m_buf) {
                        std::free(m_buf);
                }
        }

        T getValue(size_t idx) {
                return m_buf[idx & m_mask];
        }

        alignas(std::hardware_destructive_interference_size)
            std::atomic<size_t> headIdx;

        alignas(std::hardware_destructive_interference_size)
            std::atomic<size_t> tailIdx;

        alignas(std::hardware_destructive_interference_size) std::size_t
            m_capacity;

        alignas(std::hardware_destructive_interference_size) std::size_t m_mask;

        T *m_buf;
};

// template <typename T>
// struct alignas(std::hardware_destructive_interference_size) producer {
//         size_t localHead = 0;
// };
//
// template <typename T>
// struct alignas(std::hardware_destructive_interference_size) consumer {};

template <typename T>
bool ring_buffer_push(ring_buffer<T> &rb, T ins, size_t &localHead) {
        // write the push logic

        // producer thread can do relaxed read to tail index
        // producer read head acquire
        // producer inserts data if room
        // producer does a release on edit to the tail index update

        // Check if full
        // should handle overflows fine
        size_t tailIdx = rb.tailIdx.load(std::memory_order_relaxed);

        // Update local first to see if actually full
        if (static_cast<size_t>(tailIdx - localHead) == rb.m_capacity) {
                // update headIdx only when full since it is expensive
                // to to do. I.e. let it be stale no risk since tail
                // will always be "ahead" of the head. This is so we can
                // reduce cache misses
                localHead = rb.headIdx.load(std::memory_order_acquire);
                // refreshCounter++;
        }

        // Check again
        if (static_cast<size_t>(tailIdx - localHead) == rb.m_capacity) {
                // update headIdx only when full since it is expensive
                // to to do. I.e. let it be stale no risk since tail
                // will always be "ahead" of the head. This is so we can
                // reduce cache misses
                return false;
        }

        // Insert value at tail
        rb.m_buf[tailIdx & rb.m_mask] = ins;

        // Move tail
        rb.tailIdx.store(tailIdx + 1, std::memory_order_release);

        return true;
};

template <typename T>
std::optional<T> ring_buffer_pop(ring_buffer<T> &rb, size_t &localTail) {
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

        T val = rb.getValue(headIdx);

        rb.headIdx.store(headIdx + 1, std::memory_order_release);

        return val;
};

template <typename T> void doSomeConsuming(ring_buffer<T> &rb) {
        size_t localTail = rb.tailIdx.load(std::memory_order_acquire);
        int count = 0;

        while (count < MAGIC_WORK_NUMBER) {
                std::optional<int> val = ring_buffer_pop(rb, localTail);
                if (val.has_value()) {
                        // Track the amount of numbers read that == the
                        // expected
                        count++;
                }
        }
}

template <typename T> void doSomeProducing(ring_buffer<T> &rb) {
        size_t localHead = rb.headIdx.load(std::memory_order_acquire);
        int count = 0;

        while (count < MAGIC_WORK_NUMBER) {
                bool res = ring_buffer_push(rb, count, localHead);

                if (res) {
                        count++;
                }
        }
}

// testing with super small buffer size
int main() {
        ring_buffer<int> rb(1ULL << 10);

        for (int i = 0; i < 10; i++) {
                // Start thread work
                auto start = std::chrono::steady_clock::now();
                // Unsigned Long Long 1 shifted left 63 times, 2^63
                std::thread producer(doSomeProducing<int>, std::ref(rb));
                std::thread consumer(doSomeConsuming<int>, std::ref(rb));

                // Then Join
                producer.join();
                consumer.join();
                auto end = std::chrono::steady_clock::now();

                std::chrono::duration<double, std::milli> elapsed = end - start;
                std::cout << "Duration of work: " << elapsed.count() << "\n";

                // Does not block the two threads from running at the
                // same time. Need to join or program will crash. C++20
                // jthread solves this using std::thread to better
                // understand the things that std::jthread solves
        }
}
