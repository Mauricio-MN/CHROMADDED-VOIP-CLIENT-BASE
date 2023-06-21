#include "CircularBuffer.h"
#include <stdexcept>

template <typename T>
CircularBuffer<T>::CircularBuffer() {
    buffer_.resize(1024);
    buffer_check.resize(1024);
    tail_check.resize(1024);
    head_ = 0;
    tail_ = 0;
    
    for (int i = 0; i < buffer_check.size(); i++) {
        buffer_check[i] = false;
    }
    for (int i = 0; i < tail_check.size(); i++) {
        tail_check[i] = false;
    }
    tail_check[0] = true;
}

template <typename T>
CircularBuffer<T>::CircularBuffer(size_t size){
    buffer_.resize(size);
    buffer_check.resize(size);
    tail_check.resize(size);
    head_ = 0;
    tail_ = 0;

    for (int i = 0; i < buffer_check.size(); i++) {
        buffer_check[i] = false;
    }
    for (int i = 0; i < tail_check.size(); i++) {
        tail_check[i] = false;
    }
    tail_check[0] = true;
}

template <typename T>
void CircularBuffer<T>::Push(const T* elements, size_t count) {
    std::lock_guard<std::mutex> lock(mutex_push);
    for (size_t i = 0; i < count; i++) {
        buffer_[head_] = elements[i];
        buffer_check[head_] = true;
        head_ = (head_ + 1) % buffer_.size();
        
        headKickTail();
    }
}

template <typename T>
std::vector<T> CircularBuffer<T>::Pop(size_t count) {
    std::vector<T> elements;
    std::lock_guard<std::mutex> lock(mutex_pop);
    while (count > 0) {
        if (buffer_check[tail_] == false) {
            break;
        }
        elements.push_back(buffer_[tail_]);
        buffer_check[tail_] = false;
        tail_check[tail_] = false;
        tail_ = (tail_ + 1) % buffer_.size();
        tail_check[tail_] = true;
        count--;
    }
    return elements;
}

template <typename T>
size_t CircularBuffer<T>::Size() const {
    int count = 0;
    bool onTail = false;
    for(int i = 0; i < buffer_check.size(); i++){
        if(buffer_check[i]){
            count++;
            onTail = true;
        } else if(onTail) {
            break;
        }
    }
    return count;
    //return (head_ + buffer_.size() - tail_) % buffer_.size();
}

template <typename T>
size_t CircularBuffer<T>::Capacity() const {
    return buffer_.size();
}

template <typename T>
void CircularBuffer<T>::Push(T element) {
    Push(&element, 1);
}

template <typename T>
void CircularBuffer<T>::Push(const std::vector<T>& elements) {
    Push(elements.data(), elements.size());
}

template <typename T>
void CircularBuffer<T>::headKickTail(){
    if(tail_check[head_] == true){
        tail_check[head_] = false;
        std::lock_guard<std::mutex> lock(mutex_pop);
        tail_ = (tail_ + 1) % buffer_.size();
        tail_check[tail_] = true;
    }
}