#ifndef PLAYER_H // include guard
#define PLAYER_H

#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>

#include "protocol.h"
#include "recorder.h"

#include <SFML/Audio.hpp>

namespace players{

    inline int waitQueueAudioCount;

    typedef struct player{
        int id;
        bool isLoad = false;

        std::queue<protocol::data> audio_1;
        std::queue<protocol::data> audio_2;
        int audio_select = 1;
        std::mutex audioMutex_1;
        std::mutex audioMutex_2;

        recorder::MyStream stream;
        std::mutex streamMutex;

        std::mutex deleteMeMutex;

        void move(float new_x, float new_y, float new_z){
            stream.setPosition(new_x, new_y, new_z);
        }

        void minDistance(float new_MD){
            stream.setMinDistance(new_MD);
        }

        void attenuation(float new_at){
            stream.setAttenuation(new_at);
        }

        void refreshStream(){
            std::lock_guard<std::mutex> guard(deleteMeMutex);
            if(isReadToRep()){
                if(isLoad == false){
                    isLoad = true;
                    stream.load(player_read_sbuffer());
                } else {
                    stream.insert(player_read_sbuffer());
                }
                if(stream.getStatus() != recorder::MyStream::Playing){
                    stream.play();
                }
            }
        }

        bool isReadToRep(){
            bool out = false;
            if(audioQueueIsEmpty() == false){
                out = true;
            }else if(audioQueueIsAllEmpty() == false){
                if(audioChargingQueueSize() < waitQueueAudioCount){
                    out = false;
                } else {
                    swap();
                    out = true;
                }
            } else {
                out = false;
            }
            return out;
        }

        sf::SoundBuffer player_read_sbuffer(){
            protocol::data data = player_read();

            int int16_Size = (sizeof(char) * data.size) / 2;
            const sf::Int16* samples = new sf::Int16[int16_Size];
            protocol::tools::bufferToData<const sf::Int16>(samples, data.size, data.buffer);
            sf::SoundBuffer sbuffer;
            sbuffer.loadFromSamples(samples, int16_Size, SAMPLE_CHANNELS, SAMPLE_RATE);
            return sbuffer;
        }

        protocol::data player_read(){
            protocol::data out;

            if(audio_select == 1){
             audioMutex_1.lock();
             out = audio_1.front();
             audio_1.pop();
             audioMutex_1.unlock();
            } else {
             audioMutex_2.lock();
             out = audio_2.front();
             audio_2.pop();
             audioMutex_2.unlock();
            }
            
            return out;
        }

        bool audioQueueIsEmpty(){
            bool out = true;
            if(audio_select == 1){
                out = audio_1.size() <= 0;
            } else {
                out = audio_2.size() <= 0;
            }
            return out;
        }

        bool audioQueueIsAllEmpty(){
            bool out = true;
            bool out_1 = audio_1.size() <= 0;
            bool out_2 = audio_1.size() <= 0;
            out = out_1 && out_2;
            return out;
        }

        int audioChargingQueueSize(){
            int out = 0;
            if(audio_select == 1){
                out = audio_2.size();
            } else {
                out = audio_1.size();
            }
            return out;
        }

        void swap(){
            audio_select = (audio_select == 1) ? 2 : 1;
        }

        void push(protocol::data *data){
            if(audio_select == 1){
                audioMutex_2.lock();
                audio_2.push(*data);
                audioMutex_2.unlock();
            } else {
                audioMutex_1.lock();
                audio_1.push(*data);
                audioMutex_1.unlock();
            }
        }

    } playerz;

    void init(int waitAudioPackets){
        waitQueueAudioCount = waitAudioPackets;
    }
    void setWaitAudioPackets(int waitAudioPackets){
        waitQueueAudioCount = waitAudioPackets;
    }

    class manager{
        public:

            static void insertPlayer(int id, float x, float y, float z){
                insertMutex.lock();
                if(players.find(id) == players.end()){
                    players[id] = new player;
                    players[id]->id = id;
                    players[id]->move(x,y,z);
                }
                insertMutex.unlock();
            }

            static bool getPlayer(int id, player * playerREF){
                if(players.find(id) == players.end()){
                    return false;
                }
                playerREF = players[id];
                return true;
            }

            static bool movePlayer(int id, float x, float y, float z){
                if(existPlayer(id)){
                    player *playerREF = players[id];
                    playerREF->move(x,y,z);
                    return true;
                }
                return false;
            }

            static bool setAttenuation(int id, float new_at){
                if(existPlayer(id)){
                    player *playerREF = players[id];
                    playerREF->attenuation(new_at);
                    return true;
                }
                return false;
            }

            static bool setMinDistance(int id, float new_MD){
                if(existPlayer(id)){
                    player *playerREF = players[id];
                    playerREF->minDistance(new_MD);
                    return true;
                }
                return false;
            }

            static bool existPlayer(int id){
                if(players.find(id) == players.end()){
                    return false;
                }
                return true;
            }

            static void removePlayer(int id){
                if (players.find(id) == players.end()){
                    return;
                }
                std::lock_guard<std::mutex> guard(players[id]->deleteMeMutex);
                players.erase(id);
            }

            static void processDatas(){
                while(true){
                if(players.size() > 0){
                    for (auto& it: players) {
                        it.second->refreshStream();
                    }
                } else {
                    sf::sleep(sf::milliseconds(1));
                }
                }
            }
        
        private:

            static std::unordered_map<int, player*> players;
            static std::mutex insertMutex;
    };



}



#endif