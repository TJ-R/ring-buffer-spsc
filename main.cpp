#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <thread>

#define MAGIC_WORK_NUMBER 100000000

template <typename T> struct slot {
        alignas(T) std::byte storage[sizeof(T)];
};

template <typename T> class ring_buffer {
      public:
        ring_buffer(std::size_t capacity) {
                if (!(capacity > 0 && (capacity & (capacity - 1)) == 0)) {
                        throw std::invalid_argument(
                            "Capacity is not a power of 2");
                }

                if (capacity > SIZE_MAX / sizeof(slot<T>)) {
                        throw std::length_error(
                            "Capacity * sizeof(T) will overflow size_t");
                }

                m_capacity = capacity;
                m_mask = capacity - 1;

                // Choose buffer_alignment to be size of T if greater than 64
                // 64 is the most common cache line size
                m_buf = static_cast<slot<T> *>(::operator new(
                    sizeof(T) * capacity, std::align_val_t{buffer_alignment}));
        }

        // Freeing the buffer
        ~ring_buffer() {
                size_t lHead = headIdx.load(std::memory_order_relaxed);
                size_t lTail = tailIdx.load(std::memory_order_relaxed);

                // Go through every slot in buffer and destory every remaining T
                for (size_t i = lHead; i < lTail; i++) {
                        // I am currnently assuming it will try to destruct
                        // the slot of any T here and not fail if there
                        // is not
                        slot<T> &s = m_buf[i & m_mask];
                        T *ptr = std::launder(reinterpret_cast<T *>(s.storage));
                        ptr->~T();
                }

                ::operator delete(m_buf, std::align_val_t{buffer_alignment});
        }

        std::size_t getCapacity() {
                return m_capacity;
        }

        std::size_t getMask() {
                return m_mask;
        }

        T getValue(size_t idx) {
                // Copy value out of slot
                slot<T> &s = m_buf[idx & m_mask];
                T *ptr = std::launder(reinterpret_cast<T *>(s.storage));

                T val = *ptr;

                // Free slot
                ptr->~T();

                return val;
        }

        void insertVal(size_t idx, T val) {
                // Gets address for slot
                slot<T> &s = m_buf[idx & m_mask];

                void *addr = static_cast<void *>(s.storage);

                // Insert new T into slot
                new (addr) T(val);
        }

        alignas(64) std::atomic<size_t> headIdx{0};
        alignas(64) std::atomic<size_t> tailIdx{0};

      private:
        static constexpr std::size_t buffer_alignment = alignof(T) > 64
                                                            ? alignof(T)
                                                            : 64;

        std::size_t m_capacity;
        std::size_t m_mask;
        slot<T> *m_buf;
};

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
        if (static_cast<size_t>(tailIdx - localHead) == rb.getCapacity()) {
                // update headIdx only when full since it is expensive
                // to to do. I.e. let it be stale no risk since tail
                // will always be "ahead" of the head. This is so we can
                // reduce cache misses
                localHead = rb.headIdx.load(std::memory_order_acquire);
                // refreshCounter++;
        }

        // Check again
        if (static_cast<size_t>(tailIdx - localHead) == rb.getCapacity()) {
                // update headIdx only when full since it is expensive
                // to to do. I.e. let it be stale no risk since tail
                // will always be "ahead" of the head. This is so we can
                // reduce cache misses
                return false;
        }

        // Insert value at tail
        rb.insertVal(tailIdx, ins);

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
                std::optional<T> val = ring_buffer_pop(rb, localTail);
                if (val.has_value()) {
                        assert(val.value() == count);
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
        ring_buffer<int> rb(1ULL << 32);

        for (int i = 0; i < 100; i++) {
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
