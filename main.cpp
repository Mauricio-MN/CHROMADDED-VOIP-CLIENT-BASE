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

#include "listen.h"

void updateMyPos(int x, int y, int z, int r){ listen::movePos(x,y,z,r); }


void init(int id, unsigned char *key){

  unsigned char *keyC = new unsigned char[16];
  protocol::tools::bufferToData<unsigned char>(keyC, 16, (char*)key);
  crypt::init(keyC);

  protocol::init();

  bufferparser::init(id);

  listen::movePos(0,0,0,0);
  listen::startListen();

  connection::init(id);
  
}


int main(){

  printf("end \n");
    return 0;

}