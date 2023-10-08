#ifndef PLAYER_H // include guard
#define PLAYER_H

#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <memory>
#include <atomic>

#include "data.h"
#include "soundmanager.h"
#include "crpt.h"
#include "protoParse.h"
#include <SFML/Audio.hpp>

#define PLAYER std::shared_ptr<Player>

namespace player{

    class Self{
    private:
        int my_id;
        int my_secret_id;
        int talkRomm;
        int audioNumb;
        bool talkInLocal;
        bool connected;
        std::mutex mutexID;
        std::mutex mutexRegID;
        Coords coordinates;
        std::atomic<int> sampleNumber;
        AES_GCM crpt;

        void setMap(int map);
        void setX(int x);
        void setY(int y);
        void setZ(int z);

        std::mutex mutexEncrypt;
        std::mutex mutexDecrypt;

    public:

        Self(int reg_id, int id);

        bool isConnected();
        void setConnect(bool isConnected);

        int getMyID();

        int getMyRegID();

        std::vector<unsigned char> decrypt(std::vector<unsigned char>& buffer);
        std::vector<unsigned char> encrypt(std::vector<unsigned char>& buffer);

        void setCrpt(unsigned char* key, unsigned char* iv);

        void setMyID(int id);

        void setMyRegID(int reg_id);

        void setTalkRoom(int room_id);

        void talkLocal();
        void talkRoom();

        void setPos(int map, int x, int y, int z);
        void setCoords(Coords coord);
        void sendPosInfo();

        int getAndAddAudioNum();

        Coords getCoords();

        void sendConnect();

        static void sendAudio(data::buffer &buffer, int sampleTime);

    };

    class SelfImpl{
        private:
        static bool initialized;
        static int reg_id_;
        static int id_;
        static Self* instance;
        public:
        static Self& getInstance();

        static void frabric(int reg_id, int id);

    };
}

class Player{

    public:

        int id;
        bool echoEffect = false;
        int echoEffectValue = 1;
        int waitingCount = 0;

    private:

        soundmanager::NetworkAudioStream* soundStream;

        std::atomic<bool> streamIsValid;

        sf::SoundBuffer sbuffer;

        std::shared_mutex pushMutex;
        std::shared_mutex isPlayingMutex;
        std::mutex moveMutex;
        std::mutex minDistanceMutex;
        std::mutex attenuationMutex;

        sf::Clock pushClock;

        sf::Time _sampleTime;

        float attenuation;

        sf::Vector3f position;

        void initialize(sf::Time sampleTime){

            _sampleTime = sampleTime;
            soundStream = new soundmanager::NetworkAudioStream(sampleTime, SAMPLE_CHANNELS, SAMPLE_RATE, SAMPLE_BITS);
            streamIsValid = true;
            attenuation = soundStream->getAttenuation();
            pushClock.restart();
        }

    public:

        Player(){
            initialize(sf::milliseconds(SAMPLE_TIME_DEFAULT));
        }

        Player(sf::Time sampleTime){
            initialize(sampleTime);
        }

        ~Player(){
            removeImportantData();
        }

        void move(float new_x, float new_y, float new_z){
            std::lock_guard<std::mutex> guard(moveMutex);
            soundStream->setPosition(new_x, new_y, new_z);
        }

        void minDistance(float new_MD){
            std::lock_guard<std::mutex> guard(minDistanceMutex);
            soundStream->setMinDistance(new_MD);
        }

        void setAttenuation(float new_at){
            std::lock_guard<std::mutex> guard(attenuationMutex);
            attenuation = new_at;
            if(!streamIsValid.load()) return;
            soundStream->setAttenuation(new_at);
        }

        bool isPlaying(){
            if(!streamIsValid.load()) return false;
            isPlayingMutex.lock_shared();
            bool isp = false;
            if(soundStream->getStatus() == sf::SoundSource::Playing) isp = true;
            isPlayingMutex.unlock_shared();
            return isp;
        }

        void stop(){
            if(streamIsValid.load()){
                pushMutex.lock();
                if(isPlaying()){
                    soundStream->stop();
                }
                soundStream->clean();
                pushMutex.unlock();
            }
        }

        void setPosition(float x, float y, float z){
            position.x = x;
            position.y = y;
            position.z = z;
            if(!streamIsValid.load()) return;
            soundStream->setPosition(position);
        }

        sf::Vector3f getPosition(){
            return position;
        }

        void excEchoEffect(sf::Int16 *samples, int size){
            for(int i = 0; i < size - echoEffectValue; i++){
                samples[i] = (samples[i] + samples[i+echoEffectValue]) / 2;
            }
        }

        void removeImportantData(){
            if(!streamIsValid.load()) return;
            {
                pushMutex.lock();

                soundStream->stop();
                delete soundStream;
                streamIsValid = false;
                pushClock.restart();

                pushMutex.unlock();
            }
            protocolParserImpl::getInstance().getPool().stop(id);
        }

        void actCheck(){
            pushMutex.lock();
            if(pushClock.getElapsedTime().asSeconds() >= 10){
                pushMutex.unlock();
                removeImportantData();
            } else {
                pushMutex.unlock();
            }
        }

        void restartSoundStream(sf::Time sampleTime){
            removeImportantData();
            startImportantData(sampleTime);
        }

        void startImportantData(sf::Time sampleTime){
            float restoreAttenuation = attenuation;
            auto restorePosition = position;
            initialize(sampleTime);
            setAttenuation(restoreAttenuation);
            setPosition(position.x, position.y, position.z);
            pushClock.restart();
        }

        void push(int audioNum, data::buffer &audio, int sampleTime = SAMPLE_TIME_DEFAULT){
            pushMutex.lock();
            pushClock.restart();
            pushMutex.unlock();

            if(!streamIsValid.load()){
                pushMutex.lock();
                if(!streamIsValid.load()){
                    startImportantData(_sampleTime);
                }
                pushMutex.unlock();
            }
            
            if(soundStream->getSampleTime() != sampleTime){
                soundStream->resizeSampleTime(sf::milliseconds(sampleTime));
            }

            soundStream->insert(audioNum, audio);

        }

    };

    class PlayersManager
    {
    
    private:

        int waitQueueAudioCount;
        std::unordered_map<int, PLAYER> players;
        std::mutex insertMutex;

    public:

        PlayersManager();

        void setWaitAudioPackets(int waitAudioPacketsCount_);
        int getWaitAudioPackets();

        void insertPlayer(int id, float x, float y, float z);

        void insertPlayer(PLAYER player);

        PLAYER getPlayer(int id);

        bool movePlayer(int id, float x, float y, float z);

        bool setAttenuation(int id, float new_at);

        bool setMinDistance(int id, float new_MD);

        bool existPlayer(int id);

        void removePlayer(int id);

        void clean();

        void playersActCheckThread(bool &canRun);

    };

    class PlayersManagerImpl
    {
    private:
        static bool initialized;
        static std::thread actCheckThread;
        static bool canRun;
    public:
        static PlayersManager &getInstance();
        static void exit();
    };

#endif