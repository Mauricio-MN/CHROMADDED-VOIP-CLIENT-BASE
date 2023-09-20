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

#include "crpt.h"
#include "player.h"
#include <thread>
#include <queue>
//#include <mutex>
#include <shared_mutex>

#include "proto/protocol.pb.h"

#include "queue.hpp"

#define TOTAL_THREAD_PARSER 16

struct TrivialContainerProtocolServer {
    protocol::Server* data; // Ponteiro para um objeto não trivial
};

class protocolParser{
private:

    void parser_Thread();

    void tempDataWait(int id, data::buffer buffer);

    std::vector<std::thread> parserThread;
    bool runThreads;

    std::queue<protocol::Server> queueProtocol;
    lockfree::spsc::Queue<TrivialContainerProtocolServer, 256> queueProtc;
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

#endif