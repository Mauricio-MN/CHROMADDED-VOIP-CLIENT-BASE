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
#include <vector>

#define PLAYER std::shared_ptr<Player>

namespace player{

    class Self{
    private:
        std::uint32_t my_id;
        std::uint32_t my_secret_id;
        std::uint32_t talkRoom;
        std::atomic_uint32_t audioNumb;
        std::atomic_bool talkInLocal;
        std::atomic_bool talkInGroup;
        std::atomic_bool listenLocal;
        std::atomic_bool listenGroup;
        std::atomic_bool connected;
        std::mutex mutexID;
        std::mutex mutexRegID;
        Coords coordinates;
        std::atomic<int> sampleNumber;
        AES_GCM crpt;

        std::mutex mutexEncrypt;
        std::mutex mutexDecrypt;

        std::atomic_bool needCallNewSession;

        std::atomic_bool localMute;

        void constructPosInfo(protocol::Client* info);

    public:

        Self(std::uint32_t reg_id, std::uint32_t id);

        //thread safe
        bool isConnected();
        //thread safe
        void setConnect(bool isConnected);

        //thread safe
        std::uint32_t getMyID();
        //thread safe
        std::uint32_t getMyRegID();

        std::vector<unsigned char> decrypt(std::vector<unsigned char>& buffer);
        std::vector<unsigned char> encrypt(std::vector<unsigned char>& buffer);

        void setCrpt(std::vector<char> key, std::vector<char> iv);

        void setMyID(std::uint32_t id);

        void setMyRegID(std::uint32_t reg_id);

        void setTalkRoom(std::uint32_t room_id);

        void enableListenLocal();
        void disableListenLocal();
        void enableListenGroup();
        void disableListenGroup();

        void enableTalkLocal();
        void disableTalkLocal();
        void enableTalkRoom();
        void disableTalkRoom();

        void setMap(std::uint32_t map);
        void setX(float x);
        void setY(float y);
        void setZ(float z);
        
        void setPos(std::uint32_t map, float x, float y, float z);
        void setCoords(Coords& coord);
        void sendPosInfo();

        int getAndAddAudioNum();

        SimpleCoords getCoords();

        //A little slow, thread safe
        sf::Vector3f getCoords3f();

        void sendConnect();

        static void sendAudio(data::buffer &buffer, int sampleTime);

        //thread safe
        void setNeedCallNewSession(bool value);

        bool isNeedCallNewSession();

    };

    class SelfImpl{
        private:
        static bool initialized;
        static std::uint32_t reg_id_;
        static std::uint32_t id_;
        static Self* instance;
        public:
        static Self& getInstance();

        static bool isStarted();

        //can refabric
        static void fabric(std::uint32_t reg_id, std::uint32_t id);
        static void close();
    };
}

class Player{

    public:

        std::uint32_t id;
        int waitingCount = 0;

    private:

        soundmanager::NetworkAudioStream* soundStream;

        std::atomic<bool> streamIsValid;
        std::atomic<bool> isOnGroup;
        std::atomic<bool> isOnGroup_switch;
        std::atomic<bool> echoIsOn;
        std::atomic<int> echoForce;
        std::atomic<int> echoDelay;
        std::vector<std::unique_ptr<soundmanager::NetworkAudioStream>> echoBuffers;
        std::mutex echoBuffer_Mutex;
        std::mutex groupChange_Mutex;

        sf::SoundBuffer sbuffer;

        std::shared_mutex pushMutex;
        std::mutex minDistanceMutex;
        std::mutex attenuationMutex;

        sf::Clock pushClock;

        sf::Time _sampleTime;

        float attenuation;

        Coords coords;

        void initialize(sf::Time sampleTime){
            echoIsOn = false;
            echoForce = 0;
            echoDelay = 0;
            echoBuffers.resize(0);

            _sampleTime = sampleTime;
            soundStream = new soundmanager::NetworkAudioStream(sampleTime, SAMPLE_CHANNELS, SAMPLE_RATE, SAMPLE_BITS);
            streamIsValid = true;
            attenuation = soundStream->getAttenuation();
            pushClock.restart();
            soundStream->play();
        }

        std::vector<float> progressiveWalk(float total, int div){
            std::vector<float> result;
            float totalAttenuation = total; // Atenuação total (decimal)
            int rep = 5; // Número de repetições
            float res = totalAttenuation / (rep * (rep + 1) / 2.0); // Calcula a razão de atenuação
            float actualAttenuation = totalAttenuation; // Atenuação atual iniciada com a atenuação total
            for (int i = 1; i <= rep; ++i) {
                float repAttenuation = i * res; // Calcula a atenuação para esta repetição
                actualAttenuation -= repAttenuation; // Reduz a atenuação atual pela atenuação desta repetição
                result.push_back(repAttenuation);
            }
            return result;
        }

        std::vector<float> progressiveAttenuationInverse(float attenuation, int div){
            std::vector<float> result;
            for (int i = div; i > 0; --i) {
                float attenuationRep = attenuation / (float)i; // Calcula a atenuação para esta repetição
                attenuation -= attenuationRep; // Reduz a atenuação atual pela atenuação desta repetição
                result.push_back(attenuationRep);
            }
            return result;
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

        //thread safe
        bool isPlaying(){
            if(!streamIsValid.load()) return false;
            return soundStream->isPlaying();
        }

        //thread safe
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
            coords.setX(x);
            coords.setY(y);
            coords.setZ(z);
            if(!streamIsValid.load()) return;
            setToListenGroup(false);
            sf::Vector3f position(x,y,z);
            soundStream->setPosition(position);
        }

        void setToListenGroup(bool value){
            if(!streamIsValid.load()) return;
            if(isOnGroup.load() != value){
                isOnGroup = value;
                isOnGroup_switch = true;
            }
        }

        SimpleCoords getPosition(){
            return coords.getCoord();
        }

        void enableEcho(int _echoForce, int _echoDelay){
            echoForce = _echoForce;
            echoDelay = _echoDelay;
            if(!streamIsValid.load()) return;
            
            if(!echoIsOn.load()){
                echoBuffer_Mutex.lock();
                    int delay = echoDelay;
                    int samples = sampleTimeGetCount(delay, SAMPLE_RATE);
                    if (samples < 320) samples = 320;

                    std::vector<float> pVolume = progressiveWalk(100.0f, echoForce);
                    int samplesR = samples;
                    echoBuffers.clear();
                    for (int i = 0; i < echoForce; i++){
                        std::unique_ptr<soundmanager::NetworkAudioStream>  
                        ptr = std::make_unique<soundmanager::NetworkAudioStream>(sf::milliseconds(SAMPLE_TIME_DEFAULT), SAMPLE_CHANNELS, SAMPLE_RATE, SAMPLE_BITS);
                        echoBuffers.push_back(std::move(ptr));
                        //echoBuffers.push_back(OVNetworkAudioStream(sf::milliseconds(SAMPLE_TIME_DEFAULT), sampleChannels, sampleRate, sampleBits));
                        //echoBuffers.emplace_back(sf::milliseconds(SAMPLE_TIME_DEFAULT), sampleChannels, sampleRate, sampleBits);
                    }
                    int j = 0;
                    for(auto& echoBuffer : echoBuffers){
                        echoBuffer.get()->setDelaySamplesPlayData(samplesR);
                        float volume = 1.0f;
                        if (pVolume.size() > j){
                            volume = pVolume[j];
                        }
                        //echoBuffer.get()->setVolume(volume);
                        int msSamples = 20;
                        data::buffer tempData(sampleTimeGetCount(msSamples, SAMPLE_RATE));
                        echoBuffer.get()->insert(0, tempData, msSamples);
                        echoBuffer.get()->play();
                        samplesR = samplesR + samples;
                        j++;
                    }

                echoBuffer_Mutex.unlock();
                echoIsOn = true;
            }

        }

        void disableEcho(){
            if(!streamIsValid.load()) return;
            if(echoIsOn.load()){
                echoBuffer_Mutex.lock();
                            
                    for(auto& echoBuffer : echoBuffers){
                        echoBuffer.get()->stop();
                    }
                    echoBuffers.clear();

                    echoIsOn = false;

                echoBuffer_Mutex.unlock();
            }

        }

        void removeImportantData(){
            if(!streamIsValid.load()) return;
            {
                pushMutex.lock();
                std::cout << "Stop " << id << std::endl;
                soundStream->stop();
                delete soundStream;
                streamIsValid = false;
                pushClock.restart();

                pushMutex.unlock();
            }
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
            auto restorePosition = coords.getCoord();

            initialize(sampleTime);
            setAttenuation(restoreAttenuation);
            setPosition(restorePosition.x, restorePosition.y, restorePosition.z);

            pushClock.restart();
        }

        //thread safe
        void push(int audioNum, data::buffer &audio, int sampleTime = SAMPLE_TIME_DEFAULT){

            if(!streamIsValid.load()){
                pushMutex.lock();
                if(!streamIsValid.load()){
                    pushClock.restart();
                    startImportantData(_sampleTime);
                }
                pushMutex.unlock();
            }

            if(isOnGroup_switch.load()){
                isOnGroup_switch = false;
                if(groupChange_Mutex.try_lock()){
                    if(isOnGroup.load()){
                        soundStream->setPosition({0.0f, 0.0f, 0.0f});
                        soundStream->setRelativeToListener(true);
                    } else {
                        soundStream->setRelativeToListener(false);
                    }
                }
            }
            
            /*
            if(soundStream->getSampleTime() != sampleTime){
                soundStream->resizeSampleTime(sf::milliseconds(sampleTime));
            }*/

            soundStream->insert(audioNum, audio, sampleTime);

            if(echoIsOn.load()){
                if(echoBuffer_Mutex.try_lock()){
                    for(auto& echoBuffer : echoBuffers){
                        data::buffer dataP(audio.getData(), audio.size());
                        echoBuffer.get()->insert(audioNum, dataP, sampleTime);
                    }
                    echoBuffer_Mutex.unlock();
                }
            }

            if(pushMutex.try_lock()){
                pushClock.restart();
                pushMutex.unlock();
            }

        }

    };

    class PlayersManager
    {
    
    private:

        int waitQueueAudioCount;
        std::unordered_map<int, PLAYER> players;
        std::shared_mutex syncMutex;

    public:

        PlayersManager();

        //thread safe
        void insertPlayer(std::uint32_t id, float x, float y, float z);

        //thread safe
        void insertPlayer(PLAYER player);

        //thread safe
        PLAYER getPlayer(std::uint32_t id);

        //trhead safe
        std::vector<PLAYER> getAllPlayers();

        //Only one thread by player to be thread safe
        bool setPosition(std::uint32_t id, float x, float y, float z);

        //Only one thread by player to be thread safe
        bool setAttenuation(std::uint32_t id, float new_at);

        //Only one thread by player to be thread safe
        bool setMinDistance(std::uint32_t id, float new_MD);

        //thread safe
        bool existPlayer(std::uint32_t id);

        //UNSAFE - thread unsafe
        bool existPlayerUnsafe(std::uint32_t id);

        //thread safe
        void removePlayer(std::uint32_t id);

        void clean();

        void playersActCheckThread(bool &canRun);

    };

    class PlayersManagerImpl
    {
    private:
        static bool initialized;
        static std::thread actCheckThread;
        static bool canRun;
        static std::mutex syncMutex;
    public:
        static PlayersManager &getInstance();
        static void close();
    };

#endif