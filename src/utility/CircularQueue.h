#ifndef CIRCULAR_QUEUE_HPP
#define CIRCULAR_QUEUE_HPP

#include <atomic>

template <typename Element, std::size_t size>
class CircularQueue
{
public:
    enum
    {
        Capacity = size + 1
    };

    CircularQueue() : _tail(0), _head(0)
    {
        _array = new Element[Capacity];
    }

    virtual ~CircularQueue() {
        delete[] _array;
    }

    bool push(Element &item)
    {
        auto current_tail = _tail.load(std::memory_order_relaxed);
        auto next_tail = increment(current_tail);
        if (next_tail != _head.load(std::memory_order_acquire))
        {
            _array[current_tail] = item;
            _tail.store(next_tail, std::memory_order_release);
            return true;
        }

        return false; // full queue
    }

    bool pop(Element &item)
    {
        const auto current_head = _head.load(std::memory_order_relaxed);
        if (current_head == _tail.load(std::memory_order_acquire))
            return false;

        item = _array[current_head];
        _head.store(increment(current_head), std::memory_order_release);
        return true;
    }

    bool wasEmpty() const
    {
        return _tail.load() == _head.load();
    }

    bool wasFull() const
    {
        const auto next_tail = increment(_tail.load());
        return (next_tail == _head.load());
    }

private:
    std::size_t increment(std::size_t idx) const
    {
        return (idx + 1) % Capacity;
    }

    std::atomic<std::size_t> _tail;
    std::atomic<std::size_t> _head;
    Element *_array;
};

#endif // CIRCULAR_QUEUE_HPP
