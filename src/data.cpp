#include "data.h"

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
        if(pos >= size) pos -= size;

        mutexes[pos].lock();

            //Escrever sobre
            obj[pos].CopyFrom(objPack);
            objState[pos] = true;
            if(!objState[pos].load()){
                avaliable++;
            }

            writePos++;
            if(writePos.load() >= size) writePos -= size;

        mutexes[pos].unlock();

        pushMutex.unlock();
    }

    //thread safe
    bool asyncQueue::pop(protocol::Server& result){
        popMutex.lock();

        bool readResult = false;
        int pos = readPos;
        
        if(pos >= size) pos -= size;

        if(objState[pos].load()){
            mutexes[pos].lock();

            if(objState[pos].load()){
                result.CopyFrom(obj[pos]);
                objState[pos] = false;
                readResult = true;
                avaliable--;
                readPos++;
                if(readPos.load() >= size) readPos -= size;
            }
            
            mutexes[pos].unlock();
        }

        popMutex.unlock();

        return readResult;
    }

    int asyncQueue::getAvaliable(){
        return avaliable.load();
    }

};