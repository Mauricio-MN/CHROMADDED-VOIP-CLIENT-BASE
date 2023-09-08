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

#include "player.h"

#include "socketUdp.h"

#include "soundmanager.h"

#include "crmd.h"
#include "crmdprivate.h"

namespace crmd{

void updateMyPos(int map, float x, float y, float z){
  soundmanager::listener::movePos(x,y,z); 
  player::SelfImpl::getInstance().setPos(map, static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
}

void updateMyRot(float x, float y, float z){ 
  soundmanager::listener::moveRot(x,y,z); 
}

void insertPlayer(float id, float x, float y, float z){
  PlayersManagerImpl::getInstance().insertPlayer(id,x,y,z);
  }

void movePlayer(float id, float x, float y, float z){
  PlayersManagerImpl::getInstance().movePlayer(id,x,y,z);
}
void updatePlayerAttenuation(int id, float new_at){
  PlayersManagerImpl::getInstance().setAttenuation(id, new_at);
}
void updatePlayerMinDistance(int id, float new_MD){
  PlayersManagerImpl::getInstance().setMinDistance(id, new_MD);
}
void enablePlayerEchoEffect(int id){
  if(PlayersManagerImpl::getInstance().existPlayer(id)){
    PlayersManagerImpl::getInstance().getPlayer(id)->echoEffect = true;
  }
}
void disablePlayerEchoEffect(int id){
  if(PlayersManagerImpl::getInstance().existPlayer(id)){
    PlayersManagerImpl::getInstance().getPlayer(id)->echoEffect = true;
  }
}
void updatePlayerEchoEffect(int id, int value){
  if(value > 0){
    if(PlayersManagerImpl::getInstance().existPlayer(id)){
      PlayersManagerImpl::getInstance().getPlayer(id)->echoEffectValue = value;
    }
  }
}
void removePlayer(int id){ 
  PlayersManagerImpl::getInstance().removePlayer(id);
}
void updateWaitAudioPacketsCount(int pktWaitCount){
  PlayersManagerImpl::getInstance().setWaitAudioPackets(pktWaitCount);
}

void setTalkRoom(int id){
  player::SelfImpl::getInstance().setTalkRoom(id);
}

void talkInRomm(){
  player::SelfImpl::getInstance().talkRoom();
}
void talkInLocal(){
  player::SelfImpl::getInstance().talkLocal();
}

void enableRecAudio(){
  soundmanager::RecorderImpl::getInstance().enableRec();
}

void disableRecAudio(){
  soundmanager::RecorderImpl::getInstance().disableRec();
}

float getMicVolume(){
  //soundmanager::RecorderImpl::getInstance().
  
}

float setMicVolume(){
  
}

void setVolumeAudio(float volume){
  sf::Listener::setGlobalVolume(volume);
}

bool isConnected(){
  return socketUdpImpl::getInstance().isConnected();
}

int getConnectionError(){ //globaldefs.h errors
  return 0;
}

void closeSocket(){ //call and wait, call connectTo();
  socketUdpImpl::getInstance().close();
}

void connectTo(char* ip, int ip_size, unsigned short port){
  const char* ipC = static_cast<const char*>(ip);
  std::string ipStr(ipC, ipC + ip_size);
  connect(ipStr, port);
}

void connect(std::string ip, unsigned short port){
  sf::IpAddress ipaddress(ip);
  socketUdpImpl::refabric(ipaddress, port);
}


void init(int register_id, int id, char* ip, int ip_size, unsigned short port, unsigned char *key, float x, float y, float z, bool needEncrypt){
  if(initialized == false){
    initialized = true;

    player::SelfImpl::frabric(register_id, id, needEncrypt);

    data::buffer keyBuff(key, 16);

    CryptImpl::fabric((unsigned char*)(keyBuff.getData()));

    connectTo(ip, ip_size, port);

    soundmanager::listener::movePos(x,y,z);
    soundmanager::RecorderImpl::getInstance();
    
  }
}

}