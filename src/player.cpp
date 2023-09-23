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

    Self &SelfImpl::getInstance()
    {
        if (initialized)
        {
            static Self instance(reg_id_, id_);
            return instance;
        } else {
            perror("fabric player::Self");
        }
    }

    void SelfImpl::frabric(int reg_id, int id)
    {
        if(!initialized){
            reg_id_ = reg_id;
            id_ = id;
            initialized = true;
        }
    }

    Self::Self(int reg_id, int id) : crpt NEW_AES_GCM
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

    int Self::getMyID()
    {
        return my_id;
    }

    int Self::getMyRegID()
    {
        return my_secret_id;
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
        my_secret_id = reg_id;
        mutexRegID.unlock();
    }

    bool Self::isConnected(){
        return connected;
    }

    void Self::setConnect(bool isConnected){
        connected = isConnected;
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

    void Self::sendAudio(data::buffer &buffer)
    {
        protocol::Client info;
        info.set_audio(buffer.getData(), buffer.size());
        info.set_audionum(SelfImpl::getInstance().getAndAddAudioNum());

        socketUdpImpl::getInstance().send(info);
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
        if(!existPlayer(id)){
            players[id] = std::make_shared<Player>();
            players[id]->id = id;
            players[id]->move(x,y,z);
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

    PLAYER PlayersManager::getPlayer(int id){
        if(!existPlayer(id)){
            return std::make_shared<Player>();
            players[id]->id = -1;
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