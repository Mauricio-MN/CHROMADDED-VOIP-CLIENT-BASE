#ifndef MAIN_H // include guard
#define MAIN_H

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

namespace crmd{

//Do not write
inline bool initialized = false;
//Do not write
inline bool recordingAudioData = false;

/*
register_id -> O id de registro é um id secreto combinado entre Game Server e VOIP server a cada nova autenticação.
id          -> Id público fixo.
ip          -> Ip do servidor VOIP (string padrão c++ g++)
port        -> Porta do servidor VOIP.
key         -> Chave de criptografia combinada entre (GameServer e API) e (API e Servidor VOIP) para cada jogador (16 bytes).
x, y, z,    -> Coordenadas em metros do jogador (z = 0 para fixado ao chão (isométrico) ).
needEncrypt -> True = precisa encriptar (mais seguro), False = não precisa (todos os pacotes de audio e informações secretas (register_id=secretId) são livres para leitura em rede)
*/
EXPORT void init(int register_id, int id, char* ip, int ip_size, unsigned short port, unsigned char *key, float x, float y, float z, bool needEncrypt);
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

EXPORT void setTalkRoom(int id);
EXPORT void talkInRomm();
EXPORT void talkInLocal();

EXPORT void enableRecAudio();
EXPORT void disableRecAudio();
EXPORT float getMicVolume();
EXPORT float setMicVolume();

EXPORT void setVolumeAudio(float volume);

EXPORT void updateWaitAudioPacketsCount(int pktWaitCount);

EXPORT bool isConnected();

//Para fins de log e reconexão(o protocolo UDP não depende de conexão, mas a seção possui validade)
EXPORT int getConnectionError(); //globaldefs.h errors

EXPORT void closeSocket(); //call and wait(100/1000 ms), call connectTo();
EXPORT void connectTo(char* ip, int ip_size, unsigned short port);

}

#ifdef __cplusplus
}
#endif

#endif