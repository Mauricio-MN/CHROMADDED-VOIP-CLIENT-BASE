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

#include "recorder.h"

#include "main.h"

void updateMyPos(float x, float y, float z){ recorder::movePos(x,y,z); }
void updateMyRot(float x, float y, float z){ recorder::moveRot(x,y,z); }


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
void removePlayer(int id){ 
  players::manager::removePlayer(id);
}
void updateWaitAudioPackets(int pktWait){
  players::setWaitAudioPackets(pktWait);
}


void init(int id, unsigned char *key, float x, float y, float z, float oneCoordinateCorrespondsToNMeters){
  if(initialized == false){
    initialized = true;

    unsigned char *keyC = new unsigned char[16];
    protocol::tools::bufferToData<unsigned char>(keyC, 16, (char*)key);
    crypt::init(keyC);

    protocol::init();

    bufferparser::init(id);

    std::async(connection::init, id);

    players::init(3);

    recorder::movePos(x,y,z);
    recorder::startRecorder();

    
  }
}

int calculateNumbers(int a, int b){
  
}


int main(){

  printf("end \n");
    return 0;

}