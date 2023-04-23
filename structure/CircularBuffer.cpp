#include "CircularBuffer.h"
#include <stdexcept>

template <typename T>
CircularBuffer<T>::CircularBuffer() {
    buffer_.resize(1024);
}

template <typename T>
CircularBuffer<T>::CircularBuffer(size_t size){
    buffer_.resize(size);
}

template <typename T>
void CircularBuffer<T>::Push(const std::vector<T>& elements) {
    std::lock_guard<std::mutex> lock(mutex_push);
    for (const auto& element : elements) {
        buffer_[head_] = element;
        head_ = (head_ + 1) % buffer_.size();
        if (head_ == tail_) {
            tail_ = (tail_ + 1) % buffer_.size();
        }
    }
}

template <typename T>
void CircularBuffer<T>::Push(const T* elements, size_t count) {
    std::lock_guard<std::mutex> lock(mutex_push);
    for (size_t i = 0; i < count; i++) {
        buffer_[head_] = elements[i];
        head_ = (head_ + 1) % buffer_.size();
        if (head_ == tail_) {
            tail_ = (tail_ + 1) % buffer_.size();
        }
    }
}

template <typename T>
std::vector<T> CircularBuffer<T>::Pop(size_t count) {
    std::vector<T> elements;
    while (count > 0) {
        std::lock_guard<std::mutex> lock(mutex_pop);
        if (head_ == tail_) {
            std::lock_guard<std::mutex> lock(mutex_push);
            break;
        }
        T element = buffer_[tail_];
        tail_ = (tail_ + 1) % buffer_.size();
        elements.push_back(element);
        count--;
    }
    return elements;
}

template <typename T>
size_t CircularBuffer<T>::Size() const {
    std::lock_guard<std::mutex> lockpush(mutex_push);
    std::lock_guard<std::mutex> lockpop(mutex_pop);
    return (head_ + buffer_.size() - tail_) % buffer_.size();
}

template <typename T>
size_t CircularBuffer<T>::Capacity() const {
    return buffer_.size();
}

template <typename T>
void CircularBuffer<T>::Push(T element) {
    {
        std::lock_guard<std::mutex> lock(mutex_push);
        buffer_[head_] = element;
        head_ = (head_ + 1) % buffer_.size();
        if (head_ == tail_) {
            tail_ = (tail_ + 1) % buffer_.size();
        }
    }
}