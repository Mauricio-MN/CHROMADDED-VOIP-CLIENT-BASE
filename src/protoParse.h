#ifndef PROTOPARSE_H // include guard
#define PROTOPARSE_H

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

#include "proto/protocol.pb.h"

#define TOTAL_THREAD_PARSER 16

class protocolParser{
private:

    void parserThread(protocol::Server serverReceived);

    void tempDataWait(int id, data::buffer buffer);

public:

    protocolParser();

    void parse(protocol::Server *serverReceived);

};

class protocolParserImpl{
    public:
    static protocolParser& getInstance();
};

protocol::Client constructValidBufferSend(){
    protocol::Client client;
    client.set_integritycheck(956532);
}

#endif