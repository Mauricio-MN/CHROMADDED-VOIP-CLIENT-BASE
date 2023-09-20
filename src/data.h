#ifndef DATA_H // include guard
#define DATA_H

#include <stddef.h>
#include <string.h>
#include <vector>
#include <stdexcept>
#include <mutex>

#define ERROR_NO_ERROR        0
#define ERROR_LOW             10
#define ERROR_MEDIUM          20 //
#define ERROR_SERIOUS         30
#define ERROR_ADDRESS_PROBLEM 40
#define ERROR_MAX_ATTEMPT     50
#define ERROR_UNKNOW          999

struct Coords{
    int x;
    int y;
    int z;
    int map;
};

namespace data
{

/**
    * Implementation of generic buffer class
    *
    * Initialize by reference generate a copy class and copy buffer with new owner,
    * just end of scope for owner(first) object delete the buffer,
    * secure for pass by value if owner is live,
    *
    */
    class buffer
    {
        private:
            void failSize(){
                throw std::invalid_argument( "received invalid value of size" );
            }

            bool checkDestPos(int pos){
                if(pos < 0 && pos > bufferDataVec.size() - 1){
                    failSize();
                    return true;
                }
                return false;
            }

            bool checkValidInsertSize(int size){
                if(size < 0){
                    failSize();
                    return true;
                }
                return false;
            }

        protected:
            std::vector<char> bufferDataVec;

        public:

            size_t size(){
                return bufferDataVec.size();
            }

            const std::vector<char>& getVector() const{
                return bufferDataVec;
            }

            char* getData(){
                return bufferDataVec.data();
            }

            void* getGenericData(){
                return bufferDataVec.data();
            }

            void writeover(int destPos, buffer& Data){
                if(checkDestPos(destPos)){ return; }
                else if(Data.size() <= bufferDataVec.size()){
                        memcpy(bufferDataVec.data() + destPos, Data.getData(), Data.size());
                } else {
                    failSize();
                }
            }

            void writeover(int destPos, buffer& Data, int size){
                if(checkValidInsertSize(size)){ return; }
                if(checkDestPos(destPos)){ return; }
                else if(destPos + size <= bufferDataVec.size()){
                    if(Data.size() <= size){
                        memcpy(bufferDataVec.data() + destPos, Data.getData(), size);
                    } else {
                        failSize();
                    }
                } else {
                    failSize();
                }
            }

            void writeover(int destPos, char* data, int size){
                if(checkValidInsertSize(size)){ return; }
                if(checkDestPos(destPos)){ return; }
                else if(destPos + size <= bufferDataVec.size()){
                    memcpy(bufferDataVec.data() + destPos, data, size);
                } else {
                    failSize();
                }
            }

            template <typename T>
            void writeover(int destPos, T &data){
                int size = sizeof(T);
                if(checkValidInsertSize(size)){ return; }
                if(checkDestPos(destPos)){ return; }
                else if(destPos + size <= bufferDataVec.size()){
                    memcpy(bufferDataVec.data() + destPos, data, size);
                } else {
                    failSize();
                }
            }

            template <typename T>
            void writeover(int destPos, T data){
                writeover(destPos, &data);
            }

            void insert(char* data, int size){
                if(checkValidInsertSize(size)){ return; }
                bufferDataVec.insert(bufferDataVec.end(), data, data + size);
            }

            void insert(const char* data, int size){
                if(checkValidInsertSize(size)){ return; }
                bufferDataVec.insert(bufferDataVec.end(), (char*)data, ((char*)data) + size);
            }

            void insert(buffer& Data, int size){
                if(checkValidInsertSize(size)){ return; }
                if(Data.size() <= size){
                    bufferDataVec.insert(bufferDataVec.end(), Data.getVector().begin(), Data.getVector().begin() + size);
                } else {
                    failSize();
                }
            }

            void insert(buffer& Data){
                bufferDataVec.insert(bufferDataVec.end(), Data.getVector().begin(), Data.getVector().end());
            }

            void insert(const buffer& Data){
                bufferDataVec.insert(bufferDataVec.end(), Data.getVector().begin(), Data.getVector().end());
            }

            template <typename T>
            void insert(T& data){
                bufferDataVec.insert(bufferDataVec.end(), reinterpret_cast<char*>(data), reinterpret_cast<char*>(data) + sizeof(T));
            }

            template <typename T>
            void insertArray(T* data, int size){
                if(checkValidInsertSize(size)){ return; }
                    bufferDataVec.insert(bufferDataVec.end(), reinterpret_cast<char*>(data), reinterpret_cast<char*>(data) + (sizeof(T) * size));
            }

            template <typename T>
            void insertArray(const T* data, int size){
                if(checkValidInsertSize(size)){ return; }
                    bufferDataVec.insert(bufferDataVec.end(), reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + (sizeof(T) * size));
            }

            template <typename T>
            T get(int pos){
                if(bufferDataVec.size() <= pos + sizeof(T)){
                    T* value = *reinterpret_cast<T*>(bufferDataVec.data() + pos);
                    return value;
                }
                char newData[sizeof(T)];
                for(int i = 0; i < sizeof(T); i++) newData[i] = 0;
                return *reinterpret_cast<T*>(&newData);
            }

            buffer copy(){
                return buffer(this);
            }
            
            buffer(size_t len){
                bufferDataVec.reserve(256);
                    bufferDataVec.resize(len);
            }

            buffer(buffer* Data){
                bufferDataVec.reserve(256);
                    bufferDataVec.insert(bufferDataVec.end(), Data->getVector().begin(), Data->getVector().end());
            }

            buffer(const buffer& Data){
                bufferDataVec.reserve(256);
                    insert(Data);
            }

            buffer(char *refBuffer, size_t len){
                bufferDataVec.reserve(256);
                    insert(refBuffer, len);
            }

            buffer(const char *refBuffer, size_t len){
                bufferDataVec.reserve(256);
                    insert(refBuffer, len);
            }

            buffer(){
                bufferDataVec.reserve(256);
            }

            buffer(void *refBuffer, size_t len){
                bufferDataVec.reserve(256);
                    insertArray(refBuffer, len);
            }

            buffer(const void *refBuffer, size_t len){
                bufferDataVec.reserve(256);
                    insertArray(refBuffer, len);
            }

            bool isValid(){
                if(size() > 0 ){
                    return true;
                }
                return false;
            }
        
    };

    class AudioQueue{

    private:
        std::vector<std::vector<std::int16_t>> audio;
        std::vector<bool> audioState;
        std::vector<std::mutex> mutexes;
        int readPos;

        int checkValues(){
            int result = -1;
            for(int i = 1; i <= 2; i++){
                int ant = readPos - i;
                if(ant < 0) ant += 256;
                if(audioState[ant]){
                    result = ant;
                } 
            }
            return result;
        }

    public:
        AudioQueue(int audioCount): mutexes(256){
            audio.resize(256);
            for(auto& samplePack : audio){
                samplePack.resize(audioCount);
                for(auto& sample : samplePack){
                    sample = 0;
                }
            }

            audioState.resize(256);

            for(int i = 0; i < 256; i++){
                audioState[i] = false;
            }

            readPos = 0;
        }

        void push(int audioNumb, std::vector<std::int16_t> audioPack){
            mutexes[audioNumb].lock();
            audioState[audioNumb] = true;
            audio[audioNumb] = audioPack;
            mutexes[audioNumb].unlock();
        }

        std::vector<std::int16_t> pop(){
            std::vector<std::int16_t> result;
            if(!audioState[readPos]){
                int checkAnt = checkValues();
                if(checkAnt > -1){
                    readPos = checkAnt;
                }
            }
            mutexes[readPos].lock();
            if(audioState[readPos]){
                result = audio[readPos];
                audioState[readPos] = false;
            }
            mutexes[readPos].unlock();
            readPos++;
            return result;
        }

        bool canReadNext(){
            return audioState[readPos];
        }
    };

    }
#endif