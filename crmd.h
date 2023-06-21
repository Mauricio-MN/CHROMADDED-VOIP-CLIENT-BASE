#ifndef MAIN_H // include guard
#define MAIN_H

#include "osSolver.h"
#include "globaldefs.h"

namespace crmd{

inline bool initialized = false;
inline bool recordingAudioData = false;

/*
register_id -> O id de registro é um id secreto combinado entre Game Server e VOIP server a cada nova autenticação.
id          -> Id público fixo.
ip          -> Ip do servidor VOIP.
port        -> Porta do servidor VOIP.
key         -> Chave de criptografia combinada entre (GameServer e API) e (API e Servidor VOIP) para cada jogador.
x, y, z,    -> Coordenadas em metros do jogador.
needEncrypt -> True = precisa encriptar, False = não precisa
*/
EXPORT void init(int register_id, int id, char* ip, int port, unsigned char *key, float x, float y, float z, bool needEncrypt);
EXPORT void updateMyPos(int map, float x, float y, float z);
EXPORT void updateMyRot(float x, float y, float z);

EXPORT void insertPlayer(int id, float x, float y, float z);
EXPORT void movePlayer(int id, float x, float y, float z);
EXPORT void updatePlayerAttenuation(int id, float new_at);
EXPORT void updatePlayerMinDistance(int id, float new_MD);
EXPORT void enablePlayerEchoEffect(int id);
EXPORT void disablePlayerEchoEffect(int id);
EXPORT void updatePlayerEchoEffect(int id, int value);
EXPORT void removePlayer(int id);

EXPORT void setAudioType(AudioType audioType);
EXPORT void enableRecAudio();
EXPORT void disableRecAudio();
EXPORT float getMicVolume();
EXPORT float setMicVolume();

EXPORT void setVolumeAudio(float volume);


EXPORT void updateWaitAudioPacketsCount(int pktWaitCount);

EXPORT bool isConnected();

EXPORT int getConnectionError(); //globaldefs.h errors

EXPORT void closeSocket(); //call and wait(100/1000 ms), call connectTo();
EXPORT void connectTo(char* ip, int port);

}

#endif