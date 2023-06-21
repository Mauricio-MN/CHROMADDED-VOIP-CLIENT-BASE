
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
#include "data.h"
#include "opusmanager.h"

#include "bufferParser.h"

    Bufferparser& BufferParserImpl::getInstance(){
      static Bufferparser instance;
      return instance;
    }

void Bufferparser::parserThread(data::buffer buffer)
{

  using namespace protocolinfo::receive::header;
  Headers header = static_cast<Headers>((int)(buffer.getData()[0]));

  switch (header)
  {
  case Audio:
  {
    protocol::rcv_Audio_stc receivedAudioSTC =
        protocol::fromvoipserver::constructRCVaudioData(buffer);

    if (PlayersManagerImpl::getInstance().existPlayer(receivedAudioSTC.id))
    {
      PLAYER player = PlayersManagerImpl::getInstance().getPlayer(receivedAudioSTC.id);
      data::buffer decodedAud = OpusManagerImpl::getInstance().decode(receivedAudioSTC.audioData);
      player->push(decodedAud);
      //player->push(receivedAudioSTC);
    }
    else
    {
      PlayersManagerImpl::getInstance().insertPlayer(receivedAudioSTC.id, 0, 0, 0);
      PLAYER player = PlayersManagerImpl::getInstance().getPlayer(receivedAudioSTC.id);
    }

    break;
  }
  case HandChacke:
  {
    protocol::rcv_HandChacke_stc receivedHandChackeSTC =
        protocol::fromvoipserver::constructRCVhandChackeData(buffer);

    break;
  }
  default:
    {
      return;
    }
  }
  return;
}

void Bufferparser::parserBuffer(data::buffer* buffer){
  std::thread(&parserThread, this, *buffer).detach();
}

Bufferparser::Bufferparser(){
}