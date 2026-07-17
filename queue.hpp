#include <vector>
#include <atomic>
#include <functional>
#include <optional>

class LockFreeJobQueue {
private:
    struct Cell {
        std::atomic<size_t> sequence;
        std::function<void()> job;
    };

    std::vector<Cell> buffer;
    size_t buffer_mask;
    
    // Align to cache lines 
    alignas(64) std::atomic<size_t> enqueue_pos;
    alignas(64) std::atomic<size_t> dequeue_pos;

public:
    // Size must be a power of 2 for fast bitwise modulo
    explicit LockFreeJobQueue(size_t capacity) : buffer(capacity), enqueue_pos(0), dequeue_pos(0) {
        buffer_mask = capacity - 1;
        for (size_t i = 0; i < capacity; ++i) {
            buffer[i].sequence.store(i, std::memory_order_relaxed);
        }
    }
    bool try_push(std::function<void()> job) {
        Cell* cell;
        size_t pos = enqueue_pos.load(std::memory_order_relaxed);
        
        while (true) {
            cell = &buffer[pos & buffer_mask];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            
            if (diff == 0) {
                if (enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return false;
            } else {
                pos = enqueue_pos.load(std::memory_order_relaxed);
            }
        }

        cell->job = std::move(job);
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }
    std::optional<std::function<void()>> try_pop() {
        Cell* cell;
        size_t pos = dequeue_pos.load(std::memory_order_relaxed);
        
        while (true) {
            cell = &buffer[pos & buffer_mask];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            
            if (diff == 0) {
                if (dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return std::nullopt;
            } else {
                pos = dequeue_pos.load(std::memory_order_relaxed);
            }
        }
        std::function<void()> job = std::move(cell->job);
        cell->sequence.store(pos + buffer_mask + 1, std::memory_order_release);
        return job;
    }
};