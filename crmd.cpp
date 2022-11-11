#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
//#include "../libs/SDL2/SDL.h"
#include <sys/time.h>
#include <ctime>
#include <queue>
#include <thread>
#include <mutex>
#include <future>

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "SoundCustomBufferRecorder.hpp"

//#include "../libs/AL/al.h"
//#include "../libs/AL/alc.h"

#include "cript.h"

#include "protocol.h"

#include "player.h"

#include "bufferParser.h"

#include "connection.h"

#include "soundmanager.h"

#include "crmd.h"

namespace crmd{

void updateMyPos(float x, float y, float z){ soundmanager::listener::movePos(x,y,z); }
void updateMyRot(float x, float y, float z){ soundmanager::listener::moveRot(x,y,z); }


void insertPlayer(float id, float x, float y, float z){
  players::manager::insertPlayer(id,x,y,z);
  }

void movePlayer(float id, float x, float y, float z){
  players::manager::movePlayer(id,x,y,z);
}
void updatePlayerAttenuation(int id, float new_at){
  players::manager::setAttenuation(id, new_at);
}
void updatePlayerMinDistance(int id, float new_MD){
  players::manager::setMinDistance(id, new_MD);
}
void enablePlayerEchoEffect(int id){
  if(players::manager::existPlayer(id)){
    players::manager::getPlayer(id)->echoEffect = true;
  }
}
void disablePlayerEchoEffect(int id){
  if(players::manager::existPlayer(id)){
    players::manager::getPlayer(id)->echoEffect = true;
  }
}
void updatePlayerEchoEffect(int id, int value){
  if(value > 0){
    if(players::manager::existPlayer(id)){
      players::manager::getPlayer(id)->echoEffectValue = value;
    }
  }
}
void removePlayer(int id){ 
  players::manager::removePlayer(id);
}
void updateWaitAudioPacketsCount(int pktWaitCount){
  players::setWaitAudioPackets(pktWaitCount);
}

void setVolumeAudio(float volume){
  sf::Listener::setGlobalVolume(volume);
}

bool isConnected(){
  return connection::getIsConnected();
}


void init(int register_id, int id, char* ip, int ip_size, unsigned char *key, float x, float y, float z, bool needEncrypt){
  if(initialized == false){
    initialized = true;

    players::self::init(id, needEncrypt);

    unsigned char *keyC = new unsigned char[16];
    protocol::tools::bufferToData(keyC, 16, (char*)key);
    crypt::init(keyC);
    delete []keyC;

    protocol::init();

    players::init(3);

    bufferparser::init();

    soundmanager::listener::movePos(x,y,z);
    soundmanager::recorder::init();

    //std::async(connection::init, id, ip, ip_size);
    connection::init(ip, ip_size);

    delete []keyC;
    
  }
}

}