#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>

#include "soundmanager.h"
#include "player.h"
#include "socketUdp.h"

#include <SFML/Audio.hpp>

namespace player
{

    bool SelfImpl::initialized = false;
    std::uint32_t SelfImpl::reg_id_ = 0;
    std::uint32_t SelfImpl::id_ = 0;
    Self* SelfImpl::instance = nullptr;

    Self &SelfImpl::getInstance()
    {
        if (initialized)
        {
            return *instance;
        } else {
            perror("fabric player::Self");
        }
    }

    void SelfImpl::frabric(std::uint32_t reg_id, std::uint32_t id)
    {
        if(!initialized){
            reg_id_ = reg_id;
            id_ = id;
            instance = new Self(reg_id_, id_);
            initialized = true;
        }
    }

    Self::Self(std::uint32_t reg_id, std::uint32_t id) : crpt NEW_AES_GCM
    {
        my_secret_id = reg_id;
        my_id = id;
        talkRomm = 0;
        talkInLocal = true;
        coordinates.x = 0;
        coordinates.y = 0;
        coordinates.z = 0;
        coordinates.map = -1;
        connected = false;
        sampleNumber = 0;
        audioNumb = 0;
    }

    std::vector<unsigned char> Self::decrypt(std::vector<unsigned char>& buffer){
        std::vector<unsigned char> result;
        mutexDecrypt.lock();
        result = crpt.decrypt(buffer);
        mutexDecrypt.unlock();
        return result;
    }
    std::vector<unsigned char> Self::encrypt(std::vector<unsigned char>& buffer){
        std::vector<unsigned char> result;
        mutexEncrypt.lock();
        result = crpt.encrypt(buffer);
        mutexEncrypt.unlock();
        return result;
    }

    void Self::setCrpt(unsigned char* key, unsigned char* iv){
        mutexEncrypt.lock();
        mutexDecrypt.lock();
        crpt = AES_GCM(key, iv);
        mutexEncrypt.unlock();
        mutexDecrypt.unlock();
    }

    std::uint32_t Self::getMyID()
    {
        return my_id;
    }

    std::uint32_t Self::getMyRegID()
    {
        return my_secret_id;
    }

    void Self::setMyID(std::uint32_t id)
    {
        mutexID.lock();
        my_id = id;
        mutexID.unlock();
    }

    void Self::setMyRegID(std::uint32_t reg_id)
    {
        mutexRegID.lock();
        my_secret_id = reg_id;
        mutexRegID.unlock();
    }

    bool Self::isConnected(){
        return connected;
    }

    void Self::setConnect(bool isConnected){
        connected = isConnected;
    }

    void  Self::setTalkRoom(std::uint32_t room_id){
        talkRomm = room_id;
    }

    void  Self::talkLocal(){
        talkInLocal = true;
    }

    void  Self::talkRoom(){
        if(talkRomm != 0){
            talkInLocal = false;
        }
    }

    void Self::setX(float x) { coordinates.x = x; }
    void Self::setY(float y) { coordinates.y = y; }
    void Self::setZ(float z) { coordinates.z = z; }

    void Self::setMap(std::uint32_t map){
        coordinates.map = map;
    }

    void Self::setPos(std::uint32_t map, float x, float y, float z)
    {
        setMap( map );
        setX( x );
        setY( y );
        setZ( z );
    }

    void Self::setCoords(Coords coord){
        coordinates = coord;
    }

    Coords Self::getCoords()
    {
        return coordinates;
    }

    int Self::getAndAddAudioNum(){
        int result = audioNumb;
        audioNumb++;
        if(audioNumb >= 256) audioNumb = 0;
        return result;
    }

    void Self::sendPosInfo(){
        protocol::Client info;
        info.set_id(my_id);
        info.set_mapnum(coordinates.map);
        info.set_coordx(coordinates.x);
        info.set_coordy(coordinates.y);
        info.set_coordz(coordinates.z);
        socketUdpImpl::getInstance().send(info);

        socketUdpImpl::getInstance().send(info);
    }

    void Self::sendConnect(){
        protocol::Client info;
        info.set_id(my_id);
        info.set_secret_id(my_secret_id);
        info.set_mapnum(coordinates.map);
        info.set_coordx(coordinates.x);
        info.set_coordy(coordinates.y);
        info.set_coordz(coordinates.z);

        socketUdpImpl::getInstance().send(info);
    }

    void Self::sendAudio(data::buffer &buffer, int sampleTime)
    {
        protocol::Client info;
        info.set_audio(buffer.getData(), buffer.size());
        info.set_audionum(SelfImpl::getInstance().getAndAddAudioNum());
        info.set_sampletime(sampleTime);
        socketUdpImpl::getInstance().send(info);
    }

}

    //thread safe
    void PlayersManager::insertPlayer(std::uint32_t id, float x, float y, float z){
        syncMutex.lock();
        if(!existPlayerUnsafe(id)){
            players[id] = std::make_shared<Player>();
            players[id]->id = id;
            players[id]->setPosition(x,y,z);
        }
        syncMutex.unlock();
    }

    void PlayersManager::insertPlayer(PLAYER player){
        syncMutex.lock();
        if(!existPlayerUnsafe(player->id)){
            players[player->id] = player;
        }
        syncMutex.unlock();
    }

    PLAYER PlayersManager::getPlayer(std::uint32_t id){
        if(existPlayer(id)){
            syncMutex.lock_shared();
            PLAYER player = players[id];
            syncMutex.unlock_shared();
            return player;
        } else {
            return std::make_shared<Player>();
        }
    }

    bool PlayersManager::setPosition(std::uint32_t id, float x, float y, float z){
        if(existPlayer(id)){
            syncMutex.lock_shared();
            PLAYER playerREF = players[id];
            syncMutex.unlock_shared();

            playerREF->setPosition(x,y,z);
            return true;
        }
        return false;
    }

    bool PlayersManager::setAttenuation(std::uint32_t id, float new_at){
        if(existPlayer(id)){
            syncMutex.lock_shared();
            PLAYER playerREF = players[id];
            syncMutex.unlock_shared();

            playerREF->setAttenuation(new_at);
            return true;
        }
        return false;
    }

    bool PlayersManager::setMinDistance(std::uint32_t id, float new_MD){
        if(existPlayer(id)){
            syncMutex.lock_shared();
            PLAYER playerREF = players[id];
            syncMutex.unlock_shared();

            playerREF->minDistance(new_MD);
            return true;
        }
        return false;
    }

    bool PlayersManager::existPlayer(std::uint32_t id){
        syncMutex.lock_shared();
        bool result = existPlayerUnsafe(id);
        syncMutex.unlock_shared();
        return result;
    }

    bool PlayersManager::existPlayerUnsafe(std::uint32_t id){
        if(players.find(id) == players.end()){
            syncMutex.unlock_shared();
            return false;
        }
        return true;
    }

    void PlayersManager::removePlayer(std::uint32_t id){
        syncMutex.lock();
        if (!existPlayerUnsafe(id)){
            syncMutex.unlock();
            return;
        }
        players.erase(id);
        syncMutex.unlock();
    }

    void PlayersManager::clean(){
        syncMutex.lock();
        players.clear();
        syncMutex.unlock();
    }

    void PlayersManager::playersActCheckThread(bool &canRun){
        while(canRun){
            syncMutex.lock_shared();
            for(auto& playerPair : players){
                playerPair.second->actCheck();
            }
            syncMutex.unlock_shared();
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
    }

    PlayersManager::PlayersManager(){
    }

    bool PlayersManagerImpl::initialized = false;
    std::thread PlayersManagerImpl::actCheckThread = std::thread();

    bool PlayersManagerImpl::canRun = true;

    std::mutex PlayersManagerImpl::syncMutex = std::mutex();

    //Initialize to make it thread safe
    PlayersManager& PlayersManagerImpl::getInstance(){
        static PlayersManager instance;
        if(!initialized){
            syncMutex.lock();
            if(!initialized){
                actCheckThread = std::thread(&PlayersManager::playersActCheckThread, &instance, std::ref(std::ref(canRun)));
                initialized = true;
            }
            syncMutex.unlock();
        }
        return instance;
    }

    void PlayersManagerImpl::exit(){
        if(actCheckThread.joinable()){
            canRun = false;
            actCheckThread.join();
        }
    }