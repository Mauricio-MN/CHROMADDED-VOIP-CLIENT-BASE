#ifndef MAIN_H // include guard
#define MAIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
    //  Microsoft 
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    //  do nothing and hope for the best?
    #define EXPORT
    #define IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
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
};

//First Call
EXPORT void CRMD_init(CRMD_SessionDTO session);
EXPORT bool CRMD_needCallNewSession();

EXPORT void CRMD_updateMyPos(uint32_t map, float x, float y, float z);
EXPORT void CRMD_updateMyPos_map(uint32_t map);
EXPORT void CRMD_updateMyPos_coords(float x, float y, float z);
EXPORT void CRMD_updateMyRot(float x, float y, float z);

EXPORT void CRMD_insertPlayer(uint32_t id, float x, float y, float z);
EXPORT void CRMD_movePlayer(uint32_t id, float x, float y, float z);
EXPORT void CRMD_updatePlayerAttenuation(uint32_t id, float new_at);
EXPORT void CRMD_updatePlayerMinDistance(uint32_t id, float new_MD);
EXPORT void CRMD_enablePlayerEchoEffect(uint32_t id);
EXPORT void CRMD_disablePlayerEchoEffect(uint32_t id);
EXPORT void CRMD_updatePlayerEchoEffect(uint32_t id, int value);
EXPORT void CRMD_removePlayer(uint32_t id);

EXPORT void CRMD_setTalkRoom(uint32_t id);
EXPORT void CRMD_talkInRomm();
EXPORT void CRMD_talkInLocal();

EXPORT void CRMD_enableRecAudio();
EXPORT void CRMD_disableRecAudio();
EXPORT void CRMD_setListenRecAudio(bool needListen);

EXPORT float CRMD_getMicVolume();
EXPORT void CRMD_setMicVolume(float volume);

EXPORT float CRMD_getVolumeAudio();
EXPORT void CRMD_setVolumeAudio(float volume);

EXPORT bool CRMD_isConnected();

//Para fins de log e reconexão(o protocolo UDP não depende de conexão, mas a seção possui validade)
EXPORT int CRMD_getConnectionError(); //RESERVED

EXPORT void CRMD_closeSocket();

#ifdef __cplusplus
}
#endif

#endif