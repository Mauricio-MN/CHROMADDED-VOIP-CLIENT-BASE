
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
#include <future>

#include "osimports.h"

#include "cript.h"
#include "protocol.h"
#include "player.h"

#include "bufferParser.h"

namespace bufferparser{

void continuousProcessData(int id)
{
  while (true)
  {
    ProcessData(id);
  }
}

void ProcessData(int id){
  continuousProcessDataMutex[id].lock();
  if (continuousProcessDataQueue[id].empty() == false)
    {
      protocol::data buffer = continuousProcessDataQueue[id].front();
      continuousProcessDataQueue[id].pop();
      continuousProcessDataMutex[id].unlock();

      switch ((int)(buffer.buffer[0]))
      {
      case protocol::rcv_Audio:
      {
        protocol::rcv_Audio_stc receivedAudioSTC =
            protocol::fromvoipserver::constructRCVaudioData(buffer.buffer, buffer.size);

        if (players::manager::existPlayer(receivedAudioSTC.id))
        {
          players::player *player;
          players::manager::getPlayer(receivedAudioSTC.id, player);
          player->push(&receivedAudioSTC.audioData);
        }
        else
        {
          players::manager::insertPlayer(receivedAudioSTC.id, 0, 0, 0);
          players::player *player;
          players::manager::getPlayer(receivedAudioSTC.id, player);
        }
        break;
      }
      case protocol::rcv_HandChacke:
      {
        protocol::rcv_HandChacke_stc receivedHandChackeSTC =
            protocol::fromvoipserver::constructRCVhandChackeData(buffer.buffer, buffer.size);

        break;
      }
      default:
        int a = 0;
      }
    } else {
    continuousProcessDataMutex[id].unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void tempDataWait(int id, protocol::data buffer){
  continuousProcessDataMutex[id].lock();
  continuousProcessDataQueue->push(buffer);

  if(continuousProcessDataQueue->size() > 3){
    continuousProcessDataMutex[id].unlock();
    std::async(ProcessData, id);
  } else {
    continuousProcessDataMutex[id].unlock();
  }

}

void parser(protocol::data *buffer){
  selectIdDataWait++;
  if (selectIdDataWait >= TOTAL_THREAD_PARSER) selectIdDataWait = 0;
  std::async(tempDataWait, selectIdDataWait, *buffer);
}

void allocContinuousProcessDataThreads(){
  for (int i = 0; i < TOTAL_THREAD_PARSER; i++){
    std::async(continuousProcessData, i);
  }
}

void init(int id){
    my_id = id;
    allocContinuousProcessDataThreads();
}

}