#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>

#include "protocol.h"
#include "soundmanager.h"
#include "player.h"

#include <SFML/Audio.hpp>

namespace players{

    void init(int waitAudioPacketsCount_){
        waitQueueAudioCount = waitAudioPacketsCount_;
    }
    void setWaitAudioPackets(int waitAudioPacketsCount_){
        waitQueueAudioCount = waitAudioPacketsCount_;
    }

    void manager::init(){
        insertPlayer(0,-1,-1,-1);
        players[0]->stream->setVolume(0);
    }

    void manager::insertPlayer(int id, float x, float y, float z){
                insertMutex.lock();
                if(players.find(id) == players.end()){
                    players[id] = new player;
                    players[id]->id = id;
                    players[id]->move(x,y,z);
                }
                insertMutex.unlock();
            }

    player* manager::getPlayer(int id){
                if(players.find(id) == players.end()){
                    return players[0];
                }
                return players[id];
            }

    bool manager::movePlayer(int id, float x, float y, float z){
                if(existPlayer(id)){
                    player *playerREF = players[id];
                    playerREF->move(x,y,z);
                    return true;
                }
                return false;
            }

    bool manager::setAttenuation(int id, float new_at){
                if(existPlayer(id)){
                    player *playerREF = players[id];
                    playerREF->attenuation(new_at);
                    return true;
                }
                return false;
            }

    bool manager::setMinDistance(int id, float new_MD){
                if(existPlayer(id)){
                    player *playerREF = players[id];
                    playerREF->minDistance(new_MD);
                    return true;
                }
                return false;
            }

    bool manager::existPlayer(int id){
                if(players.find(id) == players.end()){
                    return false;
                }
                return true;
            }

    void manager::removePlayer(int id){
                if (players.find(id) == players.end()){
                    return;
                }
                players[id]->mutexApplyToDelete();
                players.erase(id);
            }
}