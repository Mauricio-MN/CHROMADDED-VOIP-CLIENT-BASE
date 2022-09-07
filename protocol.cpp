#include <string.h>
#include "protocol.h"

namespace protocol
{

#define header_size 1

    void init()
    {
        
        snd_HandChacke_ID = 4;

        snd_Audio_ID_1 = 4;
        snd_Audio_TYPE_2 = 1;
        snd_Audio_AUDIO_3 = 251;

        snd_Disconnect_ID_1 = 4;

        snd_Pos_id_1 = 4;
        snd_Pos_Map_2 = 4;
        snd_Pos_X_3 = 4;
        snd_Pos_Y_4 = 4;
        snd_Pos_Z_5 = 4;

        rcv_HandChacke_ID = 4;

        rcv_Audio_ID_1 = 4;
        rcv_Audio_NUMBER_2 = 1;
        rcv_Audio_AUDIO_3 = 249;

    }

    namespace tovoipserver
    {

        data* constructGeralData(int my_id, int id_size, char headerNumber)
        {
            data buffer;
            buffer.size = header_size + id_size;
            buffer.buffer = new char[buffer.size];

            tools::transformDataToBufferPos<int>((int *)&headerNumber, header_size, buffer.buffer, 0);
            tools::transformDataToBufferPos<int>(&my_id, id_size, buffer.buffer, header_size);

            return (data*)&buffer;
        }

        data constructHandChackeData(int my_id){
            return *constructGeralData(my_id, snd_HandChacke_ID, snd_HandChacke);
        }

        data constructDisconnectData(int my_id){
            return *constructGeralData(my_id, snd_Disconnect_ID_1, snd_Disconnect);
        }

        data constructPosData(int my_id, int map, int x, int y, int z)
        {
            data buffer;
            int pos_id = header_size;
            int pos_map = pos_id + snd_Pos_id_1;
            int pos_x = pos_map + snd_Pos_Map_2;
            int pos_y = pos_x + snd_Pos_X_3;
            int pos_z = pos_y + snd_Pos_Y_4;
            buffer.size = pos_z + snd_Pos_Z_5;
            buffer.buffer = new char[buffer.size];

            tools::transformDataToBufferPos<int>((int *)&snd_Pos, header_size, buffer.buffer, 0);
            tools::transformDataToBufferPos<int>(&my_id, snd_Pos_id_1, buffer.buffer, pos_id);
            tools::transformDataToBufferPos<int>(&map, snd_Pos_Map_2, buffer.buffer, pos_map);
            tools::transformDataToBufferPos<int>(&x, snd_Pos_X_3, buffer.buffer, pos_x);
            tools::transformDataToBufferPos<int>(&y, snd_Pos_Y_4, buffer.buffer, pos_y);
            tools::transformDataToBufferPos<int>(&z, snd_Pos_Z_5, buffer.buffer, pos_z);

            return buffer;
        }

        data constructAudioData(int my_id, char type, char *buffer_audio, size_t size)
        {
            data buffer;

            int pos_id = header_size;
            int pos_type = pos_id + snd_Audio_ID_1;
            int pos_audio = pos_type + snd_Audio_TYPE_2;

            buffer.size = pos_audio + size;
            buffer.buffer = new char[buffer.size];

            tools::transformDataToBufferPos<int>((int *)&snd_Audio, header_size, buffer.buffer, 0);
            tools::transformDataToBufferPos<int>(&my_id, snd_Audio_ID_1, buffer.buffer, pos_id);
            tools::transformDataToBufferPos<char>(&type, snd_Audio_TYPE_2, buffer.buffer, pos_type);
            tools::transformDataToBufferPos<char>(buffer_audio, size, buffer.buffer, pos_audio);

            return buffer;
        }

    }

    namespace fromvoipserver
    {

        rcv_Audio_stc constructRCVaudioData(char *buffer, size_t size)
        {

            rcv_Audio_stc audioData;

            int pos_Id = header_size;
            int pos_Number = pos_Id + rcv_Audio_ID_1;
            int pos_Audio = pos_Number + rcv_Audio_NUMBER_2;

            audioData.audioData.size = size - pos_Audio;
            audioData.audioData.buffer = new char[audioData.audioData.size];

            tools::bufferCutToData<int>(&audioData.id, rcv_Audio_ID_1, buffer, pos_Id);
            tools::bufferCutToData<int>(&audioData.audio_number, rcv_Audio_NUMBER_2, buffer, pos_Number);
            tools::bufferCutToData<char>(audioData.audioData.buffer, audioData.audioData.size, buffer, pos_Audio);

            return audioData;
        }

        rcv_HandChacke_stc constructRCVhandChackeData(char *buffer, size_t size)
        {
            rcv_HandChacke_stc handchackeData;

            int pos_Id = header_size;

            tools::bufferCutToData<int>(&handchackeData.id, rcv_HandChacke_ID, buffer, pos_Id);

            return handchackeData;
        }

    }

    namespace tools
    {

        void cutBuffer(char *source, int fromSource, char *dest, int destLen)
        {
            strncpy(dest, source + fromSource, destLen);
        }

        template <typename T>
        void transformArrayToBuffer(T *data, size_t size, char *buffer)
        {
            memcpy(buffer, data, sizeof(T) * size);
        }

        template <typename T>
        void transformDataToBuffer(T *data, size_t data_size, char *buffer)
        {
            memcpy(buffer, data, sizeof(data));
        }

        template <typename T>
        void transformDataToBufferPos(T *data, size_t data_size, char *buffer, size_t pos)
        {
            memcpy(buffer + pos, data, sizeof(T));
        }

        template <typename T>
        void bufferToData(T *data, size_t size_buffer, char *buffer)
        {
            memcpy(data, buffer, sizeof(char) * size_buffer);
        }

        template <typename T>
        void bufferCutToData(T *data, size_t size, char *buffer, size_t from)
        {
            memcpy(data, buffer + from, size);
        }

        template <typename T>
        void mergeDatasArray(T *dataA, size_t size_a, T *dataB, size_t size_b, T *dataOut)
        {
            memcpy(dataOut, dataA, sizeof(T) * size_a);
            memcpy(dataOut + (sizeof(T) * size_a), dataB, sizeof(T) * size_b);
        }

        template <typename T>
        bool cutbufferToType(T *data, char *buffer, size_t pos, size_t end)
        {
            if (pos - 1 < 1)
                return false;
            if (end < 1)
                return false;
            memcpy(data, buffer + pos - 1, end - pos + 1);
        }

    }
}