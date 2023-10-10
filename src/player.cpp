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
        my_secret_id = id;
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

    void Self::setX(int x) { coordinates.x = x; }
    void Self::setY(int y) { coordinates.y = y; }
    void Self::setZ(int z) { coordinates.z = z; }

    void Self::setMap(int map){
        coordinates.map = map;
    }

    void Self::setPos(std::uint32_t map, int x, int y, int z)
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
    
    void PlayersManager::setWaitAudioPackets(int waitAudioPacketsCount_){
        waitQueueAudioCount = waitAudioPacketsCount_;
    }

    int PlayersManager::getWaitAudioPackets(){
        return waitQueueAudioCount;
    }

    void PlayersManager::insertPlayer(std::uint32_t id, float x, float y, float z){
        insertMutex.lock();
        if(!existPlayer(id)){
            players[id] = std::make_shared<Player>();
            players[id]->id = id;
            players[id]->setPosition(x,y,z);
        }
        insertMutex.unlock();
    }

    void PlayersManager::insertPlayer(PLAYER player){
        insertMutex.lock();
        if(!existPlayer(player->id)){
            players[player->id] = player;
        }
        insertMutex.unlock();
    }

    PLAYER PlayersManager::getPlayer(std::uint32_t id){
        if(!existPlayer(id)){
            return std::make_shared<Player>();
        }
        return players[id];
    }

    bool PlayersManager::setPosition(std::uint32_t id, float x, float y, float z){
        if(existPlayer(id)){
            PLAYER playerREF = players[id];
            playerREF->setPosition(x,y,z);
            return true;
        }
        return false;
    }

    bool PlayersManager::setAttenuation(std::uint32_t id, float new_at){
        if(existPlayer(id)){
            PLAYER playerREF = players[id];
            playerREF->setAttenuation(new_at);
            return true;
        }
        return false;
    }

    bool PlayersManager::setMinDistance(std::uint32_t id, float new_MD){
        if(existPlayer(id)){
            PLAYER playerREF = players[id];
            playerREF->minDistance(new_MD);
            return true;
        }
        return false;
    }

    bool PlayersManager::existPlayer(std::uint32_t id){
        if(players.find(id) == players.end()){
            return false;
        }
        return true;
    }

    void PlayersManager::removePlayer(std::uint32_t id){
        if (!existPlayer(id)){
            return;
        }
        players.erase(id);
    }

    void PlayersManager::clean(){
        players.clear();
    }

    void PlayersManager::playersActCheckThread(bool &canRun){
        while(canRun){
            for(auto& playerPair : players){
                playerPair.second->actCheck();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
    }

    PlayersManager::PlayersManager(){
        waitQueueAudioCount = 0;
    }

    bool PlayersManagerImpl::initialized = false;
    std::thread PlayersManagerImpl::actCheckThread = std::thread();

    bool PlayersManagerImpl::canRun = true;

    PlayersManager& PlayersManagerImpl::getInstance(){
        static PlayersManager instance;
        if(!initialized){
            actCheckThread = std::thread(&PlayersManager::playersActCheckThread, &instance, std::ref(std::ref(canRun)));
            initialized = true;
        }
        return instance;
    }

    void PlayersManagerImpl::exit(){
        if(actCheckThread.joinable()){
            canRun = false;
            actCheckThread.join();
        }
    }