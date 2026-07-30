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
#include <functional>
#include "proto/protocol.pb.h"

struct ServerPacket;

#define ERROR_NO_ERROR        0
#define ERROR_TRY_AGAIN       10
#define ERROR_RECONNECT       20
#define ERROR_RESTART         30
#define ERROR_ADDRESS_PROBLEM 40
#define ERROR_MAX_ATTEMPT     50
#define ERROR_UNKNOW          255

struct SimpleCoords{
    float x;
    float y;
    float z;
    std::uint32_t map;
};

class Coords{
    private:
    std::atomic<float> x;
    std::atomic<float> y;
    std::atomic<float> z;
    std::atomic_uint32_t map;
    public:
    Coords(){

    }
    void setCoords(Coords& coord){
        x = coord.getX();
        y = coord.getY();
        z = coord.getZ();
        map = coord.getMap();
    }
    void setCoords(std::uint32_t _map, float _x, float _y, float _z){
        x = _x;
        y = _y;
        z = _z;
        map = _map;
    }
    void setMap(std::uint32_t _map){
        map = _map;
    }
    void setX(float _x){
        x = _x;
    }
    void setY(float _y){
        y = _y;
    }
    void setZ(float _z){
        z = _z;
    }
    float getX(){
        return x.load();
    }
    float getY(){
        return y.load();
    }
    float getZ(){
        return z.load();
    }
    std::uint32_t getMap(){
        return map.load();
    }

    SimpleCoords getCoord(){
        SimpleCoords coord;
        coord.x = getX();
        coord.y = getY();
        coord.z = getZ();
        coord.map = getMap();
        return coord;
    }
};

#define THREAD_POOL_ARGS (int, data::parseThreadPoll&)
#define THREAD_POOL_ARGS_NAMED (int threadId, data::parseThreadPoll& threadPoolL)

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

            void writeover(int destPos, void* data, int size){
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
        std::vector<std::atomic<int>> audioSize;
        std::vector<std::mutex> mutexes;
        std::atomic<int> readPos;
        std::atomic<int> lastPoped;
        std::atomic<int> lastPush;

        std::atomic<int> _size;

        int checkValues(){
            int result = -1;
            for(int i = 1; i <= 5; i++){
                int ant = readPos - i;
                if(ant < 0) ant += 256;
                if(audioState[ant].load() && ant > lastPoped.load()){
                    result = ant;
                } 
            }
            return result;
        }

        void checkPos(int& pos){
            if(pos < 0){ pos += 256; }
            else if (pos >= 256){ pos -= 256; }
        }

    public:
        AudioQueue(): mutexes(256), audioState(256), audioSize(256){
            readPos = 0;
            lastPoped = 0;
            lastPush = 0;
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

            for(int i = 0; i < 256; i++){
                audioSize[i] = 0;
            }
        }

        //thread safe
        void push(int audioNumb, std::vector<std::int16_t>& audioPack){
            if(audioNumb < 0 || audioNumb >= 256) audioNumb = 0;
                mutexes[audioNumb].lock();
                lastPush = audioNumb;

                if(!audioState[audioNumb].load()){
                    _size++;
                }
                audio[audioNumb] = audioPack;
                audioState[audioNumb] = true;
                audioSize[audioNumb] = audioPack.size();
                mutexes[audioNumb].unlock();
        }

        //unsafe, just one thread
        bool pop(std::vector<std::int16_t>& result){
            int readPosC = readPos.load();
            if(readPosC >= 256) readPosC -= 256;

            bool success = false;

            int checkAnt = checkValues();
            if(checkAnt > -1){
                readPos = checkAnt;
                readPosC = checkAnt;
            }

            if(audioState[readPosC].load()){
                mutexes[readPosC].lock();
                if(audioState[readPosC].load()){
                    result.insert(result.end(), audio[readPosC].begin(), audio[readPosC].end());
                    mutexes[readPosC].unlock();
                    success = true;
                    audioState[readPosC] = false;
                    audioSize[readPosC] = 0;
                    lastPoped = readPosC;
                    _size--;
                } else {
                    mutexes[readPosC].unlock();
                }
            }

            readPos++;
            if(readPos >= 256) readPos = 0;

            return success;
        }

        //thread safe - Relative - fast
        void setReadPos(int pos){
            if(pos >= 256) { pos -= 256; }
            else if (pos < 0) pos += 256;
            readPos = pos;
        }

        //thread safe - fast
        int getReadPos(){
            return readPos.load();
        }

        //thread safe - fast
        int getLastPoped(){
            return lastPoped.load();
        }

        int getLastPush(){
            return lastPush.load();
        }

        //thread safe - fast
        int getSampleCountInFrame(int posA, int posB){
            int absSize = 0;
            for(int i = posA; i <= posB; i++){
                absSize += audioSize[i];
            }
            return absSize;
        }

        //thread safe - fast
        bool slotState(int slot){
            checkPos(slot);
            return audioState[slot].load();
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

        //thread safe - fast - estimativa
        int relativeCompleteCountSamples(){
            int readPosC = readPos.load();
            int toPos = readPosC + 10;
            if(toPos >= 256) toPos -= 256;
            return getSampleCountInFrame(readPosC, toPos);
        }

        //thread safe - fast - estimativa
        void clearFrames(int from, int to){
            checkPos(from);
            checkPos(to);
            if(from > to){
                for(int i = from; i < 256; i++){
                    audioState[i] = false;
                    audioSize[i] = 0;
                }
                for(int i = 0; i <= to; i++){
                    audioState[i] = false;
                    audioSize[i] = 0;
                }
            } else {
                for(int i = from; i <= to; i++){
                    audioState[i] = false;
                    audioSize[i] = 0;
                }
            }
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

        bool preview(std::vector<std::int16_t>& result){
            int readPosC = readPos.load();
            if(readPosC >= 256) readPosC -= 256;

            bool success = false;

            if(audioState[readPosC].load()){
                mutexes[readPosC].lock();
                if(audioState[readPosC].load()){
                    result.insert(result.end(), audio[readPosC].begin(), audio[readPosC].end());
                    mutexes[readPosC].unlock();
                    success = true;
                } else {
                    mutexes[readPosC].unlock();
                }
            }

            return success;
        }
    };

    /**
 * OpusQueue — Thread-safe circular buffer para pacotes de áudio Opus (VOIP)
 *
 * Modelo de concorrência:
 *   - push()  → pode ser chamado por N threads simultaneamente
 *   - pop()   → deve ser chamado por apenas 1 thread consumidora
 *   - leituras (getReadPos, slotState, etc.) → thread-safe, sem lock
 *
 * Layout de slots: 256 slots circulares (índice 0–255)
 * Cada slot protegido por seu próprio mutex → máxima granularidade
 *
 * Invariantes garantidos:
 *   - audioState[i] só transita true→false DENTRO do mutex[i]
 *   - _size é mantido via fetch_add/fetch_sub com ordering correto
 *   - lastPoped e lastPush são escritos com release, lidos com acquire
 */
 
static constexpr int QUEUE_CAPACITY = 256;
static constexpr int QUEUE_MASK     = QUEUE_CAPACITY - 1; // 0xFF
 
class OpusQueue_alt {
public:
    // -----------------------------------------------------------------------
    // Construtor
    // -----------------------------------------------------------------------
    OpusQueue_alt()
        : _mutexes(QUEUE_CAPACITY)
        , _audioState(QUEUE_CAPACITY)
        , _audioSize(QUEUE_CAPACITY)
        , _audio(QUEUE_CAPACITY)
    {
        for (int i = 0; i < QUEUE_CAPACITY; ++i) {
            _audioState[i].store(false, std::memory_order_relaxed);
            _audioSize[i].store(0,     std::memory_order_relaxed);
            _audio[i].reserve(256);
        }
        _readPos.store(0,  std::memory_order_relaxed);
        _lastPopped.store(-1, std::memory_order_relaxed);
        _lastPush.store(-1,  std::memory_order_relaxed);
        _size.store(0,     std::memory_order_relaxed);
    }
 
    // Não-copiável, não-movível (mutexes não são copiáveis)
    OpusQueue_alt(const OpusQueue_alt&)            = delete;
    OpusQueue_alt& operator=(const OpusQueue_alt&) = delete;
 
    // -----------------------------------------------------------------------
    // push — thread-safe, múltiplos produtores
    // -----------------------------------------------------------------------
    void push(int slot, int sampleCount, const std::vector<char>& audioPack) {
        slot = wrapSlot(slot);
 
        std::lock_guard<std::mutex> lk(_mutexes[slot]);
 
        // Se slot estava vazio, incrementa contador
        if (!_audioState[slot].load(std::memory_order_relaxed)) {
            _size.fetch_add(1, std::memory_order_relaxed);
        }
 
        _audio[slot]     = audioPack;
        _audioSize[slot].store(sampleCount,  std::memory_order_relaxed);
        _audioState[slot].store(true, std::memory_order_release); // release: visível a leitores
 
        _lastPush.store(slot, std::memory_order_release);
    }
 
    // -----------------------------------------------------------------------
    // pop — deve ser chamado por apenas 1 thread consumidora
    // Retorna: número de samples do frame lido, ou 0 se vazio
    // -----------------------------------------------------------------------
    int pop(std::vector<char>& result) {
        int pos = _readPos.load(std::memory_order_relaxed) & QUEUE_MASK;
 
        // Tenta recuperar pacote atrasado (jitter recovery)
        int recovered = findLatePacket(pos);
        if (recovered >= 0) {
            pos = recovered;
            _readPos.store(pos, std::memory_order_relaxed);
        }
 
        int returnSize = 0;
 
        // Double-checked locking: primeiro sem lock, depois com lock
        if (_audioState[pos].load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lk(_mutexes[pos]);
 
            if (_audioState[pos].load(std::memory_order_relaxed)) {
                result.insert(result.end(), _audio[pos].begin(), _audio[pos].end());
                returnSize = _audioSize[pos].load(std::memory_order_relaxed);
 
                // Limpa slot — tudo dentro do lock
                _audio[pos].clear();
                _audioSize[pos].store(0,     std::memory_order_relaxed);
                _audioState[pos].store(false, std::memory_order_release);
                _lastPopped.store(pos,       std::memory_order_release);
                _size.fetch_sub(1, std::memory_order_relaxed);
            }
        }
 
        // Avança posição de leitura
        _readPos.store((pos + 1) & QUEUE_MASK, std::memory_order_relaxed);
        return returnSize;
    }
 
    // -----------------------------------------------------------------------
    // preview — lê sem consumir (1 thread consumidora)
    // -----------------------------------------------------------------------
    int preview(std::vector<char>& result) const {
        int pos = _readPos.load(std::memory_order_relaxed) & QUEUE_MASK;
        int returnSize = 0;
 
        if (_audioState[pos].load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lk(_mutexes[pos]);
            if (_audioState[pos].load(std::memory_order_relaxed)) {
                result.insert(result.end(), _audio[pos].begin(), _audio[pos].end());
                returnSize = _audioSize[pos].load(std::memory_order_relaxed);
            }
        }
        return returnSize;
    }
 
    // -----------------------------------------------------------------------
    // clearFrames — limpa range [from, to] (circulares)
    // AVISO: não deve ser chamado concorrentemente com pop()
    // -----------------------------------------------------------------------
    void clearFrames(int from, int to) {
        from = wrapSlot(from);
        to   = wrapSlot(to);
 
        auto clearSlot = [this](int i) {
            std::lock_guard<std::mutex> lk(_mutexes[i]);
            if (_audioState[i].load(std::memory_order_relaxed)) {
                _audioState[i].store(false, std::memory_order_release);
                _audioSize[i].store(0,     std::memory_order_relaxed);
                _audio[i].clear();
                _size.fetch_sub(1, std::memory_order_relaxed);
            }
        };
 
        if (from > to) {
            for (int i = from; i < QUEUE_CAPACITY; ++i) clearSlot(i);
            for (int i = 0;    i <= to;             ++i) clearSlot(i);
        } else {
            for (int i = from; i <= to; ++i) clearSlot(i);
        }
    }
 
    // -----------------------------------------------------------------------
    // hyperClean — remove todos os slots antes de readPos
    // -----------------------------------------------------------------------
    void hyperClean() {
        int pos = _readPos.load(std::memory_order_acquire);
        for (int i = 0; i < pos; ++i) {
            if (_audioState[i].load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lk(_mutexes[i]);
                if (_audioState[i].load(std::memory_order_relaxed)) {
                    _audioState[i].store(false, std::memory_order_release);
                    _audioSize[i].store(0,     std::memory_order_relaxed);
                    _audio[i].clear();
                    _size.fetch_sub(1, std::memory_order_relaxed);
                }
            }
        }
    }
 
    // -----------------------------------------------------------------------
    // hyperSearch — salta para o primeiro slot ocupado a partir de readPos
    // -----------------------------------------------------------------------
    void hyperSearch() {
        int pos = _readPos.load(std::memory_order_relaxed);
        for (int i = 0; i < QUEUE_CAPACITY; ++i) {
            int candidate = (pos + i) & QUEUE_MASK;
            if (_audioState[candidate].load(std::memory_order_acquire)) {
                _readPos.store(candidate, std::memory_order_release);
                return;
            }
        }
    }
 
    // -----------------------------------------------------------------------
    // setReadPos — thread-safe (uso criterioso: sincronize externamente)
    // -----------------------------------------------------------------------
    void setReadPos(int pos) {
        _readPos.store(wrapSlot(pos), std::memory_order_release);
    }
 
    // -----------------------------------------------------------------------
    // Getters — thread-safe, lock-free
    // -----------------------------------------------------------------------
    int  getReadPos()    const { return _readPos.load(std::memory_order_acquire);    }
    int  getLastPoped() const { return _lastPopped.load(std::memory_order_acquire); }
    int  getLastPush()   const { return _lastPush.load(std::memory_order_acquire);   }
    int  size()          const { return _size.load(std::memory_order_relaxed);       }
    bool canReadNext()   const { return slotState(_readPos.load(std::memory_order_relaxed)); }
 
    bool slotState(int slot) const {
        slot = wrapSlot(slot);
        return _audioState[slot].load(std::memory_order_acquire);
    }
 
    int getSampleCountInSlot(int slot) const {
        slot = wrapSlot(slot);
        if (_audioState[slot].load(std::memory_order_acquire))
            return _audioSize[slot].load(std::memory_order_relaxed);
        return 0;
    }
 
    // -----------------------------------------------------------------------
    // Contagem de samples no intervalo [posA, posB] (circular)
    // -----------------------------------------------------------------------
    int getSampleCountInFrame(int posA, int posB) const {
        posA = wrapSlot(posA);
        posB = wrapSlot(posB);
        int total = 0;
 
        if (posA > posB) {
            for (int i = posA; i < QUEUE_CAPACITY; ++i)
                total += getSampleCountInSlot(i);
            for (int i = 0; i <= posB; ++i)
                total += getSampleCountInSlot(i);
        } else {
            for (int i = posA; i <= posB; ++i)
                total += getSampleCountInSlot(i);
        }
        return total;
    }

    int getSampleCountInFrame(int pos) const {
        return getSampleCountInSlot(pos);
    }
 
    int absoluteSize() const {
        int count = 0;
        for (int i = 0; i < QUEUE_CAPACITY; ++i)
            if (_audioState[i].load(std::memory_order_acquire)) ++count;
        return count;
    }
 
    // Frames contíguos prontos a partir de readPos
    int relativeCompleteSize() const {
        int pos = _readPos.load(std::memory_order_acquire);
        int count = 0;
        for (int i = 0; i < QUEUE_CAPACITY; ++i) {
            int slot = (pos + i) & QUEUE_MASK;
            if (!_audioState[slot].load(std::memory_order_acquire)) break;
            ++count;
        }
        return count;
    }
 
    // Estimativa de samples nos próximos 10 frames
    int relativeCompleteCountSamples() const {
        int pos   = _readPos.load(std::memory_order_acquire);
        int toPos = (pos + 10) & QUEUE_MASK;
        return getSampleCountInFrame(pos, toPos);
    }
 
    int relativeActualCountSamples() const {
        return getSampleCountInFrame(getReadPos(), getLastPush());
    }
 
    int relativeActualCountFrames() const {
        int posA = getReadPos();
        int posB = getLastPush();
        if (posB < 0) return 0;
        int count = 0;
 
        if (posA > posB) {
            for (int i = posA; i < QUEUE_CAPACITY; ++i)
                if (_audioState[i].load(std::memory_order_acquire)) ++count;
            for (int i = 0; i <= posB; ++i)
                if (_audioState[i].load(std::memory_order_acquire)) ++count;
        } else {
            for (int i = posA; i <= posB; ++i)
                if (_audioState[i].load(std::memory_order_acquire)) ++count;
        }
        return count;
    }
 
private:
    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    static int wrapSlot(int slot) {
        return ((slot % QUEUE_CAPACITY) + QUEUE_CAPACITY) % QUEUE_CAPACITY;
    }
 
    /**
     * Recuperação de pacotes atrasados (jitter buffer):
     * Procura nos últimos 5 slots antes de pos se há algum pacote
     * que ainda não foi consumido e não é anterior ao lastPopped.
     * Retorna o índice do candidato mais antigo válido, ou -1.
     */
    int findLatePacket(int pos) const {
        int lastPop = _lastPopped.load(std::memory_order_acquire);
        int result  = -1;
 
        for (int i = 1; i <= 5; ++i) {
            int candidate = (pos - i + QUEUE_CAPACITY) & QUEUE_MASK;
 
            // Só recupera se: tem dados E é mais recente que o último consumido
            if (_audioState[candidate].load(std::memory_order_acquire)) {
                // Evita regredir além do lastPopped
                bool isAhead = (lastPop < 0) ||
                               (candidate != lastPop &&
                                distanceForward(lastPop, candidate) < QUEUE_CAPACITY / 2);
                if (isAhead) {
                    result = candidate; // pega o mais próximo (i menor = mais próximo)
                    break;
                }
            }
        }
        return result;
    }
 
    // Distância circular de `from` até `to` no sentido crescente
    static int distanceForward(int from, int to) {
        return (to - from + QUEUE_CAPACITY) & QUEUE_MASK;
    }
 
    // -----------------------------------------------------------------------
    // Membros
    // -----------------------------------------------------------------------
    mutable std::vector<std::mutex>       _mutexes;     // 1 mutex por slot
    std::vector<std::atomic<bool>>        _audioState;  // slot ocupado?
    std::vector<std::atomic<int>>         _audioSize;   // sample count
    std::vector<std::vector<char>>        _audio;       // dados PCM/Opus
 
    std::atomic<int> _readPos{0};
    std::atomic<int> _lastPopped{-1};
    std::atomic<int> _lastPush{-1};
    std::atomic<int> _size{0};
};

    class OpusQueue{

    private:
        std::vector<std::vector<char>> audio;
        std::vector<std::atomic<bool>> audioState;
        std::vector<std::atomic<int>> audioSize;
        std::vector<std::mutex> mutexes;
        std::atomic<int> readPos;
        std::atomic<int> lastPoped;
        std::atomic<int> lastPush;

        std::atomic<int> _size;

        int checkValues(){
            int result = -1;
            for(int i = 1; i <= 5; i++){
                int ant = readPos - i;
                if(ant < 0) ant += 256;
                if(audioState[ant].load() && ant > lastPoped.load()){
                    result = ant;
                } 
            }
            return result;
        }

        void checkPos(int& pos){
            if(pos < 0){ pos += 256; }
            else if (pos >= 256){ pos -= 256; }
        }

    public:
        OpusQueue(): mutexes(256), audioState(256), audioSize(256){
            readPos = 0;
            lastPoped = 0;
            lastPush = 0;
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

            for(int i = 0; i < 256; i++){
                audioSize[i] = 0;
            }
        }

        //thread safe
        void push(int audioNumb, int sampleCount, const std::vector<char>& audioPack){
            if(audioNumb < 0 || audioNumb >= 256) audioNumb = 0;

                mutexes[audioNumb].lock();
                if(!audioState[audioNumb].load()){
                    _size++;
                }
                audio[audioNumb] = audioPack;
                audioSize[audioNumb] = sampleCount;
                audioState[audioNumb] = true;
                lastPush = audioNumb;

                mutexes[audioNumb].unlock();
        }

        //unsafe, just one thread
        int pop(std::vector<char>& result){
            int readPosC = readPos.load();
            if(readPosC >= 256) readPosC -= 256;

            int returnSize = 0;

            int checkAnt = checkValues();
            if(checkAnt > -1){
                readPos = checkAnt;
                readPosC = checkAnt;
            }

            if(audioState[readPosC].load()){
                mutexes[readPosC].lock();
                if(audioState[readPosC].load()){
                    result.insert(result.end(), audio[readPosC].begin(), audio[readPosC].end());
                    returnSize = audioSize[readPosC];
                    mutexes[readPosC].unlock();
                    audioSize[readPosC] = 0;
                    audioState[readPosC] = false;
                    lastPoped = readPosC;
                    _size--;
                } else {
                    mutexes[readPosC].unlock();
                }
            }

            readPos++;
            if(readPos >= 256) readPos = 0;

            return returnSize;
        }

        //thread safe - Relative - fast
        void setReadPos(int pos){
            if(pos >= 256) { pos -= 256; }
            else if (pos < 0) pos += 256;
            readPos = pos;
        }

        //thread safe - fast
        int getReadPos(){
            return readPos.load();
        }

        //thread safe - fast
        int getLastPoped(){
            return lastPoped.load();
        }

        //thread safe - fast
        int getLastPush(){
            return lastPush.load();
        }

        //thread safe - fast
        int getSampleCountInFrame(int posA, int posB){
            int absSize = 0;
            if(posA > posB + 2){
                for(int i = posA; i < 256; i++){
                    if(audioState[i].load()){
                        absSize += audioSize[i];
                    }
                }
                for(int i = 0; i <= posB; i++){
                    if(audioState[i].load()){
                        absSize += audioSize[i];
                    }
                }
            } else {
                for(int i = posA; i <= posB; i++){
                    if(audioState[i].load()){
                        absSize += audioSize[i];
                    }
                }
            }
            return absSize;
        }

        //thread safe - fast
        int getSampleCountInFrame(int frame){
            if(audioState[frame].load()){
                return audioSize[frame];
            } else {
                return 0;
            }
        }

        //thread safe - fast
        bool slotState(int slot){
            if(slot >= 256 || slot < 0){
                slot = 0;
            }
            bool result = false;
            result = audioState[slot].load();
            return result;
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

        //thread safe - fast - estimativa
        int relativeCompleteCountSamples(){
            int readPosC = readPos.load();
            int toPos = readPosC + 10;
            if(toPos >= 256) toPos -= 256;
            return getSampleCountInFrame(readPosC, toPos);
        }

        //thread safe - fast
        int relativeActualCountSamples(){
            int readPosActual = getReadPos();
            int lastPush = getLastPush();
            int sampleCountInQueue = getSampleCountInFrame(readPosActual, lastPush);
            return sampleCountInQueue;
        }

        int relativeActualCountFrames(){
            int posA = getReadPos();
            int posB = getLastPush();
            int absSize = 0;
            if(posA > posB + 2){
                for(int i = posA; i < 256; i++){
                    if(audioState[i].load()){
                        absSize += 1;
                    }
                }
                for(int i = 0; i <= posB; i++){
                    if(audioState[i].load()){
                        absSize += 1;
                    }
                }
            } else {
                for(int i = posA; i <= posB; i++){
                    if(audioState[i].load()){
                        absSize += 1;
                    }
                }
            }
            return absSize;
        }

        //thread safe - fast - estimativa
        void clearFrames(int from, int to){
            checkPos(from);
            checkPos(to);
            if(from > to){
                for(int i = from; i < 256; i++){
                    audioState[i] = false;
                    audioSize[i] = 0;
                }
                for(int i = 0; i <= to; i++){
                    audioState[i] = false;
                    audioSize[i] = 0;
                }
            } else {
                for(int i = from; i <= to; i++){
                    audioState[i] = false;
                    audioSize[i] = 0;
                }
            }
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

        int preview(std::vector<char>& result){
            int readPosC = readPos.load();
            if(readPosC >= 256) readPosC -= 256;

            int returnSize = 0;

            if(audioState[readPosC].load()){
                mutexes[readPosC].lock();
                if(audioState[readPosC].load()){
                    result.insert(result.end(), audio[readPosC].begin(), audio[readPosC].end());
                    returnSize = audioSize[readPosC];
                    mutexes[readPosC].unlock();
                } else {
                    mutexes[readPosC].unlock();
                }
            }

            return returnSize;
        }
    };

    class asyncQueue{

    private:
        std::vector<ServerPacket> obj;
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

        void push(ServerPacket& objPack);

        ServerPacket pop(bool& result);

        int getAvaliable();
    };

    //thread safe
    class parseThreadPoll{

        private:
        std::vector<asyncQueue*> queue;
        std::vector<std::thread> threadPool;
        std::vector<std::atomic<bool>*> threadState;

        std::shared_mutex syncMutex;

        int size;
        int position;

        public:

        void parser(std::function<void THREAD_POOL_ARGS> parse, std::uint32_t threadId);

        parseThreadPoll();

        void insertThread(std::function<void THREAD_POOL_ARGS> parse);

        //thread safe
        void push(int threadID, ServerPacket& received);

        //no safe
        void push(ServerPacket& received);

        //thread safe
        ServerPacket pop(int threadID, bool& result);

        //thread safe
        void stopAll();

        ~parseThreadPoll();

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