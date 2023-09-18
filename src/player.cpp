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
    int SelfImpl::reg_id_ = 0;
    int SelfImpl::id_ = 0;
    bool SelfImpl::needEncrypt_ = false;

    Self &SelfImpl::getInstance()
    {
        if (initialized)
        {
            static Self instance(reg_id_, id_, needEncrypt_);
            return instance;
        } else {
            perror("fabric player::Self");
        }
    }

    void SelfImpl::frabric(int reg_id, int id, bool needEncrypt)
    {
        if(!initialized){
            reg_id_ = reg_id;
            id_ = id;
            needEncrypt_ = needEncrypt;
            initialized = true;
        }
    }

    Self::Self(int reg_id, int id, bool needEncrypt)
    {
        my_reg_id = id;
        my_id = id;
        encrypt = needEncrypt;
        talkRomm = 0;
        talkInLocal = true;
        coordinates.x = 0;
        coordinates.y = 0;
        coordinates.z = 0;
        coordinates.map = -1;
        connected = false;
    }

    int Self::getMyID()
    {
        return my_id;
    }

    int Self::getMyRegID()
    {
        return my_reg_id;
    }

    void Self::setMyID(int id)
    {
        mutexID.lock();
        my_id = id;
        mutexID.unlock();
    }

    void Self::setMyRegID(int reg_id)
    {
        mutexRegID.lock();
        my_reg_id = reg_id;
        mutexRegID.unlock();
    }

    bool Self::isConnected(){
        return connected;
    }

    void Self::setConnect(bool isConnected){
        connected = isConnected;
    }

    bool Self::needEncrypt()
    {
        return encrypt;
    }

    void  Self::setTalkRoom(int room_id){
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

    void Self::setPos(int map, int x, int y, int z)
    {
        setMap( map );
        setX( x );
        setY( y );
        setZ( z );
    }

    void Self::setCoords(Coords coord){
        coordinates = coord;
    }

    void Self::sendPosInfo(){
        protocol::Client info;
        info.set_id(my_id);
        info.set_mapnum(coordinates.map);
        info.set_coordx(coordinates.x);
        info.set_coordy(coordinates.y);
        info.set_coordz(coordinates.z);
        socketUdpImpl::getInstance().send(info);
    }

    Coords Self::getCoords()
    {
        return coordinates;
    }

    void Self::sendAudio(data::buffer &buffer)
    {
        Self& GlobalInstance = SelfImpl::getInstance();

        protocol::Client info;
        info.set_audio(buffer.getData(), buffer.size());
    }

}
    
    void PlayersManager::setWaitAudioPackets(int waitAudioPacketsCount_){
        waitQueueAudioCount = waitAudioPacketsCount_;
    }

    int PlayersManager::getWaitAudioPackets(){
        return waitQueueAudioCount;
    }

    void PlayersManager::insertPlayer(int id, float x, float y, float z){
        insertMutex.lock();
        if(existPlayer(id)){
            players[id] = std::make_shared<Player>();
            players[id]->id = id;
            players[id]->move(x,y,z);
        }
        insertMutex.unlock();
    }

    void PlayersManager::insertPlayer(PLAYER player){
        insertMutex.lock();
        if(players.find(player->id) == players.end()){
            players[player->id] = player;
        }
        insertMutex.unlock();
    }

    PLAYER PlayersManager::getPlayer(int id){
        if(existPlayer(id)){
            return players[0];
        }
        return players[id];
    }

    bool PlayersManager::movePlayer(int id, float x, float y, float z){
        if(existPlayer(id)){
            PLAYER playerREF = players[id];
            playerREF->move(x,y,z);
            return true;
        }
        return false;
    }

    bool PlayersManager::setAttenuation(int id, float new_at){
        if(existPlayer(id)){
            PLAYER playerREF = players[id];
            playerREF->attenuation(new_at);
            return true;
        }
        return false;
    }

    bool PlayersManager::setMinDistance(int id, float new_MD){
        if(existPlayer(id)){
            PLAYER playerREF = players[id];
            playerREF->minDistance(new_MD);
            return true;
        }
        return false;
    }

    bool PlayersManager::existPlayer(int id){
        if(players.find(id) == players.end()){
            return false;
        }
        return true;
    }

    void PlayersManager::removePlayer(int id){
        if (existPlayer(id)){
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
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
            actCheckThread = std::thread(&PlayersManager::playersActCheckThread, &instance, canRun);
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