#ifndef PROTOCOL_H // include guard
#define PROTOCOL_H

#include <string.h>

namespace protocol
{

    void init();

    const int ptc_HandChacke = 0;
    inline int ptc_HandChacke_ID;

    const int ptc_Audio = 1;
    inline int ptc_Audio_ID_1;
    inline int ptc_Audio_TYPE_2;
    inline int ptc_Audio_AUDIO_3;

    const int ptc_Disconnect = 3;
    inline int ptc_Disconnect_ID_1;

    const int ptc_Pos = 3;
    inline int ptc_Pos_id_1;
    inline int ptc_Pos_Map_2;
    inline int ptc_Pos_X_3;
    inline int ptc_Pos_Y_4;
    inline int ptc_Pos_Z_5;

    const int rcv_HandChacke = 0;
    inline int rcv_HandChacke_ID;

    const int rcv_Audio = 1;
    inline int rcv_Audio_ID_1;
    inline int rcv_Audio_NUMBER_2;
    inline int rcv_Audio_AUDIO_3;

    const int rcv_Disconnect = 2;
    inline int rcv_Disconnect_ID = 4;

#define header_size 1

    typedef struct data
    {
        size_t size;
        char *buffer;
    } zdata;

    typedef struct rcv_Audio_stc
    {
        int id;
        int audio_number;
        data audioData;

    } zrcv_Audio_stc;

    typedef struct rcv_HandChacke_stc
    {
        int id;
    } zid;

    namespace tovoipserver
    {

        data constructHandChackeData(int my_id);

        data constructPosData(int my_id, int map, int x, int y, int z);

        data constructAudioData(int my_id, char type, char *buffer_audio, size_t size);

    }

    namespace fromvoipserver
    {

        rcv_Audio_stc constructRCVaudioData(char *buffer, size_t size);

        rcv_HandChacke_stc constructRCVhandChackeData(char *buffer, size_t size);

    }

    namespace tools
    {

        void cutBuffer(char *source, int fromSource, char *dest, int destLen);

        template <typename T>
        void transformArrayToBuffer(T *data, size_t size, char *buffer);

        template <typename T>
        void transformDataToBuffer(T *data, size_t data_size, char *buffer);

        template <typename T>
        void transformDataToBufferPos(T *data, size_t data_size, char *buffer, size_t pos);

        template <typename T>
        void bufferToData(T *data, size_t size_buffer, char *buffer);

        template <typename T>
        void bufferCutToData(T *data, size_t size, char *buffer, size_t from);

        template <typename T>
        void mergeDatasArray(T *dataA, size_t size_a, T *dataB, size_t size_b, T *dataOut);

        template <typename T>
        bool cutbufferToType(T *data, char *buffer, size_t pos, size_t end);

    }
}

#endif