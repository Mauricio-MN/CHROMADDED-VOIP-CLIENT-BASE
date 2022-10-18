#ifndef PROTOCOLTOOLS_H // include guard
#define PROTOCOLTOOLS_H

#include <string.h>

namespace protocol::tools
    {

        inline void cutBuffer(char *dest, char *source, int from, int sizeOfslice)
        {
            strncpy(dest, source + from, sizeof(char) * sizeOfslice);
        }

        template <typename T>
        inline void transformArrayToBuffer(T *data, size_t size, char *buffer)
        {
            memcpy(buffer, data, sizeof(T) * size);
        }

        template <typename T>
        inline void transformDataToBuffer(T *data, char *buffer)
        {
            memcpy(buffer, data, sizeof(T));
        }

        template <typename T>
        inline void transformDataToBufferPos(T *data, char *buffer, size_t pos)
        {
            memcpy(buffer + pos, data, sizeof(T));
        }

        inline void bufferToData(void *data, size_t size_buffer, char *buffer)
        {
            memcpy(data, buffer, sizeof(char) * size_buffer);
        }

        inline void DataToData(void *data_dest, void *data_src, size_t data_src_type_size, size_t data_src_array_size)
        {
            memcpy(data_dest, data_src, data_src_type_size * data_src_array_size);
        }

        template <typename T>
        inline void bufferCutToData(T *data, char *buffer, size_t from, size_t sizeOfslice)
        {
            memcpy(data, buffer + from, sizeof(char)*sizeOfslice);
        }

        template <typename T>
        inline void mergeDatasArray(T *dataA, size_t size_a, T *dataB, size_t size_b, T *dataOut)
        {
            memcpy(dataOut, dataA, sizeof(T) * size_a);
            memcpy(dataOut + (sizeof(T) * size_a), dataB, sizeof(T) * size_b);
        }

        template <typename T>
        inline bool cutbufferToType(T *data, char *buffer, size_t pos, size_t sizeOfslice)
        {
            if (pos < 0)
                return false;
            if (sizeOfslice < 0)
                return false;
            memcpy(data, buffer + pos, sizeof(char) * sizeOfslice);
            return true;
        }

    }

#endif