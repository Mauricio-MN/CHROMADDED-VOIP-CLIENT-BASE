#ifndef GLOBADEFS_H // include guard
#define GLOBADEFS_H

#define ERROR_NO_ERROR        0
#define ERROR_LOW             10
#define ERROR_MEDIUM          20 //
#define ERROR_SERIOUS         30
#define ERROR_ADDRESS_PROBLEM 40
#define ERROR_MAX_ATTEMPT     50
#define ERROR_UNKNOW          999

enum AudioType
    {
        LOCAL = 1,
        PLAYERS_GROUP = 2
    };

inline char AudioTypeToChar(AudioType type){

    return (char)type;

}

struct coords{
    int x;
    int y;
    int z;
    int map;
};

#endif