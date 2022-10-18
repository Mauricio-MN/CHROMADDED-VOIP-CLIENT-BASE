#ifndef MAIN_H // include guard
#define MAIN_H

#include "osSolver.h"

enum AudioType
    {
        LOCAL,
        MYGROUP
    };

inline bool initialized = false;
inline bool recordingAudioData = false;
inline AudioType recordingAudioDataType = AudioType::LOCAL;

EXPORT void init(int id, unsigned char *key, float x, float y, float z, float oneCoordinateCorrespondsToNMeters);
EXPORT void updateMyPos(float x, float y, float z);
EXPORT void updateMyRot(float x, float y, float z);

EXPORT void insertPlayer(int id, float x, float y, float z);
EXPORT void movePlayer(int id, float x, float y, float z);
EXPORT void updatePlayerAttenuation(int id, float new_at);
EXPORT void updatePlayerMinDistance(int id, float new_MD);
EXPORT void removePlayer(int id);

EXPORT void enableRecAudio(AudioType audioType);
EXPORT void disableRecAudio();
EXPORT void setVolumeAudio(float volume);


EXPORT void updateWaitAudioPacketsCount(int pktWaitCount);

#endif