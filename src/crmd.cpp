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

#include "WinDump.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
#include <windows.h>  
#include <iostream>
#include <fcntl.h>
#include <io.h>

#include "exchndl.h"

#define OPENCONSOLE
#define isWindowsOS
void initDbg(){
  #ifdef NEED_DGB
  SetUnhandledExceptionFilter(unhandled_handler);
  #endif
}


#endif

std::shared_mutex initSync = std::shared_mutex();

bool CRMD_isTalking(){
  soundmanager::RecorderImpl::getInstance().isTalking();
}

void CRMD_sendMyPos(){
  player::SelfImpl::getInstance().sendPosInfo();
}

void CRMD_updateMyPos(uint32_t map, float x, float y, float z){
  soundmanager::listener::movePos(x,y,z); 
  player::SelfImpl::getInstance().setPos(map, static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
}

void CRMD_updateMyPos_map(uint32_t map){
  player::SelfImpl::getInstance().setMap(map);
}
void CRMD_updateMyPos_coords(float x, float y, float z){
  soundmanager::listener::movePos(x,y,z); 
  player::SelfImpl::getInstance().setX(x);
  player::SelfImpl::getInstance().setX(y);
  player::SelfImpl::getInstance().setX(z);
}

void CRMD_updateMyDir(float x, float y, float z){ 
  soundmanager::listener::moveDir(x,y,z); 
}

void CRMD_setUpVector(float x, float y, float z){
  soundmanager::listener::setUpVector(x,y,z); 
}

void CRMD_setEcho(bool isOn, float decay, float delay){
  soundmanager::RecorderImpl::getInstance().setEcho(isOn, decay, delay);
}

void CRMD_setReverb(bool isOn, float decay, float delay){
  soundmanager::RecorderImpl::getInstance().setReverb(isOn, decay, delay);
}

void CRMD_insertPlayer(uint32_t id, float x, float y, float z){
  PlayersManagerImpl::getInstance().insertPlayer(id,x,y,z);
}

void CRMD_movePlayer(uint32_t id, float x, float y, float z){
  PlayersManagerImpl::getInstance().setPosition(id,x,y,z);
}

bool CRMD_playerIsTalking(uint32_t id){
  return PlayersManagerImpl::getInstance().getPlayer(id).get()->isPlaying();
}

template <typename T>
void extdbgCout(T t) 
{
    std::cout << t;
}
template<typename T, typename... Args>
void dbgCout(T t, Args... args) // recursive variadic function
{
  #ifdef NEED_DGB
    std::cout << t ;

    extdbgCout(args...) ;

    std::cout << std::endl;
  #endif
}

void CRMD_getAllPlayerTalking(uint32_t* playersIds, uint32_t playersIds_size, uint32_t *out_size){
  std::vector<PLAYER> players = PlayersManagerImpl::getInstance().getAllPlayers();
  std::vector<PLAYER> playersTalking;
  for(PLAYER player : players){
    dbgCout("Player ", player.get()->id);
    if(player.get()->isPlaying()){
      dbgCout("is Talking ", player.get()->id);
      playersTalking.push_back(player);
    }
  }

  int size = 0;
  for(int i = 0; i < playersIds_size && i < playersTalking.size(); i++){
    playersIds[i] = playersTalking[i].get()->id;
    size = i+1;
  }
  *out_size = size;
}

void CRMD_updatePlayerAttenuation(uint32_t id, float new_at){
  PlayersManagerImpl::getInstance().setAttenuation(id, new_at);
}
void CRMD_updatePlayerMinDistance(uint32_t id, float new_MD){
  PlayersManagerImpl::getInstance().setMinDistance(id, new_MD);
}

void CRMD_removePlayer(uint32_t id){ 
  PlayersManagerImpl::getInstance().removePlayer(id);
}
void CRMD_setAudioPacketWaitCount(int pktWaitCount){
  
}

void CRMD_setTalkRoom(uint32_t id){
  player::SelfImpl::getInstance().setTalkRoom(id);
}

void CRMD_setTalkInRomm(bool talk){
  if(talk){
    player::SelfImpl::getInstance().enableTalkRoom();
  } else {
    player::SelfImpl::getInstance().disableTalkRoom();
  }
}

void CRMD_setTalkInLocal(bool talk){
    if(talk){
      player::SelfImpl::getInstance().enableTalkLocal();
    } else {
      player::SelfImpl::getInstance().disableTalkLocal();
    }
}

void CRMD_setListenRomm(bool listen){
  if(listen){
    player::SelfImpl::getInstance().enableListenGroup();
  } else {
    player::SelfImpl::getInstance().disableListenGroup();
  }
}
void CRMD_setListenLocal(bool listen){
  if(listen){
    player::SelfImpl::getInstance().enableListenLocal();
  } else {
    player::SelfImpl::getInstance().disableListenLocal();
  }
}

void CRMD_talkInRomm(){
  player::SelfImpl::getInstance().enableTalkRoom();
}
void CRMD_talkInLocal(){
  player::SelfImpl::getInstance().enableTalkLocal();
}

void CRMD_enableRecAudio(){
  soundmanager::RecorderImpl::getInstance().enableRec();
}

void CRMD_disableRecAudio(){
  soundmanager::RecorderImpl::getInstance().disableRec();
}

void CRMD_enableSendAudio(){
  soundmanager::RecorderImpl::getInstance().enableSendData();
}

void CRMD_disableSendAudio(){
  soundmanager::RecorderImpl::getInstance().disableSendData();
}

void CRMD_enableAutoDetect(){
  soundmanager::RecorderImpl::getInstance().setNeedDetect(true);
}
void CRMD_disableAutoDetect(){
  soundmanager::RecorderImpl::getInstance().setNeedDetect(false);
}
void CRMD_setAutoDetect(float value){
    soundmanager::RecorderImpl::getInstance().setDetectValue(value);
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
  return sf::Listener::getGlobalVolume();
}
void CRMD_setVolumeAudio(float volume){
  sf::Listener::setGlobalVolume(volume);
}

void CRMD_getMicDevices(char** outDevicesName, size_t maxDevices, size_t maxNameSize, size_t& devicesCount){
  std::vector<std::string> devices = soundmanager::RecorderImpl::getInstance().getDevices();
  for(int i = 0; i < maxDevices && i < devices.size(); i++){
    if(devices[i].size() <= maxNameSize){
      strcpy(outDevicesName[i], devices[i].c_str());
    }
  }
}

bool CRMD_setMicDevice(char* name, size_t size){
  std::string device(name, size);
  return soundmanager::RecorderImpl::getInstance().swapDevice(device);
}

void CRMD_enableReverb(float density, float diffusion, float gain, float gainHf, float decayTime, float decayHfRatio){
  soundmanager::NetworkAudioStream::enableReverb(density, diffusion, gain, gainHf, decayTime, decayHfRatio);
}
void CRMD_disableReverb(){
  soundmanager::NetworkAudioStream::disableReverb();
}

bool CRMD_isConnected(){
  return socketUdpImpl::getInstance().isConnected();
}

int CRMD_getConnectionError(){ //globaldefs.h errors
  return 0;
}

void CRMD_closeSocket(){
  std::cout << "Closing socket..." << std::endl;
  socketUdpImpl::getInstance().close();
  std::cout << "Socket closed" << std::endl;
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
  if(!initialized.load()){ 
    std::cout << "Not Initialized, call new session" << std::endl;
    return true;
  }

  initSync.lock_shared();
  if(player::SelfImpl::isStarted()){
    if(player::SelfImpl::getInstance().isNeedCallNewSession()){
      std::cout << "Socket is closed, call new session" << std::endl;
      initSync.unlock_shared();
      return true;
    }
  } else {
    std::cout << "Self not exist, call new session" << std::endl;
    initSync.unlock_shared();
    return true;
  }
  initSync.unlock_shared();

  return false;
}

void CRMD_openConsole(){
  #if defined(OPENCONSOLE)

  int hConHandle;
    intptr_t lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE *fp;

    // Aloca uma janela de console para este aplicativo
    AllocConsole();

    // Configura o buffer de tela para ser grande o suficiente para permitir rolagem de texto
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = 500; // Número máximo de linhas que a janela de console deve ter
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // Redireciona STDOUT não bufferizado para a janela de console
    lStdHandle = (intptr_t)GetStdHandle(STD_OUTPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen(hConHandle, "w");
    *stdout = *fp;
    setvbuf(stdout, NULL, _IONBF, 0);

    // Redireciona STDIN não bufferizado para a janela de console
    lStdHandle = (intptr_t)GetStdHandle(STD_INPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen(hConHandle, "r");
    *stdin = *fp;
    setvbuf(stdin, NULL, _IONBF, 0);

    // Redireciona STDERR não bufferizado para a janela de console
    lStdHandle = (intptr_t)GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen(hConHandle, "w");
    *stderr = *fp;
    setvbuf(stderr, NULL, _IONBF, 0);

    // Faz com que cout, wcout, cin, wcin, wcerr, cerr, wclog e clog
    // apontem para a janela de console também
    std::ios::sync_with_stdio();

  /*
    if (AllocConsole())
    {
        freopen("CONOUT$", "w", stdout);
        SetConsoleTitleA("Debug Console");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);  
    }

    HWND consoleWindow = GetConsoleWindow();

    // Verifica se o console foi obtido com sucesso
    if (consoleWindow != NULL)
    {
      // Força a janela do console a ser visível
      ShowWindow(consoleWindow, SW_SHOW);
    }*/
  #endif
}

void CRMD_exit(){
  soundmanager::RecorderImpl::close(); //Stop send Data
  socketUdpImpl::close(); //First data flush and data process (kill threads)
  PlayersManagerImpl::close(); //Second player manager and players, (kill sound threads)
  player::SelfImpl::close(); //last kill the players
}

void CRMD_init(CRMD_SessionDTO session){
  if(player::SelfImpl::isStarted()){
    player::SelfImpl::getInstance().setNeedCallNewSession(false);
  } else {
    #ifdef isWindowsOS
    WCHAR buffer[512];
    DWORD size = GetCurrentDirectoryW(512, buffer);
    std::wstring cwd(buffer, size);
    cwd = cwd + L"\\crmd";
    AddDllDirectory(cwd.c_str());

    #ifdef NEED_DGB
    //initDbg();
    //ExcHndlInit();
    //ExcHndlSetLogFileNameA("crmd.RTP");
    // Load the DLL
    
    HMODULE hModule = LoadLibraryW(L"exchndl.dll");

    if (hModule != NULL) {
        // Get the function pointer
        typedef void (*PFNEXCHNDLINIT)();
        PFNEXCHNDLINIT pfnExcHndlInit = (PFNEXCHNDLINIT)GetProcAddress(hModule, "ExcHndlInit");

        if (pfnExcHndlInit != NULL) {
            pfnExcHndlInit();
        } else {
            // Handle the error if GetProcAddress fails
            // Add your error handling code here
        }

        // Do not free the DLL module
    } else {
        // Handle the error if LoadLibrary fails
    }
    #endif
    #endif
  }

    bool isInit = true;

    initSync.lock();

    if(session.needEffects){
      soundmanager::load_EFX_functions();
    }

    data::buffer ivBuffer(12);
    std::string iv("123456789012");
    ivBuffer.writeover(0, iv.data(), 12);

    std::cout << "Config instance" << std::endl;

    ConfigImpl::getInstance().setCryptoKey(std::vector<char>(session.key, session.key + 32));
    ConfigImpl::getInstance().setCryptoIV(ivBuffer.getVector());
    ConfigImpl::getInstance().setNeedCryptography(session.needEncrypt);

    std::cout << "Self configure" << std::endl;

    player::SelfImpl::fabric(session.secret_id, session.id);
    player::SelfImpl::getInstance().setNeedCallNewSession(false);
    player::SelfImpl::getInstance().setCrpt(ConfigImpl::getInstance().getCryptoKey(), ConfigImpl::getInstance().getCryptoIV());
    Coords coord;
    coord.setCoords(session.map, session.x, session.y, session.z);
    player::SelfImpl::getInstance().setCoords(coord);

    std::cout << "Recorder instance" << std::endl;

    soundmanager::RecorderImpl::fabric(SAMPLE_RATE, sf::milliseconds(40));

    std::cout << "Socket config" << std::endl;
    connectTo(session.hostname, session.hostname_size, session.port);

    std::cout << "Connecting..." << std::endl;
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

    std::cout << "Listener position" << std::endl;
    soundmanager::listener::movePos(session.x,session.y,session.z);

    if(!initialized){
      if(isInit){
        std::cout << "Initalized" << std::endl;
      } else {
        std::cout << "Fail connection..." << std::endl;
      }
    }

    initialized = isInit;

    initSync.unlock();
}