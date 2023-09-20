
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

#include "crpt.h"
#include "player.h"
#include "data.h"
#include "opusmanager.h"
#include "socketUdp.h"

    protocolParser& protocolParserImpl::getInstance(){
      static protocolParser instance;
      return instance;
    }

void protocolParser::parser_Thread()
{
  while(runThreads){
    
    TrivialContainerProtocolServer container;
    queueProtc.Pop(container);
    protocol::Server serverReceived(*container.data);
    delete container.data;

    if(serverReceived.has_notconnected()){
      if(serverReceived.notconnected()){
        socketUdpImpl::refabric();
      }
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

      } else {

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
            int audioSize = serverReceived.audio().size();
            data::buffer audioData(serverReceived.audio().data(), audioSize);

            PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.id());
            data::buffer decodedAud = OpusManagerImpl::getInstance().decode(audioData, audioSize);
            int audioNum = 0;
            if(serverReceived.has_audionum()){
              audioNum = serverReceived.audionum();
            }
            player->push(audioNum, decodedAud);
        }

      }
    } else {
      if(serverReceived.handshake()){
          if(serverReceived.id() == player::SelfImpl::getInstance().getMyID()){
              player::SelfImpl::getInstance().setConnect(true);
          }
      }
    } 

  }

}

void protocolParser::parse(protocol::Server *serverReceived){
  TrivialContainerProtocolServer container;
  container.data = new protocol::Server(*serverReceived);
  queueProtc.Push(container);
}

protocolParser::protocolParser(){
  runThreads = true;
  for(int i = 0; i < 1; i++){
    parserThread.push_back(std::thread(&protocolParser::parser_Thread, this));
  }
}

protocolParser::~protocolParser(){
  runThreads = false;
  for(auto& lThread : parserThread){
    lThread.join();
  }
}