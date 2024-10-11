#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>

#include "soundmanager.h"
#include "player.h"
#include "socketUdp.h"

#include "protoParse.h"

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


    void SelfImpl::fabric(std::uint32_t reg_id, std::uint32_t id)
    {
        reg_id_ = reg_id;
        id_ = id;
        if(!initialized){
            instance = new Self(reg_id_, id_);
            initialized = true;
        } else {
            instance->setMyID(id);
            instance->setMyRegID(reg_id);
        }
    }

    void SelfImpl::close(){
        if(initialized){
            delete instance;
            initialized = false;
        }
    }

    bool SelfImpl::isStarted(){
        return initialized;
    }

    Self::Self(std::uint32_t reg_id, std::uint32_t id) : crpt NEW_AES_GCM
    {
        my_secret_id = reg_id;
        my_id = id;
        talkRoom = 0;
        talkInLocal = true;
        talkInGroup = false;
        listenLocal = true;
        listenGroup = false;
        coordinates.setCoords(-1,0,0,0);
        connected = false;
        sampleNumber = 0;
        audioNumb = 0;
        needCallNewSession = false;
        localMute = false;
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

    void Self::setCrpt(std::vector<char> key, std::vector<char> iv){
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
        return connected.load();
    }

    void Self::setConnect(bool isConnected){
        connected = isConnected;
    }

    void  Self::setTalkRoom(std::uint32_t room_id){
        talkRoom = room_id;
    }

    void Self::enableListenLocal(){
        listenLocal = true;
    }
    void Self::disableListenLocal(){
        listenLocal = false;
    }
    void Self::enableListenGroup(){
        listenGroup = true;
    }
    void Self::disableListenGroup(){
        listenGroup = false;
    }

    void Self::enableTalkLocal(){
        talkInLocal = true;
    }
    void Self::disableTalkLocal(){
        talkInLocal = false;
    }
    void Self::enableTalkRoom(){
        talkInGroup = true;
    }
    void Self::disableTalkRoom(){
        talkInGroup = false;
    }

    void Self::setX(float x) { coordinates.setX(x); }
    void Self::setY(float y) { coordinates.setY(y); }
    void Self::setZ(float z) { coordinates.setZ(z); }

    void Self::setMap(std::uint32_t map){
        coordinates.setMap(map);
    }

    void Self::setPos(std::uint32_t map, float x, float y, float z)
    {
        coordinates.setCoords(map,x,y,z);
    }

    void Self::setCoords(Coords& coord){
        coordinates.setCoords(coord);
    }

    SimpleCoords Self::getCoords()
    {
        return coordinates.getCoord();
    }

    sf::Vector3f Self::getCoords3f(){
        return sf::Vector3f(coordinates.getX(), coordinates.getY(), coordinates.getZ());
    }

    int Self::getAndAddAudioNum(){
        int result = audioNumb;
        audioNumb++;
        if(audioNumb.load() >= 256) audioNumb = 0;
        return result;
    }

    void Self::constructPosInfo(protocol::Client* info){
        info->set_mapnum(coordinates.getMap());
        info->set_coordx(coordinates.getX());
        info->set_coordy(coordinates.getY());
        info->set_coordz(coordinates.getZ());
    }

    void Self::sendPosInfo(){
        if(isConnected()){
            protocol::ClientBase cb;
            protocol::Client* info = cb.mutable_clientext();
            info->set_id(my_id);
            constructPosInfo(info);
            socketUdpImpl::getInstance().send(cb);
        }
    }

    void Self::sendConnect(){
        protocol::ClientBase cb;
        protocol::Client* info = cb.mutable_clientext();

        info->set_id(my_id);
        info->set_secret_id(my_secret_id);
        google::protobuf::Timestamp* timestamp = info->mutable_packettime();
        using namespace std::chrono;
        high_resolution_clock::time_point timePoint = high_resolution_clock::now();
        auto duration = duration_cast<nanoseconds>(timePoint.time_since_epoch());
        timestamp->set_seconds(duration_cast<seconds>(duration).count());
        timestamp->set_nanos(duration.count() % 1'000'000'000);

        socketUdpImpl::getInstance().send(cb);

        system_clock::time_point currentTimePoint = time_point_cast<system_clock::duration>(timePoint);
        ProtocolParser::setTimeConnectSend(currentTimePoint);
        std::cout << "Recalcule time server/client" << std::endl;
    }

    void Self::sendAudio(data::buffer &buffer, int sampleTime)
    {
        if(SelfImpl::getInstance().isConnected()){
            protocol::ClientBase cb;
            protocol::ClientAudio* infoAud = cb.mutable_clientextaudio();
            infoAud->set_audio(buffer.getData(), buffer.size());
            int audioNum = SelfImpl::getInstance().getAndAddAudioNum();
            infoAud->set_audionum(audioNum);
            infoAud->set_sampletime(sampleTime);
            //se for par medir ping
            if(audioNum % 2 == 0){
                /*
                google::protobuf::Timestamp* timestamp = info->mutable_packettime();
                using namespace std::chrono;
                high_resolution_clock::time_point timePoint = high_resolution_clock::now();

                //server no passado
                if(ProtocolParser::isNegativeTimeDiffServer){
                    timePoint -= ProtocolParser::getTimeDiffServer();
                } else {
                //server no futuro
                    timePoint += ProtocolParser::getTimeDiffServer();
                }

                auto duration = duration_cast<nanoseconds>(timePoint.time_since_epoch());
                timestamp->set_seconds(duration_cast<seconds>(duration).count());
                timestamp->set_nanos(duration.count() % 1'000'000'000);*/
            }
            //std::cout << "SAudio: " << info->audionum() << " : " << sampleTimeGetCount(sampleTime, SAMPLE_RATE) << std::endl;
            socketUdpImpl::getInstance().send(cb);
            if(audioNum == 0){
                protocol::ClientBase cb_;
                protocol::Client* info_ = cb_.mutable_clientext();
                SelfImpl::getInstance().constructPosInfo(info_);
                socketUdpImpl::getInstance().send(cb_);
                SelfImpl::getInstance().sendConnect();
            }
        }
    }

    void Self::setNeedCallNewSession(bool value){
        needCallNewSession = value;
    }

    bool Self::isNeedCallNewSession(){
        return needCallNewSession.load();
    }

}

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

    std::vector<PLAYER> PlayersManager::getAllPlayers(){
        std::vector<PLAYER> result;
        syncMutex.lock_shared();
        for(std::pair<int, PLAYER> player : players){
            result.push_back(player.second);
        }
        syncMutex.unlock_shared();
        return result;
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

    void PlayersManagerImpl::close(){
        if(actCheckThread.joinable()){
            canRun = false;
            actCheckThread.join();
        }
    }