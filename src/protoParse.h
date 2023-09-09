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
#include "player.h"
#include <thread>
#include <queue>
//#include <mutex>
#include <shared_mutex>

#include "proto/protocol.pb.h"

#define TOTAL_THREAD_PARSER 16

class protocolParser{
private:

    void parser_Thread();

    void tempDataWait(int id, data::buffer buffer);

    std::vector<std::thread> parserThread;
    bool runThreads;

    std::queue<protocol::Server> queueProtocol;
    std::shared_mutex queueMutex;

public:

    protocolParser();

    ~protocolParser();

    void parse(protocol::Server *serverReceived);

};

class protocolParserImpl{
    public:
    static protocolParser& getInstance();
};

inline protocol::Client constructValidBufferSend(){
    protocol::Client client;
    client.set_integritycheck(956532);
}

#endif