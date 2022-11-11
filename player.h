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

namespace players{

    inline int waitQueueAudioCount;

    class player{

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

        player(){
            stream = new soundmanager::NetworkAudioStream();
        }

        ~player(){
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

        void push(data::buffer* audio){
            std::lock_guard<std::mutex> guard(deleteMePushMutex);

            protocol::tools::bufferToData(samples, audio->size, audio->getBuffer());

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
            }

            //checkPlayng();
        }

    };

    void init(int waitAudioPacketsCount_);
    
    void setWaitAudioPackets(int waitAudioPacketsCount_);

    class manager{
        public:

            static void init();

            static void insertPlayer(int id, float x, float y, float z);

            static player* getPlayer(int id);

            static bool movePlayer(int id, float x, float y, float z);

            static bool setAttenuation(int id, float new_at);

            static bool setMinDistance(int id, float new_MD);

            static bool existPlayer(int id);

            static void removePlayer(int id);
        
        private:

            static inline std::unordered_map<int, player*> players;
            static inline std::mutex insertMutex;
    };

    class self{
    private:
        static int my_id;
        static bool encrypt;
        static AudioType AudType;
        static std::mutex mutexID;
        static coords coordinates;
        static bool isConnected;
    public:
        static void init(int id, bool needEncrypt){
            my_id = id;
            encrypt = needEncrypt;
            AudType = AudioType::LOCAL;
            coordinates.x = 0;
            coordinates.y = 0;
            coordinates.z = 0;
        }

        static int getMyID(){
            return my_id;
        }

        static void setMyID(int id){
            mutexID.lock();
            my_id = id;
            mutexID.unlock();
        }

        static bool needEncrypt(){
            return encrypt;
        }

        static void setAudioType(AudioType type){
            AudType = type;
        }

        static void setX(int x){ coordinates.x = x; }
        static void setY(int y){ coordinates.y = y; }
        static void setZ(int z){ coordinates.z = z; }
        static void setCoords(int x, int y, int z){
            coordinates.x = x;
            coordinates.y = y;
            coordinates.z = z;
        }

        static coords getCoords(){
            return coordinates;
        }

        static void sendAudio(char* buffer, int size){
            data::buffer databuff(buffer, size);
            data::buffer outbuffer = protocol::tovoipserver::constructAudioData(my_id, AudioTypeToChar(AudType), &databuff);
            connection::send(outbuffer.getBuffer(), outbuffer.size, encrypt);
        }

    };



}



#endif