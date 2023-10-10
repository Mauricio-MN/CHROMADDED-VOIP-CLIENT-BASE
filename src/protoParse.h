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
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "proto/protocol.pb.h"

#include "queue.hpp"
#include "data.h"

#define TOTAL_THREAD_PARSER 32

struct TrivialContainerProtocolServer {
    protocol::Server* data; // Ponteiro para um objeto não trivial
};

class protocolParser{
private:

    data::parseThreadPoll threadPool;

public:

    static void m_Parser_only THREAD_POOL_ARGS_NAMED;

    data::parseThreadPoll& getPool();

    protocolParser();

    ~protocolParser();

    void parse(protocol::Server& serverReceived);

};

class protocolParserImpl{
    public:
    static protocolParser& getInstance();
};

#endif