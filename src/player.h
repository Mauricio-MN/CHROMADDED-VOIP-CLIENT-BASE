#ifndef PLAYER_H // include guard
#define PLAYER_H

#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <memory>

#include "soundmanager.h"
#include <SFML/Audio.hpp>

#define PLAYER std::shared_ptr<Player>

namespace player{

    class Self{
    private:
        int my_id;
        int my_reg_id;
        int talkRomm;
        bool talkInLocal;
        bool encrypt;
        bool connected;
        std::mutex mutexID;
        std::mutex mutexRegID;
        Coords coordinates;

        void setMap(int map);
        void setX(int x);
        void setY(int y);
        void setZ(int z);

    public:

        Self(int reg_id, int id, bool needEncrypt);

        bool isConnected();
        void setConnect(bool isConnected);

        int getMyID();

        int getMyRegID();

        void setMyID(int id);

        void setMyRegID(int reg_id);

        bool needEncrypt();

        void setTalkRoom(int room_id);

        void talkLocal();
        void talkRoom();

        void setPos(int map, int x, int y, int z);
        void setCoords(Coords coord);
        void sendPosInfo();

        Coords getCoords();

        static void sendAudio(data::buffer &buffer);

    };

    class SelfImpl{
        private:
        static bool initialized;
        static int reg_id_;
        static int id_;
        static bool needEncrypt_;
        public:
        static Self& getInstance();

        static void frabric(int reg_id, int id, bool needEncrypt);

    };
}

class Player{

    public:

        int id;
        bool isLoad = false;
        bool echoEffect = false;
        int echoEffectValue = 1;
        int waitingCount = 0;

        soundmanager::NetworkAudioStream* soundStream;

    private:

        bool streamIsValid;

        sf::SoundBuffer sbuffer;

        std::mutex pushMutex;
        std::mutex isPlayingMutex;
        std::mutex moveMutex;
        std::mutex minDistanceMutex;
        std::mutex attenuationMutex;

        sf::Clock pushClock;

        sf::Time _sampleTime;

        void initialize(sf::Time sampleTime){
            _sampleTime = sampleTime;
            soundStream = new soundmanager::NetworkAudioStream(sampleTime, SAMPLE_CHANNELS, SAMPLE_RATE, SAMPLE_BITS);
            streamIsValid = true;
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
            deleteSoundStream();
        }


        void move(float new_x, float new_y, float new_z){
            std::lock_guard<std::mutex> guard(moveMutex);
            soundStream->setPosition(new_x, new_y, new_z);
        }

        void minDistance(float new_MD){
            std::lock_guard<std::mutex> guard(minDistanceMutex);
            soundStream->setMinDistance(new_MD);
        }

        void attenuation(float new_at){
            std::lock_guard<std::mutex> guard(attenuationMutex);
            soundStream->setAttenuation(new_at);
        }

        bool isPlaying(){
            std::lock_guard<std::mutex> guard(isPlayingMutex);
            if(soundStream->getStatus() == soundmanager::NetworkAudioStream::Playing){
                return true;
            }
            return false;
        }

        void excEchoEffect(sf::Int16 *samples, int size){
            for(int i = 0; i < size - echoEffectValue; i++){
                samples[i] = (samples[i] + samples[i+echoEffectValue]) / 2;
            }
        }

        void deleteSoundStream(){
            std::lock_guard<std::mutex> guard(pushMutex);
            soundStream->stop();
            delete soundStream;
            streamIsValid = false;
            pushClock.restart();
        }

        void actCheck(){
            if(pushClock.getElapsedTime().asSeconds() >= 10){
                deleteSoundStream();
            }
        }

        void restartSoundStream(sf::Time sampleTime){
            deleteSoundStream();
            startSoundStream(sampleTime);
        }

        void startSoundStream(sf::Time sampleTime){
            initialize(sampleTime);
            isLoad = false;
            pushClock.restart();
        }

        void push(data::buffer &audio, int sampleTime = 40){
            std::lock_guard<std::mutex> guard(pushMutex);

            if(!streamIsValid){
                startSoundStream(_sampleTime);
            }
            pushClock.restart();

            if(echoEffect){
                //excEchoEffect(samples, int16_Size);
            }

            if(isLoad == false){
                isLoad = true;
                //soundStream->load(samples, int16_Size);
                //soundStream->load(audio);
                //soundStream->setWaitBufferSize(waitQueueAudioCount);
                soundStream->receive(audio);
                soundStream->play();
            } else {
                //soundStream->insert(samples, int16_Size);
                //soundStream->insert(audio);
                soundStream->receive(audio);
                if(!isPlaying()){
                    soundStream->play();
                }
            }

            //checkPlayng();
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