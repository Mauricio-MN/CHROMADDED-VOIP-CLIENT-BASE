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

std::shared_mutex initSync = std::shared_mutex();

void CRMD_updateMyPos(uint32_t map, float x, float y, float z){
  soundmanager::listener::movePos(x,y,z); 
  player::SelfImpl::getInstance().setPos(map, static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
}

void CRMD_updateMyPos_map(uint32_t map){
  player::SelfImpl::getInstance().setMap(map);
}
void CRMD_updateMyPos_coords(float x, float y, float z){
  player::SelfImpl::getInstance().setX(x);
  player::SelfImpl::getInstance().setX(y);
  player::SelfImpl::getInstance().setX(z);
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

void CRMD_setListenRecAudio(bool needListen){
  soundmanager::RecorderImpl::getInstance().setListenAudio(needListen);
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

void connectTo(char* hostname, int hostname_size, unsigned short port){
  const char* ipC = static_cast<const char*>(hostname);
  std::string ipStr(ipC, ipC + hostname_size);
  connect(ipStr, port);
}

void disconnect(){
  socketUdpImpl::getInstance().close();
}

std::atomic_bool initialized = std::atomic_bool();

bool CRMD_needCallNewSession(){
  if(!initialized.load()) return true;

  initSync.lock_shared();
  if(player::SelfImpl::isStarted()){
    return player::SelfImpl::getInstance().isNeedCallNewSession();
  } else {
    return true;
  }
  initSync.lock_shared();
}

void CRMD_init(CRMD_SessionDTO session){
  if(player::SelfImpl::isStarted()){
    player::SelfImpl::getInstance().setNeedCallNewSession(false);
  }

    bool isInit = true;

    initSync.lock();

    data::buffer ivBuffer(12);
    std::string iv("123456789012");
    ivBuffer.writeover(0, iv.data(), 12);

    ConfigImpl::getInstance().setCryptoKey(std::vector<char>(session.key, session.key + 32));
    ConfigImpl::getInstance().setCryptoIV(ivBuffer.getVector());
    ConfigImpl::getInstance().setNeedCryptography(session.needEncrypt);

    player::SelfImpl::fabric(session.secret_id, session.id);
    player::SelfImpl::getInstance().setNeedCallNewSession(false);
    player::SelfImpl::getInstance().setCrpt(ConfigImpl::getInstance().getCryptoKey(), ConfigImpl::getInstance().getCryptoIV());
    Coords coord;
    coord.setCoords(session.map, session.x, session.y, session.z);
    player::SelfImpl::getInstance().setCoords(coord);

    connectTo(session.hostname, session.hostname_size, session.port);

    int tryConnectCount = 0;
    while(!player::SelfImpl::getInstance().isConnected()){
      player::SelfImpl::getInstance().sendConnect();
      tryConnectCount++;
      if(tryConnectCount > 20){
        isInit = false;
        player::SelfImpl::getInstance().setNeedCallNewSession(true);
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    soundmanager::listener::movePos(session.x,session.y,session.z);
    soundmanager::RecorderImpl::fabric(SAMPLE_RATE, sf::milliseconds(20));

    initialized = isInit;

    initSync.unlock();
}