#ifndef PROTOCOL_H // include guard
#define PROTOCOL_H

#include <string.h>
#include <memory>
#include <mutex>
#include <thread>
#include "data.h"

namespace protocol
{

    void init();

    const char snd_HandChacke = 0;
    inline int snd_HandChacke_REGISTER_ID;
    inline int snd_HandChacke_ID;

    const char snd_Audio = 1;
    inline int snd_Audio_ID_1;
    inline int snd_Audio_TYPE_2;
    inline int snd_Audio_AUDIO_3;

    const char snd_Disconnect = 2;
    inline int snd_Disconnect_ID_1;

    const char snd_Pos = 3;
    inline int snd_Pos_id_1;
    inline int snd_Pos_Map_2;
    inline int snd_Pos_X_3;
    inline int snd_Pos_Y_4;
    inline int snd_Pos_Z_5;

    const char rcv_HandChacke = 0;
    inline int rcv_HandChacke_ID;

    const char rcv_Audio = 1;
    inline int rcv_Audio_ID_1;
    inline int rcv_Audio_NUMBER_2;
    inline int rcv_Audio_AUDIO_3;

    const char rcv_Disconnect = 2;
    inline int rcv_Disconnect_ID;

#define header_size 1

    typedef struct rcv_Audio_stc
    {
        int id = -1;
        int audio_number = -1;
        data::buffer *audioData;
        size_t validAddressOwner;
        int owner;

        rcv_Audio_stc(size_t audioData_size){
            audioData = new data::buffer(audioData_size);
            owner = 1;
            validAddressOwner = reinterpret_cast<size_t>(&owner);
        }

        rcv_Audio_stc(rcv_Audio_stc* RCV){
            audioData = new data::buffer(RCV->audioData);
            owner = 1;
            validAddressOwner = reinterpret_cast<size_t>(&owner);
        }

        ~rcv_Audio_stc(){
            if(validAddressOwner == reinterpret_cast<size_t>(&owner)){
                delete audioData;
            }
        }

    } zrcv_Audio_stc;

    typedef struct rcv_HandChacke_stc
    {
        int id;
    } zid;

    namespace tovoipserver
    {

        data::buffer constructHandChackeData(int my_id);

        data::buffer constructPosData(int my_id, int map, int x, int y, int z);

        data::buffer constructAudioData(int my_id, char type, data::buffer *buffer_audio);

        data::buffer constructDisconnectData(int my_id);

    }

    namespace fromvoipserver
    {

        rcv_Audio_stc constructRCVaudioData(data::buffer buffer);

        rcv_HandChacke_stc constructRCVhandChackeData(data::buffer buffer);

        int constructRCVdisconnectData(data::buffer buffer);
    }
}

#include "protocolTools.h"

#endif