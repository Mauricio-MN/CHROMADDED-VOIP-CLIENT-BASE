#ifndef DATA_H // include guard
#define DATA_H

namespace data
{

/**
    * Implementation of buffer class
    *
    * Initialize by reference generate a copy class and copy buffer with new owner,
    * just end of scope for owner(first) object delete the buffer,
    * secure for pass by value if owner is live,
    *
    */
    class buffer
    {
        protected:
            bool valid = false;
            char* bufferData;
            size_t validAddressOwner;
            int owner;
        public:

            size_t size;

            char *getBuffer(){
                if(valid){
                    return bufferData;
                }
                return nullptr;
            }

            ~buffer()
            {
                if(validAddressOwner == reinterpret_cast<size_t>(&owner)){
                    delete []bufferData;
                    valid = false;
                }
            }
            
            buffer(size_t len){
                if(valid == false){
                    bufferData = new char[len];
                    size = len;
                    valid = true;
                    owner = 1;
                    validAddressOwner = reinterpret_cast<size_t>(&owner);
                }
            }

            buffer(buffer* Data){
                if(valid == false){
                    bufferData = new char[Data->size];
                    size = Data->size;
                    memcpy(bufferData,Data->bufferData, sizeof(char)*size);
                    valid = true;
                    owner = 1;
                    validAddressOwner = reinterpret_cast<size_t>(&owner);
                }
            }

            buffer(const buffer& Data){
                if(valid == false){
                    bufferData = new char[Data.size];
                    size = Data.size;
                    memcpy(bufferData,Data.bufferData, sizeof(char)*size);
                    valid = true;
                    owner = 1;
                    validAddressOwner = reinterpret_cast<size_t>(&owner);
                }
            }

            buffer(char *refBuffer, size_t len){
                if(valid == false){
                    bufferData = new char[len];
                    size = len;
                    memcpy(bufferData,refBuffer, sizeof(char)*size);
                    valid = true;
                    owner = 1;
                    validAddressOwner = reinterpret_cast<size_t>(&owner);
                }
            }

            buffer(){
                valid = false;
            }

            bool isValid(){
                return valid;
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
            buffer* buff;
            bool valid = false;
        public:

            size_t size;

            void init(Audio_player_data &audiodata){
                buff = new buffer(audiodata.buff);
                size = buff->size;
                valid = true;
            }

            Audio_player_data(buffer* audiobuff){
                buff = new buffer(audiobuff);
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

    }
#endif