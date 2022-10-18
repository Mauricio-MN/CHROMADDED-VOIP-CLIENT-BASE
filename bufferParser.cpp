
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
#include <memory>

#include "osimports.h"

#include "cript.h"
#include "protocol.h"
#include "player.h"

#include "bufferParser.h"

namespace bufferparser{

void ProcessData(int id){
  while(true){
    continuousProcessDataMutex[id].lock();
    if (continuousProcessDataQueue[id].empty() == false)
    {
        protocol::data buffer(continuousProcessDataQueue[id].front());
        continuousProcessDataQueue[id].pop();
        continuousProcessDataMutex[id].unlock();

        switch ((int)(buffer.getBuffer()[0]))
        {
        case protocol::rcv_Audio:
        {
          protocol::rcv_Audio_stc receivedAudioSTC =
              protocol::fromvoipserver::constructRCVaudioData(buffer);

          if (players::manager::existPlayer(receivedAudioSTC.id))
          {
            players::player *player = players::manager::getPlayer(receivedAudioSTC.id);
            //player->push(receivedAudioSTC);
            player->push(receivedAudioSTC.audioData);
          }
          else
          {
            players::manager::insertPlayer(receivedAudioSTC.id, 0, 0, 0);
            players::player *player = players::manager::getPlayer(receivedAudioSTC.id);
          }

          break;
        }
        case protocol::rcv_HandChacke:
        {
          protocol::rcv_HandChacke_stc receivedHandChackeSTC =
              protocol::fromvoipserver::constructRCVhandChackeData(buffer);

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
}

void parserThread(protocol::data buffer)
{
  switch ((int)(buffer.getBuffer()[0]))
  {
  case protocol::rcv_Audio:
  {
    protocol::rcv_Audio_stc receivedAudioSTC =
        protocol::fromvoipserver::constructRCVaudioData(buffer);

    if (players::manager::existPlayer(receivedAudioSTC.id))
    {
      players::player *player = players::manager::getPlayer(receivedAudioSTC.id);
      player->push(receivedAudioSTC.audioData);
      //player->push(receivedAudioSTC);
    }
    else
    {
      players::manager::insertPlayer(receivedAudioSTC.id, 0, 0, 0);
      players::player *player = players::manager::getPlayer(receivedAudioSTC.id);
    }

    break;
  }
  case protocol::rcv_HandChacke:
  {
    protocol::rcv_HandChacke_stc receivedHandChackeSTC =
        protocol::fromvoipserver::constructRCVhandChackeData(buffer);

    break;
  }
  default:
    int a = 0;
  }
  return;
}

void parserBuffer(protocol::data* buffer){
  std::thread(parserThread, *buffer).detach();
}

int getProcessingDataCount(int i){
  return continuousProcessDataQueue[i].size();
}

void tempDataWait(int id, protocol::data buffer){
  continuousProcessDataMutex[id].lock();
  continuousProcessDataQueue[id].push(buffer);
  continuousProcessDataMutex[id].unlock();
}

void parser(protocol::data *buffer){
  selectIdDataWait++;
  if (selectIdDataWait >= TOTAL_THREAD_PARSER) selectIdDataWait = 0;
  //std::async(tempDataWait, selectIdDataWait, *buffer);
  //std::thread(tempDataWait, selectIdDataWait, *buffer).detach();
  continuousProcessDataMutex[selectIdDataWait].lock();
  continuousProcessDataQueue[selectIdDataWait].push(buffer);
  continuousProcessDataMutex[selectIdDataWait].unlock();
}

void allocContinuousProcessDataThreads(){
  for (int i = 0; i < TOTAL_THREAD_PARSER; i++){
    //std::async(ProcessData, i);
    std::thread(ProcessData, i).detach();
  }
}

void init(int id){
    my_id = id;
    //allocContinuousProcessDataThreads();
}

}