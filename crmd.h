#ifndef MAIN_H // include guard
#define MAIN_H

#include "osSolver.h"
#include "globaldefs.h"

namespace crmd{

inline bool initialized = false;
inline bool recordingAudioData = false;

EXPORT void init(int id, char* ip, int ip_size, unsigned char *key, float x, float y, float z, float oneCoordinateCorrespondsToNMeters, bool needEncrypt);
EXPORT void updateMyPos(float x, float y, float z);
EXPORT void updateMyRot(float x, float y, float z);

EXPORT void insertPlayer(int id, float x, float y, float z);
EXPORT void movePlayer(int id, float x, float y, float z);
EXPORT void updatePlayerAttenuation(int id, float new_at);
EXPORT void updatePlayerMinDistance(int id, float new_MD);
EXPORT void enablePlayerEchoEffect(int id);
EXPORT void disablePlayerEchoEffect(int id);
EXPORT void updatePlayerEchoEffect(int id, int value);
EXPORT void removePlayer(int id);

EXPORT void enableRecAudio(AudioType audioType);
EXPORT void disableRecAudio();
EXPORT void setVolumeAudio(float volume);


EXPORT void updateWaitAudioPacketsCount(int pktWaitCount);

}

#endif