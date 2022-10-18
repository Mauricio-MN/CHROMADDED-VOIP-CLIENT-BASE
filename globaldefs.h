#ifndef GLOBADEFS_H // include guard
#define GLOBADEFS_H

enum AudioType
    {
        LOCAL,
        GROUP
    };

inline char AudioTypeToChar(AudioType type){

    switch (type)
    {
        case AudioType::LOCAL:
        {
            return (char)0x1;
        }
        case AudioType::GROUP:
        {
            return (char)0x2;
        }
        default:
        {
            return (char)0x1;
        }
    }

}

#endif