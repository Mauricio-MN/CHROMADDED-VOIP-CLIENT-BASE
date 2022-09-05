#ifndef PLAYER_H // include guard
#define PLAYER_H

#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>

#include "protocol.h"
#include "listen.h"

#include <SFML/Audio.hpp>

namespace players{

    typedef struct player{
        int id;
        int x;
        int y;
        int z;
        int r;
        bool isLoad = false;

        std::queue<protocol::data> audio_1;
        std::queue<protocol::data> audio_2;
        int audio_select = 1;
        std::mutex audioMutex_1;
        std::mutex audioMutex_2;

        listen::MyStream stream;
        std::mutex streamMutex;

        void refreshStream(){
            if(isReadToListen()){
                if(isLoad == false){
                    isLoad = true;
                    stream.load(player_read_sbuffer());
                } else {
                    stream.insert(player_read_sbuffer());
                }
                if(stream.getStatus() != MyStream::Playing){
                    stream.play();
                }
            }
        }

        bool isReadToListen(){
            bool out = false;
            if(audioQueueIsEmpty() == false){
                out = true;
            }else if(audioQueueIsAllEmpty() == false){
                swap();
                out = true;
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

        void swap(){
            audio_select = (audio_select == 1) ? 2 : 1;
        }

        void push(protocol::data *data){
            if(audio_select == 1){
                audioMutex_2.lock();
                audio_2.push(data);
                audioMutex_2.unlock();
            } else {
                audioMutex_1.lock();
                audio_1.push(data);
                audioMutex_1.unlock();
            }
        }

    } playerz;

    class manager{
        public:

            static void insertPlayer(int id, int x, int y, int z, int r){
                insertMutex.lock();
                if(players.find(id) == players.end()){
                    players[id] = new player;
                    players[id]->id = id;
                    players[id]->x = x;
                    players[id]->y = y;
                    players[id]->z = z;
                    players[id]->r = r;

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
                players.erase(id);
            }
        
        private:

            static std::unordered_map<int, player*> players;
            static std::mutex insertMutex;
    };



}



#endif