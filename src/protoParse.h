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

#include "data.h"

#define TOTAL_THREAD_PARSER 32

#define chronoTimePointSteady std::chrono::time_point<std::chrono::steady_clock,std::chrono::steady_clock::duration>
#define chronoDurationSteady std::chrono::steady_clock::duration

struct TrivialContainerProtocolServer {
    protocol::Server* data; // Ponteiro para um objeto não trivial
};

class ProtocolParser{
private:

    data::parseThreadPoll threadPool;

    static std::shared_mutex timeDiffServer_mutex;
    static std::chrono::system_clock::duration timeDiffServer;

    static std::shared_mutex connectSendTime_mutex;
    static std::chrono::system_clock::time_point connectSendTime;

public:

    static std::atomic_int invalidCalsFromServer;

    static void m_Parser_only THREAD_POOL_ARGS_NAMED;

    data::parseThreadPoll& getPool();

    ProtocolParser();

    ~ProtocolParser();

    void parse(protocol::Server& serverReceived);

    //Thread safe
    static int getLossPercent();
    //Thread safe
    static int getTrotling();
    //readOnly
    static bool isNegativeTimeDiffServer;
    //Thread safe, but is shared_mutex
    static void setTimeDiffServer(std::chrono::system_clock::duration time, bool isNegative);
    //Thread safe, but is shared_mutex
    static std::chrono::system_clock::duration getTimeDiffServer();

    //Thread safe, but is shared_mutex
    static void setTimeConnectSend(std::chrono::system_clock::time_point time);
    //Thread safe, but is shared_mutex
    static std::chrono::system_clock::time_point getTimeConnectSend();

    static void calcTimeDiffServer_andSet(std::chrono::system_clock::time_point serverTime);

};

#endif