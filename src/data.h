#ifndef DATA_H // include guard
#define DATA_H

#include <atomic>
#include <stddef.h>
#include <string.h>
#include <vector>
#include <stdexcept>
#include <mutex>
#include <chrono>
#include <math.h>
#include <thread>
#include <shared_mutex>
#include "proto/protocol.pb.h"

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

#define THREAD_POOL_ARGS (int, data::parseThreadPoll&)
#define THREAD_POOL_ARGS_NAMED (int myid, data::parseThreadPoll& threadPoolL)

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

            void resize(int newSize){
                bufferDataVec.resize(newSize);
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

            void writeover(void* dest){
                memcpy(dest, bufferDataVec.data(), bufferDataVec.size());
            }

            void writeover(int destPos, buffer& Data){
                if(checkDestPos(destPos)){ return; }
                if(Data.size() <= bufferDataVec.size()){
                        memcpy(bufferDataVec.data() + destPos, Data.getData(), Data.size());
                } else {
                    failSize();
                }
            }

            void writeover(int destPos, buffer* Data){
                if(checkDestPos(destPos)){ return; }
                if(Data->size() <= bufferDataVec.size()){
                        memcpy(bufferDataVec.data() + destPos, Data->getData(), Data->size());
                } else {
                    failSize();
                }
            }

            void writeover(int destPos, buffer& Data, int size){
                if(checkValidInsertSize(size)){ return; }
                if(checkDestPos(destPos)){ return; }
                if(destPos + size <= bufferDataVec.size()){
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
                if(destPos + size <= bufferDataVec.size()){
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
                if(destPos + size <= bufferDataVec.size()){
                    memcpy(bufferDataVec.data() + destPos, data, size);
                } else {
                    failSize();
                }
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

            buffer(){
                bufferDataVec.reserve(256);
            }

            template <typename T>
            buffer(T *refBuffer, size_t len){
                bufferDataVec.reserve(256);
                    insertArray(refBuffer, len);
            }

            template <typename T>
            buffer(const T *refBuffer, size_t len){
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
        std::vector<std::atomic<bool>> audioState;
        std::vector<std::mutex> mutexes;
        int readPos;

        std::atomic<int> _size;

        int checkValues(){
            int result = -1;
            for(int i = 1; i <= 5; i++){
                int ant = readPos - i;
                if(ant < 0) ant += 256;
                if(audioState[ant].load()){
                    result = ant;
                } 
            }
            return result;
        }

    public:
        AudioQueue(): mutexes(256), audioState(256){
            readPos = 0;
            audio.resize(256);

            for(auto& samplePack : audio){
                samplePack.resize(256);
                for(auto& sample : samplePack){
                    sample = 0;
                }
            }

            for(int i = 0; i < 256; i++){
                audioState[i] = false;
            }
        }

        void push(int audioNumb, std::vector<std::int16_t>& audioPack){
            if(audioNumb < 0 || audioNumb >= 256) audioNumb = 0;

            if(!audioState[audioNumb].load()){
                if(audioNumb < 0 || audioNumb >= 256) audioNumb = 0;
                _size++;
                audio[audioNumb] = audioPack;
                audioState[audioNumb] = true;
            } else {
                mutexes[readPos].lock();
                audio[audioNumb] = audioPack;
                audioState[audioNumb] = true;
                mutexes[readPos].unlock();
            }
        }

        bool pop(std::vector<std::int16_t>& result){
            if(readPos >= 256) readPos -= 256;
            bool success = false;

            int checkAnt = checkValues();
            if(checkAnt > -1){
                readPos = checkAnt;
            }

            if(audioState[readPos].load()){
                mutexes[readPos].lock();
                if(audioState[readPos].load()){
                    result.insert(result.end(), audio[readPos].begin(), audio[readPos].end());
                    mutexes[readPos].unlock();
                    success = true;
                    audioState[readPos] = false;
                    _size--;
                } else {
                    mutexes[readPos].unlock();
                }
            }

            readPos++;
            if(readPos >= 256) readPos -= 256;

            return success;
        }

        int size(){
            return _size.load();
        }

        int absoluteSize(){
            int absSize = 0;
            for(int i = 0; i < 256; i++){
                if(audioState[i].load()){
                    absSize++;
                }
            }
            return absSize;
        }

        int relativeCompleteSize(){
            int absSize = 0;
            for(int i = readPos; i < 256; i++){
                if(audioState[i].load()){
                    absSize++;
                } else {
                    break;
                }
            }
            return absSize;
        }

        bool canReadNext(){
            return audioState[readPos].load();
        }

        void hyperSearch(){
            for(int i = 0; i < 256; i++){
                if(audioState[i].load()){
                    mutexes[i].lock();
                    readPos = i;
                    mutexes[i].unlock();
                }
            }
        }

        void hyperClean(){
            for(int i = 0; i < readPos; i++){
                if(audioState[i].load()){
                    mutexes[i].lock();
                    audioState[i] = false;
                    _size--;
                    mutexes[i].unlock();
                }
            }
        }
    };

    class asyncQueue{

    private:
        std::vector<protocol::Server> obj;
        std::vector<std::atomic<bool>> objState;
        std::vector<std::mutex> mutexes;
        std::atomic_int writePos;
        std::atomic_int readPos;
        std::atomic_int avaliable;
        size_t size;

        std::mutex pushMutex;
        std::mutex popMutex;

        void objNumCheck(int &num);

        void init(size_t _size);

    public:
        asyncQueue();

        asyncQueue(size_t _size);

        void push(protocol::Server& objPack);

        bool pop(protocol::Server& result);

        int getAvaliable();
    };

    typedef std::unordered_map<int, std::shared_ptr<asyncQueue>> asyncQueueMap;
    typedef std::vector<std::shared_ptr<asyncQueue>> asyncQueueVector;
    typedef std::unordered_map<int, std::shared_ptr<std::thread>> asyncThreadPool;
    typedef std::unordered_map<int, std::atomic<bool>*> asyncThreadPool_State;

    //thread safe
    class parseThreadPoll{

        private:
        asyncQueueMap queue;
        asyncThreadPool threadPool;
        asyncThreadPool_State threadState;

        std::shared_mutex syncMutex;

        public:

        void parser(std::function<void THREAD_POOL_ARGS> parse, int myid){
            syncMutex.lock_shared();
            bool isRuning = threadState[myid]->load();
            syncMutex.unlock_shared();

            while(isRuning){
                parse(myid, *this);
                
                syncMutex.lock_shared();
                bool isRuning = threadState[myid]->load();
                syncMutex.unlock_shared();
            }
        }

        parseThreadPoll(){

        }

        //thread safe
        void insert(int playerID, std::function<void THREAD_POOL_ARGS> parse){
            if(!exist(playerID)){
                syncMutex.lock();
                threadState[playerID] = new std::atomic<bool>();
                *threadState[playerID] = true;

                queue[playerID] = std::make_shared<asyncQueue>();

                threadPool[playerID] = std::make_shared<std::thread>([this, parse, playerID]() {
                    parser(parse, playerID);
                });
                
                syncMutex.unlock();
            }
        }

        //thread safe
        bool exist(int playerID){
            syncMutex.lock_shared();
            if(threadPool.find(playerID) != threadPool.end()){
                syncMutex.unlock_shared();
                return true;
            }
            syncMutex.unlock_shared();
            return false;
        }

        //thread safe
        bool push(int playerID, protocol::Server& received){
            if(exist(playerID)){
                syncMutex.lock_shared();
                queue[playerID]->push(received);
                syncMutex.unlock_shared();
                return true;
            }
            return false;
        }

        //thread safe
        bool pop(int playerID, protocol::Server& result){
            if(exist(playerID)){
                syncMutex.lock_shared();
                bool success = queue[playerID]->pop(result);
                syncMutex.unlock_shared();
                return success;
            }
            return false;
        }

        //thread safe
        void stop(int playerID){
            if(exist(playerID)){
                syncMutex.lock();
                *threadState[playerID] = false;
                if(threadPool[playerID]->joinable()){
                    threadPool[playerID]->join();
                }
                syncMutex.unlock();
            }
        }

        //thread safe
        void stopAll(){

            syncMutex.lock();
            for(auto& state : threadState){
                *state.second = false;
            }
            for(auto& threadL : threadPool){
                if(threadL.second->joinable()){
                    threadL.second->join();
                }
            }
            threadPool.clear();
            queue.clear();
            threadState.clear();
            syncMutex.unlock();

        }

    };

    /*

    class PlayerBinaryTree{

        private:

        PlayerBinaryTree* left;
        PlayerBinaryTree* right;
        PLAYER player;
        int id;

        bool empty;

        public:

        void setPlayer(PLAYER _player){
            player = _player;
        }

        void setId(int _id){
            id = _id;
        }

        int leftID(){
            left->getID();
        }

        int rightID(){
            right->getID();
        }

        int getID(){
            return id;
        }

        PLAYER getPlayer(){
            return player;
        }

        PlayerBinaryTree(PLAYER _player): player(_player){
            id = _player->id;
            empty = false;
        }

        bool isEmpty(){
            return empty;
        }

        void setNotEmpty(){
            empty = false;
        }

        int insert(PLAYER& _player, int _id){
            if(left == nullptr){
                left = new PlayerBinaryTree(_player);
            } else if(_id < leftID()){
                if(left->isEmpty()){
                    left->setPlayer(_player);
                    left->setId(_id);
                    left->setNotEmpty();
                    return;
                }
                left->insert(_player, _id);
            } else if (right == nullptr){
                right = new PlayerBinaryTree(_player);
            } else {
                if(right->isEmpty()){
                    right->setPlayer(_player);
                    right->setId(_id);
                    right->setNotEmpty();
                    return;
                }
                right->insert(_player, _id);
            }
        }

        void remove(int _id){
            if(_id == id){
                empty = true;
                return;
            }

            if(left != nullptr){
                if(_id < leftID()){
                    left->remove(_id);
                    return;
                }
            }
            if(right != nullptr){
                if(_id < rightID()){
                    right->remove(_id);
                    return;
                }
            }
        }

        PlayerBinaryTree* search(int _id){
            if(_id == id){
                return this;
            }

            if(left != nullptr){
                if(_id < leftID()){
                    return left->search(_id);
                }
            }
            if(right != nullptr){
                return right->search(_id);
            }
            return nullptr;
        }

    };
    */

    typedef std::chrono::high_resolution_clock clock;
    template <typename T>
    using duration = std::chrono::duration<T>;

    static void preciseSleep(double seconds) {
        using namespace std;
        using namespace std::chrono;

        static double estimate = 5e-3;
        static double mean = 5e-3;
        static double m2 = 0;
        static int64_t count = 1;

        while (seconds > estimate) {
            auto start = high_resolution_clock::now();
            std::this_thread::sleep_for(milliseconds(1));
            auto end = high_resolution_clock::now();

            double observed = (end - start).count() / 1e9;
            seconds -= observed;

            ++count;
            double delta = observed - mean;
            mean += delta / count;
            m2   += delta * (observed - mean);
            double stddev = sqrt(m2 / (count - 1));
            estimate = mean + stddev;
        }

        // spin lock
        auto start = high_resolution_clock::now();
        while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
    }

    }
#endif