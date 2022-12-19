#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>

#include "protocol.h"
#include "soundmanager.h"
#include "player.h"

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
        AudType = AudioType::LOCAL;
        coordinates.x = 0;
        coordinates.y = 0;
        coordinates.z = 0;
        coordinates.map = -1;
    }

    int Self::getMyID()
    {
        return my_id;
    }

    int Self::getMyRegID()
    {
        return my_reg_id;
    }

    AudioType Self::getAudioType(){
        return AudType;
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

    bool Self::needEncrypt()
    {
        return encrypt;
    }

    void Self::setAudioType(AudioType type)
    {
        AudType = type;
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
        sendPosInfo();
    }

    void Self::setCoords(coords coord){
        coordinates = coord;
        sendPosInfo();;
    }

    void Self::sendPosInfo(){
        data::buffer packet = protocol::tovoipserver::constructPosData(my_id, coordinates.map, coordinates.x, coordinates.y, coordinates.z);
        ConnectionImpl::getInstance().send(packet, encrypt);
    }

    coords Self::getCoords()
    {
        return coordinates;
    }

    void Self::sendAudio(data::buffer &buffer)
    {
        Self& GlobalInstance = SelfImpl::getInstance();
        data::buffer outbuffer = protocol::tovoipserver::constructAudioData(GlobalInstance.getMyID(),
            AudioTypeToChar(GlobalInstance.getAudioType()), buffer);
        ConnectionImpl::getInstance().send(outbuffer, GlobalInstance.needEncrypt());
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
                if(players.find(id) == players.end()){
                    players[id] = std::make_shared<Player>();
                    players[id]->id = id;
                    players[id]->move(x,y,z);
                }
                insertMutex.unlock();
            }

    PLAYER PlayersManager::getPlayer(int id){
                if(players.find(id) == players.end()){
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
                if (players.find(id) == players.end()){
                    return;
                }
                players[id]->mutexApplyToDelete();
                players.erase(id);
    }

    PlayersManager::PlayersManager(){
        waitQueueAudioCount = 0;
    }

    PlayersManager& PlayersManagerImpl::getInstance(){
        static PlayersManager instance;
        return instance;
    }