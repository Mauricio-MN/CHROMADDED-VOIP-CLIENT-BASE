#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <array>
#include <mutex>
#include <semaphore.h>
#include <vector>

template <typename T>
class CircularBuffer {
public:
    CircularBuffer();
    CircularBuffer(size_t size);
    void Push(const std::vector<T>& elements);
    void Push(const T* elements, size_t count);
    void Push(T element);
    std::vector<T> Pop(size_t count);
    size_t Size() const;
    size_t Capacity() const;

private:
    std::vector<T> buffer_;
    size_t head_ = 0;
    size_t tail_ = 0;
    mutable std::mutex mutex_;
    sem_t sem_;
};

#include "CircularBuffer.cpp"

#endif