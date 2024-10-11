#ifndef CRMD_H // include guard
#define CRMD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(API_EXPORT)

#if defined(_MSC_VER)
    //  Microsoft 
    #define API __declspec(dllexport)
#elif defined(__GNUC__)
    //  GCC
    #define API __attribute__((visibility("default")))
#else
    #define API
    #pragma warning Unknown dynamic link import/export semantics.
#endif

#else

#if defined(_MSC_VER)
    //  Microsoft 
    #define API __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define API
#else
    #define API
    #pragma warning Unknown dynamic link import/export semantics.
#endif

#endif

#define callInvalidSession

struct CRMD_SessionDTO{
    uint32_t id;
    uint32_t secret_id;

    char* hostname;
    size_t hostname_size;
    unsigned short port;

    unsigned char *key;

    uint32_t map;
    float x;
    float y;
    float z;

    bool needEncrypt;

    bool needEffects;
};

//Supports multithreading, stop everything from lib, use this just to close the app
API void CRMD_exit();
//Need execute first
API void CRMD_init(CRMD_SessionDTO session);
//Only Windows OS
API void CRMD_openConsole();
//Supports multithreading
API bool CRMD_needCallNewSession();

//Supports multithreading
API bool CRMD_isTalking();

//Supports multithreading, don't exaggerate too much
API void CRMD_sendMyPos();
//Supports multithreading
API void CRMD_updateMyPos(uint32_t map, float x, float y, float z);
//Supports multithreading
API void CRMD_updateMyPos_map(uint32_t map);
//Supports multithreading
API void CRMD_updateMyPos_coords(float x, float y, float z);
//Only single thread
API void CRMD_updateMyDir(float x, float y, float z);
//Only single thread
API void CRMD_setUpVector(float x, float y, float z);
/**********************************************************************/
  /*!
    @brief  Max 4 seconds, mutex operation
    @param  isOn    On/Off boolean switch
    @param  decay    0.0f to 1.0f
    @param  delay    In samples
  */
/**********************************************************************/
void CRMD_setEcho(bool isOn, float decay, float delay);

/**********************************************************************/
  /*!
    @brief  Max 6 seconds, mutex operation
    @param  isOn    On/Off boolean switch
    @param  decay    0.0f to 1.0f
    @param  delay    In samples
  */
/**********************************************************************/
void CRMD_setReverb(bool isOn, float decay, float delay);

//Supports multithreading
API void CRMD_insertPlayer(uint32_t id, float x, float y, float z);
//Supports multithreading
API void CRMD_movePlayer(uint32_t id, float x, float y, float z);
//Supports multithreading
API bool CRMD_playerIsTalking(uint32_t id);
//Supports multithreading, don't exaggerate too much
API void CRMD_getAllPlayerTalking(uint32_t* playersIds, uint32_t playersIds_size, uint32_t *out_size);
//Supports multithreading
API void CRMD_updatePlayerAttenuation(uint32_t id, float new_at);
//Supports multithreading
API void CRMD_updatePlayerMinDistance(uint32_t id, float new_MD);

//Supports multithreading
API void CRMD_removePlayer(uint32_t id);

//OFF, do in game server side
API void CRMD_setTalkRoom(uint32_t id);
//OFF, do in game server side
API void CRMD_setTalkInRomm(bool talk);
//OFF, do in game server side
API void CRMD_setTalkInLocal(bool talk);
//OFF, do in game server side
API void CRMD_setListenRomm(bool listen);
//OFF, do in game server side
API void CRMD_setListenLocal(bool listen);

/**********************************************************************/
  /*!
    @brief  Only single thread. This starts the audio management process, only turn off if not necessary in the current context, such as login screens, reconnect or if you need to change the audio device.
  */
/**********************************************************************/
API void CRMD_enableRecAudio();
/**********************************************************************/
  /*!
    @brief  Only single thread. This stops the audio management process, only turn off if not necessary in the current context, such as login screens, reconnect or if you need to change the audio device.
  */
/**********************************************************************/
API void CRMD_disableRecAudio();

//Only single thread recommended, it start to send audio to server
API void CRMD_enableSendAudio();
//Only single thread recommended, it stop to send audio to server
API void CRMD_disableSendAudio();

//Supports multithreading
API void CRMD_enableAutoDetect();
//Supports multithreading
API void CRMD_disableAutoDetect();
//Supports multithreading, sound sensitivity
API void CRMD_setAutoDetect(float value);

//Supports multithreading
API void CRMD_setListenRecAudio(bool needListen);

//Supports multithreading
API float CRMD_getMicVolume();
//Supports multithreading
API void CRMD_setMicVolume(float volume);

//Supports multithreading
API float CRMD_getVolumeAudio();
//Supports multithreading
API void CRMD_setVolumeAudio(float volume);

//Receive 2d array ([devices][names])
API void CRMD_getMicDevices(char** outDevicesName, size_t maxDevices, size_t maxNameSize, size_t& devicesCount);
API bool CRMD_setMicDevice(char* name, size_t size);

//Supports multithreading
API bool CRMD_isConnected();

//Para fins de log e reconexão(o protocolo UDP não depende de conexão, mas a seção possui validade)
API int CRMD_getConnectionError(); //RESERVED

//Supports multithreading
API void CRMD_closeSocket();

#ifdef __cplusplus
}
#endif

#endif