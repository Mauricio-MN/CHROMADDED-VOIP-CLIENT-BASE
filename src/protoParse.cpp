
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
//#include "../libs/SDL2/SDL.h"
#include <sys/time.h>
#include <ctime>
#include <thread>
#include <mutex>
#include <memory>
#include <future>

#include "osimports.h"

#include "player.h"
#include "data.h"
#include "opusmanager.h"
#include "socketUdp.h"

protocolParser& protocolParserImpl::getInstance(){
  static protocolParser instance;
  return instance;
}

void protocolParser::m_Parser_only THREAD_POOL_ARGS_NAMED{
  OpusManager opusMng;

  protocol::Server serverReceived;

  bool success = threadPoolL.pop(myid, serverReceived);

  if(!success){
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    return;
  }

  if(!serverReceived.has_handshake() || serverReceived.handshake() == false){
      if(!PlayersManagerImpl::getInstance().existPlayer(serverReceived.id())){
        int sampleTime = SAMPLE_TIME_DEFAULT;
        if(serverReceived.has_sampletime()){
          sampleTime = serverReceived.sampletime();
        }
        PLAYER player = PLAYER(new Player(sf::milliseconds(sampleTime)));
        player->id = serverReceived.id();
        player->move(0,0,0);
        PlayersManagerImpl::getInstance().insertPlayer(player);

      }

      Coords coordinate = player::SelfImpl::getInstance().getCoords();
      bool altCoord = false;
      if(serverReceived.has_coordx()){
          coordinate.x = serverReceived.coordx();
          altCoord = true;
      }
      if(serverReceived.has_coordy()){
          coordinate.x = serverReceived.coordy();
          altCoord = true;
      }
      if(serverReceived.has_coordz()){
          coordinate.x = serverReceived.coordz();
          altCoord = true;
      }
      if(altCoord){
        player::SelfImpl::getInstance().setCoords(coordinate);
      }

      if(serverReceived.has_audio()){
          PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.id());

          int audioNum = 0;
          int sampleTime = SAMPLE_TIME_DEFAULT;
          if(serverReceived.has_audionum()){
            audioNum = serverReceived.audionum();
          }
          if(serverReceived.has_sampletime()){
            sampleTime = serverReceived.sampletime();
          }

          int sampleCount = sampleTimeGetCount(sampleTime, SAMPLE_RATE);
          data::buffer decodedAud = opusMng.decode(
            reinterpret_cast<const unsigned char*>(serverReceived.audio().data()), serverReceived.audio().size(), sampleCount);

          player->push(audioNum, decodedAud, sampleTime);
      }

    } else {
      if(serverReceived.handshake()){
          if(serverReceived.id() == player::SelfImpl::getInstance().getMyID()){
              player::SelfImpl::getInstance().setConnect(true);
          }
      }
    }
}

void protocolParser::parse(protocol::Server& serverReceived){
  //std::thread(&protocolParser::m_Parser_only, this, serverReceived).detach();

  if(!threadPool.exist(serverReceived.id())){
    threadPool.insert(serverReceived.id(), &protocolParser::m_Parser_only);
  }
  threadPool.push(serverReceived.id(), serverReceived);
  //std::thread(&protocolParser::parser_Finder, this, serverReceived).detach();
}

data::parseThreadPoll& protocolParser::getPool(){
  return threadPool;
}

protocolParser::protocolParser(){

}

protocolParser::~protocolParser(){
  
}