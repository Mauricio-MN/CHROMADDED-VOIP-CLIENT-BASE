#include <string.h>
#include "protocol.h"
#include <memory>
#include <iostream>

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

        data constructGeralData(int my_id, int id_size, char headerNumber)
        {
            data buffer(header_size + id_size);

            tools::transformDataToBufferPos(&headerNumber, buffer.getBuffer(), 0);
            tools::transformDataToBufferPos(&my_id, buffer.getBuffer(), header_size);

            return buffer;
        }

        data constructHandChackeData(int my_id){
            return constructGeralData(my_id, snd_HandChacke_ID, snd_HandChacke);
        }

        data constructDisconnectData(int my_id){
            return constructGeralData(my_id, snd_Disconnect_ID_1, snd_Disconnect);
        }

        data constructPosData(int my_id, int map, int x, int y, int z)
        {
            int pos_id = header_size;
            int pos_map = pos_id + snd_Pos_id_1;
            int pos_x = pos_map + snd_Pos_Map_2;
            int pos_y = pos_x + snd_Pos_X_3;
            int pos_z = pos_y + snd_Pos_Y_4;
            data buffer(pos_z + snd_Pos_Z_5);

            tools::transformDataToBufferPos(&snd_Pos, buffer.getBuffer(), 0);
            tools::transformDataToBufferPos(&my_id, buffer.getBuffer(), pos_id);
            tools::transformDataToBufferPos(&map, buffer.getBuffer(), pos_map);
            tools::transformDataToBufferPos(&x, buffer.getBuffer(), pos_x);
            tools::transformDataToBufferPos(&y, buffer.getBuffer(), pos_y);
            tools::transformDataToBufferPos(&z, buffer.getBuffer(), pos_z);

            return buffer;
        }

        data constructAudioData(int my_id, char type, data *buffer_audio)
        {

            int pos_id = header_size;
            int pos_type = pos_id + snd_Audio_ID_1;
            int pos_audio = pos_type + snd_Audio_TYPE_2;
            data buffer(pos_audio + buffer_audio->size);

            tools::transformDataToBufferPos(&snd_Audio, buffer.getBuffer(), 0);
            tools::transformDataToBufferPos(&my_id, buffer.getBuffer(), pos_id);
            tools::transformDataToBufferPos(&type, buffer.getBuffer(), pos_type);
            tools::transformArrayToBuffer(buffer_audio->getBuffer(), buffer_audio->size, buffer.getBuffer() + pos_audio);

            return buffer;
        }

    }

    namespace fromvoipserver
    {

        rcv_Audio_stc constructRCVaudioData(data buffer)
        {

            int pos_Id = header_size;
            int pos_Number = pos_Id + rcv_Audio_ID_1;
            int pos_Audio = pos_Number + rcv_Audio_NUMBER_2;
            
            rcv_Audio_stc audioDataPK(buffer.size - pos_Audio);
            char audio_number = 0;
            
            tools::bufferCutToData(&audioDataPK.id, buffer.getBuffer(), pos_Id, rcv_Audio_ID_1);
            tools::bufferCutToData(&audio_number, buffer.getBuffer(), pos_Number, rcv_Audio_NUMBER_2);
            audioDataPK.audio_number = audio_number;
            tools::bufferCutToData<char>(audioDataPK.audioData->getBuffer(), buffer.getBuffer(), pos_Audio, audioDataPK.audioData->size);
            
            return audioDataPK;
        }

        rcv_HandChacke_stc constructRCVhandChackeData(data buffer)
        {
            rcv_HandChacke_stc handchackeData;

            int pos_Id = header_size;

            tools::bufferCutToData<int>(&handchackeData.id, buffer.getBuffer(), pos_Id, rcv_HandChacke_ID);

            return handchackeData;
        }

    }
}