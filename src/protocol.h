#ifndef PROTOCOL_H // include guard
#define PROTOCOL_H

#include <string.h>
#include <memory>
#include <mutex>
#include <thread>
#include "data.h"

#define ptc_header_size 1

namespace protocolinfo{

namespace send{

    namespace header {

    enum Headers{

        HandChacke = 0,
        Audio = 1,
        Disconnect = 2,
        Position = 3

    };

    }

}

namespace receive{

    namespace header{

        enum Headers{

            HandChacke = 0,
            Audio = 1,
            Disconnect = 2

        };

    }

}

}

class ProtocolInfo
{
    public:
    static int snd_HandChacke_REGISTER_ID;
    static int snd_HandChacke_ID;

    static int snd_Audio_ID_1;
    static int snd_Audio_TYPE_2;
    static int snd_Audio_AUDIO_3;

    static int snd_Disconnect_ID_1;

    static int snd_Pos_id_1;
    static int snd_Pos_Map_2;
    static int snd_Pos_X_3;
    static int snd_Pos_Y_4;
    static int snd_Pos_Z_5;

    static int rcv_HandChacke_ID;

    static int rcv_Audio_ID_1;
    static int rcv_Audio_NUMBER_2;
    static int rcv_Audio_AUDIO_3;

    static int rcv_Disconnect_ID;

};

namespace protocol {

    typedef struct rcv_Audio_stc
    {
        int id = -1;
        int audio_number = -1;
        data::buffer audioData;

        rcv_Audio_stc(){
        }

        rcv_Audio_stc(size_t audioData_size){
            audioData = data::buffer(audioData_size);
        }

        rcv_Audio_stc(rcv_Audio_stc* RCV){
            audioData = data::buffer(RCV->audioData);
            id = RCV->id;
            audio_number = RCV->audio_number;
        }

    } zrcv_Audio_stc;

    typedef struct rcv_HandChacke_stc
    {
        int id;
    } zid;

    namespace tovoipserver
    {

        data::buffer constructHandChackeData(int register_id, int my_id);

        data::buffer constructPosData(int my_id, int map, int x, int y, int z);

        data::buffer constructAudioData(int my_id, char type, data::buffer &buffer_audio);

        data::buffer constructDisconnectData(int my_id);

    }

    namespace fromvoipserver
    {

        rcv_Audio_stc constructRCVaudioData(data::buffer &buffer);

        rcv_HandChacke_stc constructRCVhandChackeData(data::buffer &buffer);

        int constructRCVdisconnectData(data::buffer &buffer);
    }
}

#include "protocolTools.h"

#endif