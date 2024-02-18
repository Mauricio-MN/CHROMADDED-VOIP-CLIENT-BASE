#include "data.h"

#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace data{

    void asyncQueue::objNumCheck(int &num){
        if(num < 0 || num >= size) num = 0;
    }

    asyncQueue::asyncQueue(): mutexes(256), objState(256), obj(256){
        size = 256;

        for(int i = 0; i < size; i++){
            objState[i] = false;
        }

        writePos = 0;
        readPos = 0;
    }

    asyncQueue::asyncQueue(size_t _size): mutexes(_size), objState(_size), obj(_size){
        size = _size;

        for(int i = 0; i < size; i++){
            objState[i] = false;
        }

        writePos = 0;
        readPos = 0;
    }

    //thread safe
    void asyncQueue::push(protocol::Server& objPack){
        pushMutex.lock();
            int pos = writePos;
            writePos++;
            if(writePos.load() >= size) writePos -= size;
        pushMutex.unlock();

        mutexes[pos].lock();

            //Escrever sobre
            obj[pos] = objPack;
            objState[pos] = true;
            if(!objState[pos].load()){
                avaliable++;
            }

        mutexes[pos].unlock();
    }

    //thread safe
    protocol::Server asyncQueue::pop(bool& result){
        popMutex.lock();
        result = false;

        protocol::Server objectptc;
        int pos = readPos;

        if(objState[pos].load()){
            mutexes[pos].lock();

            if(objState[pos].load()){
                objectptc = obj[pos];
                objState[pos] = false;
                result = true;
                avaliable--;
                readPos++;
                if(readPos.load() >= size) readPos -= size;
            }
            
            mutexes[pos].unlock();
        }

        popMutex.unlock();

        return objectptc;
    }

    int asyncQueue::getAvaliable(){
        return avaliable.load();
    }

    //parseThreadPoll

    void parseThreadPoll::parser(std::function<void THREAD_POOL_ARGS> parse, std::uint32_t threadId){
        syncMutex.lock_shared();
        bool isRuning = threadState[threadId]->load();
        syncMutex.unlock_shared();

        while(isRuning){
            parse(threadId, *this);
            
            syncMutex.lock_shared();
            isRuning = threadState[threadId]->load();
            syncMutex.unlock_shared();
        }
    }

    parseThreadPoll::parseThreadPoll(){
        size = 0;
        position = 0;
    }

    void parseThreadPoll::insertThread(std::function<void THREAD_POOL_ARGS> parse){
            syncMutex.lock();

            queue.push_back(new asyncQueue());

            threadState.push_back(new std::atomic<bool>());
            *threadState.back() = true;

            int pos = threadPool.size();
            threadPool.emplace_back(&parseThreadPoll::parser, this, parse, pos);
            //threadPool.push_back(std::thread([this, parse, pos]() {
            //    parser(parse, pos);
            //}));

            size = threadPool.size();
            
            syncMutex.unlock();
    }

    //thread safe
    void parseThreadPoll::push(int threadID, protocol::Server& received){
        syncMutex.lock_shared();
        queue[threadID]->push(received);
        syncMutex.unlock_shared();
    }

    //no safe
    void parseThreadPoll::push(protocol::Server& received){
        push(position, received);
        position++;
        if(position >= size) position = 0;
    }

    //thread safe
    protocol::Server parseThreadPoll::pop(int threadID, bool& result){
        syncMutex.lock_shared();
        protocol::Server serverReceive = queue[threadID]->pop(result);
        syncMutex.unlock_shared();
        return serverReceive;
    }

    //thread safe
    void parseThreadPoll::stopAll(){

        syncMutex.lock();
        for(auto state : threadState){
            *state = false;
        }
        syncMutex.unlock();

        for(auto& threadL : threadPool){
            if(threadL.joinable()){
                threadL.join();
            }
        }

        syncMutex.lock();

        threadPool.clear();
        for (auto p : threadState)
        {
            delete p;
        } 
        threadState.clear();

        for (auto p : queue)
        {
            delete p;
        } 
        queue.clear();

        syncMutex.unlock();

    }

    parseThreadPoll::~parseThreadPoll(){
        stopAll();
    }

};