#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <QtCore>

#include <stdlib.h>

template <class T, int size>
class RingBuffer
{
public:
    RingBuffer() : pos(0) {};

    const T& at(int i) {
        Q_ASSERT(i < size);
        return ring_buffer.at(i);
    }

    const std::array<T, size>& buffer() {
        return ring_buffer;
    }

    void append(const T& value) {
        ring_buffer[pos] = value;
        pos = (pos + 1) % size;
    }

private:
    int pos;
    std::array<T, size> ring_buffer;
};

#endif // RINGBUFFER_H
