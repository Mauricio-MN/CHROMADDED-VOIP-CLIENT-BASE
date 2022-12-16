#ifndef BUFFERPARSER_H // include guard
#define BUFFERPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
//#include "../libs/SDL2/SDL.h"
#include <sys/time.h>
#include <ctime>
#include <queue>
#include <thread>
#include <mutex>

#include "osimports.h"

#include "cript.h"
#include "protocol.h"
#include "player.h"

#define TOTAL_THREAD_PARSER 16

class Bufferparser{
private:

    void parserThread(data::buffer buffer);

    void tempDataWait(int id, data::buffer buffer);

public:

    Bufferparser();

    void parserBuffer(data::buffer* buffer);

};

class BufferParserImpl{
    public:
    static Bufferparser& getInstance();
};

#endif