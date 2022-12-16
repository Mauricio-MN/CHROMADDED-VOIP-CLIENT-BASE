#ifndef PLAYER_H // include guard
#define PLAYER_H

#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <memory>

#include "protocol.h"
#include "soundmanager.h"
#include "globaldefs.h"
#include <SFML/Audio.hpp>


namespace player{

    class Self{
    private:
        int my_id;
        int my_reg_id;
        bool encrypt;
        AudioType AudType;
        std::mutex mutexID;
        std::mutex mutexRegID;
        coords coordinates;

        void setMap(int map);
        void setX(int x);
        void setY(int y);
        void setZ(int z);

    public:

        Self(int reg_id, int id, bool needEncrypt);

        int getMyID();

        int getMyRegID();

        void setMyID(int id);

        void setMyRegID(int reg_id);

        bool needEncrypt();

        void setAudioType(AudioType type);
        AudioType getAudioType();

        void setPos(int map, int x, int y, int z);
        void setCoords(coords coord);
        void sendPosInfo();

        coords getCoords();

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

        soundmanager::NetworkAudioStream* stream;

    private:

        int int16_Size = SAMPLE_COUNT;
        sf::Int16* samples = new sf::Int16[int16_Size];

        sf::SoundBuffer sbuffer;

        std::mutex deleteMePushMutex;
        std::mutex deleteMeIsPlayingMutex;
        std::mutex deleteMeMoveMutex;
        std::mutex deleteMeMinDistanceMutex;
        std::mutex deleteMeAttenuationMutex;

    public:

        Player(){
            stream = new soundmanager::NetworkAudioStream();
        }

        ~Player(){
        }

        void mutexApplyToDelete(){
            deleteMePushMutex.lock();
            deleteMeIsPlayingMutex.lock();
            deleteMeMoveMutex.lock();
            deleteMeMinDistanceMutex.lock();
            deleteMeAttenuationMutex.lock();

            delete []samples;
            delete stream;
        }


        void move(float new_x, float new_y, float new_z){
            std::lock_guard<std::mutex> guard(deleteMeMoveMutex);
            stream->setPosition(new_x, new_y, new_z);
        }

        void minDistance(float new_MD){
            std::lock_guard<std::mutex> guard(deleteMeMinDistanceMutex);
            stream->setMinDistance(new_MD);
        }

        void attenuation(float new_at){
            std::lock_guard<std::mutex> guard(deleteMeAttenuationMutex);
            stream->setAttenuation(new_at);
        }

        bool isPlaying(){
            std::lock_guard<std::mutex> guard(deleteMeIsPlayingMutex);
            if(stream->getStatus() == soundmanager::NetworkAudioStream::Playing){
                return true;
            }
            return false;
        }

        void excEchoEffect(sf::Int16 *samples, int size){
            for(int i = 0; i < size - echoEffectValue; i++){
                samples[i] = (samples[i] + samples[i+echoEffectValue]) / 2;
            }
        }

        void push(data::buffer &audio){
            std::lock_guard<std::mutex> guard(deleteMePushMutex);

            protocol::tools::bufferToData(samples, audio.size(), audio.getData());

            if(echoEffect){
                excEchoEffect(samples, int16_Size);
            }

            if(isLoad == false){
                isLoad = true;
                //stream->load(samples, int16_Size);
                //stream->load(audio);
                //stream->setWaitBufferSize(waitQueueAudioCount);
                stream->receive(audio);
                stream->play();
            } else {
                //stream->insert(samples, int16_Size);
                //stream->insert(audio);
                stream->receive(audio);
                if(!isPlaying()){
                    stream->play();
                }
            }

            //checkPlayng();
        }

    };

    class PlayersManager
    {
    
    private:

        int waitQueueAudioCount;
        std::unordered_map<int, Player *> players;
        std::mutex insertMutex;

    public:

        PlayersManager();

        void setWaitAudioPackets(int waitAudioPacketsCount_);
        int getWaitAudioPackets();

        void insertPlayer(int id, float x, float y, float z);

        Player *getPlayer(int id);

        bool movePlayer(int id, float x, float y, float z);

        bool setAttenuation(int id, float new_at);

        bool setMinDistance(int id, float new_MD);

        bool existPlayer(int id);

        void removePlayer(int id);

    };

    class PlayersManagerImpl
    {
    public:
        static PlayersManager &getInstance();
    };

#endif