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

namespace bufferparser{

inline std::queue<data::buffer> continuousProcessDataQueue[TOTAL_THREAD_PARSER];
inline std::mutex continuousProcessDataMutex[TOTAL_THREAD_PARSER];
inline std::thread continuousProcessDataThreads[TOTAL_THREAD_PARSER];

inline int selectIdDataWait = 0;

inline void ProcessData(int id);

void parserThread(data::buffer buffer);

void parserBuffer(data::buffer* buffer);

void tempDataWait(int id, data::buffer buffer);

void parser(data::buffer *buffer);

void allocContinuousProcessDataThreads();

int getProcessingDataCount(int i);

void init();

}

#endif