#include <string.h>
#include "protocol.h"
#include <memory>
#include <iostream>


    int ProtocolInfo::snd_HandChacke_REGISTER_ID = 4;
    int ProtocolInfo::snd_HandChacke_ID = 4;

    int ProtocolInfo::snd_Audio_ID_1 = 4;
    int ProtocolInfo::snd_Audio_TYPE_2 = 1;
    int ProtocolInfo::snd_Audio_AUDIO_3 = 251;

    int ProtocolInfo::snd_Disconnect_ID_1 = 4;

    int ProtocolInfo::snd_Pos_id_1 = 4;
    int ProtocolInfo::snd_Pos_Map_2 = 4;
    int ProtocolInfo::snd_Pos_X_3 = 4;
    int ProtocolInfo::snd_Pos_Y_4 = 4;
    int ProtocolInfo::snd_Pos_Z_5 = 4;

    int ProtocolInfo::rcv_HandChacke_ID = 4;

    int ProtocolInfo::rcv_Audio_ID_1 = 4;
    int ProtocolInfo::rcv_Audio_NUMBER_2 = 1;
    int ProtocolInfo::rcv_Audio_AUDIO_3 = 249;

    int ProtocolInfo::rcv_Disconnect_ID = 4;

namespace protocol
{

    namespace tovoipserver
    {

        data::buffer constructGeralData(int my_id, int id_size, char headerNumber)
        {
            data::buffer buffer(ptc_header_size + id_size);

            tools::transformDataToBufferPos(&headerNumber, buffer.getData(), 0);
            tools::transformDataToBufferPos(&my_id, buffer.getData(), ptc_header_size);

            return buffer;
        }

        data::buffer constructHandChackeData(int register_id, int my_id){

            data::buffer buffer(ptc_header_size + ProtocolInfo::snd_HandChacke_REGISTER_ID + ProtocolInfo::snd_HandChacke_ID);

            char HeaderHandChacke = (char)protocolinfo::send::header::Headers::HandChacke;
            tools::transformDataToBufferPos(&HeaderHandChacke, buffer.getData(), 0);
            tools::transformDataToBufferPos(&register_id, buffer.getData(), ptc_header_size);
            tools::transformDataToBufferPos(&my_id, buffer.getData(), ptc_header_size + ProtocolInfo::snd_HandChacke_REGISTER_ID);

            return buffer;
        }

        data::buffer constructDisconnectData(int my_id){
            char HeaderDisconnect = (char)protocolinfo::send::header::Headers::Disconnect;
            return constructGeralData(my_id, ProtocolInfo::snd_Disconnect_ID_1, HeaderDisconnect);
        }

        data::buffer constructPosData(int my_id, int map, int x, int y, int z)
        {
            int pos_id = ptc_header_size;
            int pos_map = pos_id + ProtocolInfo::snd_Pos_id_1;
            int pos_x = pos_map + ProtocolInfo::snd_Pos_Map_2;
            int pos_y = pos_x + ProtocolInfo::snd_Pos_X_3;
            int pos_z = pos_y + ProtocolInfo::snd_Pos_Y_4;
            data::buffer buffer(pos_z + ProtocolInfo::snd_Pos_Z_5);

            char HeaderPos = (char)protocolinfo::send::header::Headers::Position;
            tools::transformDataToBufferPos(&HeaderPos, buffer.getData(), 0);
            tools::transformDataToBufferPos(&my_id, buffer.getData(), pos_id);
            tools::transformDataToBufferPos(&map, buffer.getData(), pos_map);
            tools::transformDataToBufferPos(&x, buffer.getData(), pos_x);
            tools::transformDataToBufferPos(&y, buffer.getData(), pos_y);
            tools::transformDataToBufferPos(&z, buffer.getData(), pos_z);

            return buffer;
        }

        data::buffer constructAudioData(int my_id, char type, data::buffer &buffer_audio)
        {

            int pos_id = ptc_header_size;
            int pos_type = pos_id + ProtocolInfo::snd_Audio_ID_1;
            int pos_audio = pos_type + ProtocolInfo::snd_Audio_TYPE_2;
            data::buffer buffer(pos_audio + buffer_audio.size());

            char HeaderAudio = (char)protocolinfo::send::header::Headers::Audio;
            tools::transformDataToBufferPos(&HeaderAudio, buffer.getData(), 0);
            tools::transformDataToBufferPos(&my_id, buffer.getData(), pos_id);
            tools::transformDataToBufferPos(&type, buffer.getData(), pos_type);
            tools::transformArrayToBuffer(buffer_audio.getData(), buffer_audio.size(), buffer.getData() + pos_audio);

            return buffer;
        }

    }

    namespace fromvoipserver
    {

        protocol::rcv_Audio_stc constructRCVaudioData(data::buffer &buffer)
        {

            int pos_Id = ptc_header_size;
            int pos_Number = pos_Id + ProtocolInfo::rcv_Audio_ID_1;
            int pos_Audio = pos_Number + ProtocolInfo::rcv_Audio_NUMBER_2;
            
            protocol::rcv_Audio_stc audioDataPK(buffer.size() - pos_Audio);
            char audio_number = 0;
            
            tools::bufferCutToData(&audioDataPK.id, buffer.getData(), pos_Id, ProtocolInfo::rcv_Audio_ID_1);
            tools::bufferCutToData(&audio_number, buffer.getData(), pos_Number, ProtocolInfo::rcv_Audio_NUMBER_2);
            audioDataPK.audio_number = audio_number;
            tools::bufferCutToData<char>(audioDataPK.audioData.getData(), buffer.getData(), pos_Audio, audioDataPK.audioData.size());
            
            return audioDataPK;
        }

        protocol::rcv_HandChacke_stc constructRCVhandChackeData(data::buffer &buffer)
        {
            protocol::rcv_HandChacke_stc handchackeData;

            int pos_Id = ptc_header_size;

            tools::bufferCutToData<int>(&handchackeData.id, buffer.getData(), pos_Id, ProtocolInfo::rcv_HandChacke_ID);

            return handchackeData;
        }

        /**
         * @brief 
         * 
         * @param buffer data
         * @return public ID
         */
        int constructRCVdisconnectData(data::buffer &buffer)
        {
            int myID;

            int pos_Id = ptc_header_size;

            tools::bufferCutToData<int>(&myID, buffer.getData(), pos_Id, ProtocolInfo::rcv_Disconnect_ID);

            return myID;
        }

    }
}