#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <array>
#include <mutex>
#include <semaphore.h>
#include <vector>
#include <atomic>
#include <thread>

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/circular_buffer.hpp>

template <typename T>
class ConcurrentCircularBuffer {
public:
    ConcurrentCircularBuffer(size_t size) :
        m_buffer(size),
        m_head(0),
        m_tail(0),
        m_count(0) {}

    bool write(const T* data, size_t count) {
        std::lock_guard<std::mutex> lock(mutex_write);

        // Verifica se há espaço suficiente para escrita
        if (count > m_count - m_count) {
            return false;
        }

        // Verifica se a posição de escrita ultrapassa a posição de leitura
        size_t nextWrite = m_tail;
        size_t distanceToEnd = m_count - nextWrite;
        if (m_head < nextWrite) {
            distanceToEnd = m_head - nextWrite;
        }
        if (distanceToEnd < count) {
            return false;
        }

        // Escreve os dados no buffer circular
        if (nextWrite > m_head) {
            memcpy(&m_buffer[m_head], data, count * sizeof(T));
        } else {
            size_t itemsToEnd = m_count - m_head;
            memcpy(&m_buffer[m_head], data, itemsToEnd * sizeof(T));
            memcpy(m_buffer.data(), &data[itemsToEnd], (count - itemsToEnd) * sizeof(T));
        }

        // Atualiza a posição de escrita e a contagem de itens
        m_tail = nextWrite;
        m_count += count;

        return true;
    }

    bool read(T* data, size_t count) {
        std::lock_guard<std::mutex> lock(mutex_read);
        if (count > m_count) {
            return false;
        }

        // Verifica se a posição de leitura ultrapassa a posição de escrita
        size_t nextRead = m_head;
        if (nextRead >= m_tail) {
            return false;
        }

        // Verifica se há dados disponíveis para leitura
        nextRead = (m_tail + count) % m_count;
        if (nextRead == m_head) {
            return false;
        }

        // Lê os dados do buffer circular
        if (nextRead > m_tail) {
            memcpy(data, &m_buffer[m_tail], count * sizeof(T));
        } else {
            size_t itemsToEnd = m_count - m_tail;
            memcpy(data, &m_buffer[m_tail], itemsToEnd * sizeof(T));
            memcpy(&data[itemsToEnd], m_buffer.data(), (count - itemsToEnd) * sizeof(T));
        }

        // Atualiza a posição de leitura
        m_tail = nextRead;
        m_count -= count;

        return true;
    }

private:
    std::vector<T> m_buffer;
    std::atomic<size_t> m_head;
    std::atomic<size_t> m_tail;
    std::atomic<size_t> m_count;

    mutable std::mutex mutex_read;
    mutable std::mutex mutex_write;
};

/*
Use para buffers pequenos.
Performance para muitas threads, poucos dados.
Suporta multipla leitura e multipla escrita.
Tamanho padrão: 1024
*/
template <typename T>
class CircularBuffer {
public:
    //Inicia o buffer com tamanho 1024.
    CircularBuffer();
    //Inicia o buffer com tamanho predifinido.
    //size : tamanho do buffer
    CircularBuffer(size_t size);
    void Push(const std::vector<T>& elements);
    void Push(const T* elements, size_t count);
    void Push(T element);
    std::vector<T> Pop(size_t count);
    //Retorna a quantidade e elementos válidos.
    //Demanda mais tempo quanto maior o buffer.
    size_t Size() const;
    //Capacidade do buffer
    size_t Capacity() const;

private:
    void headKickTail();
    std::vector<T> buffer_;
    std::vector<bool> buffer_check;
    std::vector<bool> tail_check;
    size_t head_ = 0;
    size_t tail_ = 0;
    mutable std::mutex mutex_push;
    mutable std::mutex mutex_pop;
};

template <typename T>
class circ_buffer {
public:
    typedef boost::mutex::scoped_lock lock;
    circ_buffer() {}
    circ_buffer(int n) {cb.set_capacity(n);}

    void push(T imdata) {
        lock lk(mpush);
        cb.push_back(imdata);
        buffer_not_empty.notify_one();
    }

    void push(T* imdata, size_t size) {
        lock lk(mpush);
        cb.insert(imdata,imdata + (size * sizeof(T)));
        buffer_not_empty.notify_one();
    }

    T pop() {
        lock lk(mpop);
        while (cb.empty())
            buffer_not_empty.wait(lk);
        T imdata = cb.front();
        cb.pop_front();
        return imdata;
    }

    T pop(size_t size) {
        lock lk(mpop);
        while (cb.empty())
            buffer_not_empty.wait(lk);
        T imdata = cb.front();
        cb.pop_front();
        return imdata;
    }

    void clear() {
        lock lk(mpush);
        lock lkk(mpop);
        cb.clear();
    }
    int size() {
        lock lk(mpush);
        lock lkk(mpop);
        return cb.size();
    }
    void set_capacity(int capacity) {
        lock lk(mpush);
        lock lkk(mpop);
        cb.set_capacity(capacity);
    }
private:
    boost::condition buffer_not_empty;
    boost::mutex mpush;
    boost::mutex mpop;
    boost::circular_buffer<T> cb;
};


template <typename T>
class hyperBuffer{

private:

std::atomic<size_t> _capacity;

std::vector<T> writeBuffer;
std::vector<T> readBuffer;

std::atomic<size_t> _sizeW;
std::atomic<size_t> _sizeR;

std::atomic<bool> canW;
std::atomic<bool> canR;

bool lockW(){
    if(!canW){
        canW = true;
        return true;
    }
    while(canW){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if(!canW){
        canW = true;
        return true;
    } else {
        return false;
    }
}
void unLockW(){
    canW = false;
}

bool lockR(){
    if(!canR){
        canR = true;
        return true;
    }
    while(canR){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if(!canR){
        canR = true;
        return true;
    } else {
        return false;
    }
}
void unLockR(){
    canR = false;
}

size_t tryRead(T *data, size_t size){
    size_t sizeR = _sizeR;

    if(canR){
        memcpy(data, readBuffer.data(), size * sizeof(T));
        
        return size;
    }

    if(sizeR > 0){
    if(size <= sizeR ){
        memcpy(data, readBuffer.data(), size * sizeof(T));
        _sizeR -= sizeR;
        return size;
    } else {
        memcpy(data, readBuffer.data(), sizeR * sizeof(T));
        _sizeR -= sizeR;
        return sizeR;
    }
    }

    return 0;

}

public:

hyperBuffer(size_t capacity) : writeBuffer(capacity), readBuffer(capacity), _sizeW(0), _sizeR(0)
{
    _capacity = capacity;
}

size_t write(T *data, size_t size){
    if(!lockW()){
        return 0;
    }

    //try swap to read if have space or 
    if(_sizeW > 0 && _sizeR <= 0){
        //try lock read
        bool rIsLocked = false;
        if(!canR){
            canR = true;
            rIsLocked = true;
        }
        if(rIsLocked){
            readBuffer.swap(writeBuffer);
            size_t sizeR_TEMP = _sizeR;
            size_t sizeW_TEMP = _sizeW;
            _sizeR = sizeW_TEMP;
            _sizeW = sizeR_TEMP;
            unLockR();
        }
    }

    size_t sizeW = _sizeW;
    size_t sizeR = _sizeR;

    if(sizeR + size < _capacity){
        //try lock read
        bool rIsLocked = false;
        if(!canW){
            canW = true;
            rIsLocked = true;
        }

        if (rIsLocked)
        {
            if (sizeW + sizeR <= _capacity)
            {
                // add write buffer to read
                memcpy(readBuffer.data() + sizeR, writeBuffer.data(), sizeW * sizeof(T));
                _sizeR += sizeW;
                sizeR = _sizeR;
                _sizeW = 0;
                sizeW = 0;
                // try add actual buffer
                if (sizeR + size < _capacity)
                {
                    memcpy(readBuffer.data() + sizeR, data, size * sizeof(T));
                    _sizeR += size;
                    unLockR();
                    unLockW();
                    return size;
                }
                unLockR();
            }
        }
    }

    if(sizeW + size < _capacity){
        memcpy(writeBuffer.data() + _sizeW, data, size * sizeof(T));
        _sizeW += size;
        unLockW();
        return size;
    }

    return 0;
    
}

size_t read(T *data, size_t size){
    size_t isRead = 0;

    if(!lockW()){
        return 0;
    }

    if(_sizeR <= 0 && _sizeW <= 0){
        return 0;
    }

    if(_sizeR <= 0){
        //try lock write
        bool wIsLocked = false;
        if(!canW){
            canW = true;
            wIsLocked = true;
        }
        if(!wIsLocked) return 0;

        readBuffer.swap(writeBuffer);
        size_t sizeR_TEMP = _sizeR;
        size_t sizeW_TEMP = _sizeW;
        _sizeR = sizeW_TEMP;
        _sizeW = sizeR_TEMP;
        unLockW();
    }

    isRead = tryRead(data, size);
    unLockR();

    return isRead;
}

};

#include "CircularBuffer.cpp"

#endif