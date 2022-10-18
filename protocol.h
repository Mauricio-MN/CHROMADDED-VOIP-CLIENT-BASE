#ifndef PROTOCOL_H // include guard
#define PROTOCOL_H

#include <string.h>
#include <memory>
#include <mutex>
#include <thread>

namespace protocol
{

    void init();

    const char snd_HandChacke = 0;
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
    inline int rcv_Disconnect_ID = 4;

#define header_size 1

    /**
    * Implementation of buffer class
    *
    * Initialize by reference generate a copy class and copy buffer with new owner,
    * just end of scope for owner(first) object delete the buffer,
    * secure for pass by value if owner is live,
    *
    */
    class data
    {
        private:
            bool valid = false;
            char* buffer;
            size_t validAddressOwner;
            int owner;
        public:

            size_t size;

            char *getBuffer(){
                if(valid){
                    return buffer;
                }
                return nullptr;
            }

            ~data()
            {
                if(validAddressOwner == reinterpret_cast<size_t>(&owner)){
                    delete []buffer;
                    valid = false;
                }
            }
            
            data(size_t len){
                if(valid == false){
                    buffer = new char[len];
                    size = len;
                    valid = true;
                    owner = 1;
                    validAddressOwner = reinterpret_cast<size_t>(&owner);
                }
            }

            data(data* Data){
                if(valid == false){
                    buffer = new char[Data->size];
                    size = Data->size;
                    memcpy(buffer,Data->buffer, sizeof(char)*size);
                    valid = true;
                    owner = 1;
                    validAddressOwner = reinterpret_cast<size_t>(&owner);
                }
            }

            data(const data& Data){
                if(valid == false){
                    buffer = new char[Data.size];
                    size = Data.size;
                    memcpy(buffer,Data.buffer, sizeof(char)*size);
                    valid = true;
                    owner = 1;
                    validAddressOwner = reinterpret_cast<size_t>(&owner);
                }
            }

            data(){
                valid = false;
            }

            bool isValid(){
                return valid;
            }

            bool assign(char *callNewHere, size_t len){
                if(valid == false){
                    valid = true;
                    buffer = callNewHere;
                    size = len;
                    owner = 1;
                    validAddressOwner = reinterpret_cast<size_t>(&owner);
                    return true;
                }
                return false;
            }
        
    };

    /**
    * Implementation of audio buffer class
    *
    * Initialize by reference generate a copy class of 'data' and copy buffer with new owner,
    * !IMPORTANT! no 'new' allocator.
    * Call '.destroy()' on end off responsability.
    * 
    */
    class Audio_player_data{
        private:
            data* buff;
            bool valid = false;
        public:

            size_t size;

            void init(Audio_player_data &audiodata){
                buff = new data(audiodata.buff);
                size = buff->size;
                valid = true;
            }

            Audio_player_data(data* audiobuff){
                buff = new data(audiobuff);
                size = buff->size;
                valid = true;
            }

            Audio_player_data(){
                valid = false;
                buff = nullptr;
            }

            char *getBuffer(){
                if(valid){
                    return buff->getBuffer();
                }
                return nullptr;
            }

            void destroy(){
                if(valid == true){
                    delete buff;
                    valid = false;
                }
            }
            
        private:
            void * operator new(size_t size)
            {
                return nullptr;
            }

            void * operator new[] (size_t size)
            {
                return nullptr;
            }

            void   operator delete   (void *);
            void   operator delete[] (void*);
    };

    typedef struct rcv_Audio_stc
    {
        int id = -1;
        int audio_number = -1;
        data *audioData;
        size_t validAddressOwner;
        int owner;

        rcv_Audio_stc(size_t audioData_size){
            audioData = new data(audioData_size);
            owner = 1;
            validAddressOwner = reinterpret_cast<size_t>(&owner);
        }

        rcv_Audio_stc(rcv_Audio_stc* RCV){
            audioData = new data(RCV->audioData);
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

        data constructHandChackeData(int my_id);

        data constructPosData(int my_id, int map, int x, int y, int z);

        data constructAudioData(int my_id, char type, data *buffer_audio);

        data constructDisconnectData(int my_id);

    }

    namespace fromvoipserver
    {

        rcv_Audio_stc constructRCVaudioData(data buffer);

        rcv_HandChacke_stc constructRCVhandChackeData(data buffer);

    }
}

#include "protocolTools.h"

#endif