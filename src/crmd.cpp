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

#include "soundmanagerRecorder.h"

//#include "../libs/AL/al.h"
//#include "../libs/AL/alc.h"

#include "player.h"

#include "socketUdp.h"

#include "soundmanager.h"

#include "crmd.h"
#include "crmdprivate.h"

#include "config.h"

#include "crpt.h"

void CRMD_updateMyPos(uint32_t map, float x, float y, float z){
  soundmanager::listener::movePos(x,y,z); 
  player::SelfImpl::getInstance().setPos(map, static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
}

void CRMD_updateMyPos_map(uint32_t map){

}
void CRMD_updateMyPos_coords(float x, float y, float z){
  
}

void CRMD_updateMyRot(float x, float y, float z){ 
  soundmanager::listener::moveRot(x,y,z); 
}

void CRMD_insertPlayer(uint32_t id, float x, float y, float z){
  PlayersManagerImpl::getInstance().insertPlayer(id,x,y,z);
  }

void CRMD_movePlayer(uint32_t id, float x, float y, float z){
  PlayersManagerImpl::getInstance().setPosition(id,x,y,z);
}
void CRMD_updatePlayerAttenuation(uint32_t id, float new_at){
  PlayersManagerImpl::getInstance().setAttenuation(id, new_at);
}
void CRMD_updatePlayerMinDistance(uint32_t id, float new_MD){
  PlayersManagerImpl::getInstance().setMinDistance(id, new_MD);
}
void CRMD_enablePlayerEchoEffect(uint32_t id){
  if(PlayersManagerImpl::getInstance().existPlayer(id)){
    PlayersManagerImpl::getInstance().getPlayer(id)->echoEffect = true;
  }
}
void CRMD_disablePlayerEchoEffect(uint32_t id){
  if(PlayersManagerImpl::getInstance().existPlayer(id)){
    PlayersManagerImpl::getInstance().getPlayer(id)->echoEffect = true;
  }
}
void CRMD_updatePlayerEchoEffect(uint32_t id, int value){
  if(value > 0){
    if(PlayersManagerImpl::getInstance().existPlayer(id)){
      PlayersManagerImpl::getInstance().getPlayer(id)->echoEffectValue = value;
    }
  }
}
void CRMD_removePlayer(uint32_t id){ 
  PlayersManagerImpl::getInstance().removePlayer(id);
}
void CRMD_setAudioPacketWaitCount(int pktWaitCount){
  
}

void CRMD_setTalkRoom(uint32_t id){
  player::SelfImpl::getInstance().setTalkRoom(id);
}

void CRMD_talkInRomm(){
  player::SelfImpl::getInstance().talkRoom();
}
void CRMD_talkInLocal(){
  player::SelfImpl::getInstance().talkLocal();
}

void CRMD_enableRecAudio(){
  soundmanager::RecorderImpl::getInstance().enableRec();
}

void CRMD_disableRecAudio(){
  soundmanager::RecorderImpl::getInstance().disableRec();
}

float CRMD_getMicVolume(){
  return soundmanager::RecorderImpl::getInstance().getVolume();
}
void CRMD_setMicVolume(float volume){
  soundmanager::RecorderImpl::getInstance().setVolume(volume);
}

float CRMD_getVolumeAudio(){
  sf::Listener::getGlobalVolume();
}
void CRMD_setVolumeAudio(float volume){
  sf::Listener::setGlobalVolume(volume);
}

bool CRMD_isConnected(){
  return socketUdpImpl::getInstance().isConnected();
}

int CRMD_getConnectionError(){ //globaldefs.h errors
  return 0;
}

void CRMD_closeSocket(){
  socketUdpImpl::getInstance().close();
}

void connect(std::string hostname, unsigned short port){
  sf::IpAddress ipaddress(hostname);
  socketUdpImpl::refabric(ipaddress, port);
}

void CRMD_connectTo(char* hostname, int hostname_size, unsigned short port){
  const char* ipC = static_cast<const char*>(hostname);
  std::string ipStr(ipC, ipC + hostname_size);
  connect(ipStr, port);
}

void disconnect(){
  socketUdpImpl::getInstance().close();
}

bool initialized = false;

void CRMD_init(uint32_t register_id, uint32_t id, char* hostname, size_t hostname_size, unsigned short port, unsigned char *key, uint32_t map, float x, float y, float z, bool needEncrypt){
  if(initialized == false){
    initialized = true;

    data::buffer ivBuffer(IV_SIZE);
    std::string iv("123456789012");
    ivBuffer.writeover(0, iv.data(), IV_SIZE);

    ConfigImpl::getInstance().setCryptoKey(std::vector<char>(key, key + 32));
    ConfigImpl::getInstance().setCryptoIV(ivBuffer.getVector());
    ConfigImpl::getInstance().setNeedCryptography(needEncrypt);

    player::SelfImpl::frabric(register_id, id);

    CRMD_connectTo(hostname, hostname_size, port);

    player::SelfImpl::getInstance().sendConnect();

    soundmanager::listener::movePos(x,y,z);
    soundmanager::RecorderImpl::fabric(SAMPLE_RATE, sf::milliseconds(20));
    
  }
}