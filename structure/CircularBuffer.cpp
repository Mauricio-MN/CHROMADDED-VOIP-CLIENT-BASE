#include "CircularBuffer.h"
#include <stdexcept>

template <typename T>
CircularBuffer<T>::CircularBuffer() {
    buffer_.resize(1024);
    sem_init(&sem_, 0, 1024);
}

template <typename T>
CircularBuffer<T>::CircularBuffer(size_t size){
    buffer_.resize(size);
    sem_init(&sem_, 0, size);
}

template <typename T>
void CircularBuffer<T>::Push(const std::vector<T>& elements) {
    for (const auto& element : elements) {
        Push(element);
    }
}

template <typename T>
void CircularBuffer<T>::Push(const T* elements, size_t count) {
    for (size_t i = 0; i < count; i++) {
        Push(elements[i]);
    }
}

template <typename T>
std::vector<T> CircularBuffer<T>::Pop(size_t count) {
    std::vector<T> elements;
    while (count > 0) {
        sem_wait(&sem_);
        std::lock_guard<std::mutex> lock(mutex_);
        if (head_ == tail_) {
            sem_post(&sem_);
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
    std::lock_guard<std::mutex> lock(mutex_);
    return (head_ + buffer_.size() - tail_) % buffer_.size();
}

template <typename T>
size_t CircularBuffer<T>::Capacity() const {
    return buffer_.size();
}

template <typename T>
void CircularBuffer<T>::Push(T element) {
    sem_wait(&sem_);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_[head_] = element;
        head_ = (head_ + 1) % buffer_.size();
        if (head_ == tail_) {
            tail_ = (tail_ + 1) % buffer_.size();
        }
    }
}