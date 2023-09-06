
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
#include "socketUdp.h"

#include "protoParse.h"

    protocolParser& protocolParserImpl::getInstance(){
      static protocolParser instance;
      return instance;
    }

void protocolParser::parserThread(protocol::Server serverReceived)
{
  if(serverReceived.has_notconnected()){
    if(serverReceived.notconnected()){
      socketUdpImpl::refabric();
    }
  }

  if(!serverReceived.has_handshake() || serverReceived.handshake() == false){
    if(PlayersManagerImpl::getInstance().existPlayer(serverReceived.id())){
      coords coordinate = player::SelfImpl::getInstance().getCoords();
      if(serverReceived.has_coordx()){
          coordinate.x = serverReceived.coordx();
      }
      if(serverReceived.has_coordy()){
          coordinate.x = serverReceived.coordy();
      }
      if(serverReceived.has_coordz()){
          coordinate.x = serverReceived.coordz();
      }

      if(serverReceived.has_audio()){
        int audioSize = serverReceived.audio().size();
        data::buffer audioData(serverReceived.audio().data(), audioSize);

        if (PlayersManagerImpl::getInstance().existPlayer(serverReceived.id()))
        {
            PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.id());
            data::buffer decodedAud = OpusManagerImpl::getInstance().decode(audioData, audioSize);
            player->push(decodedAud);
        } else {
            PlayersManagerImpl::getInstance().insertPlayer(serverReceived.id(), 0, 0, 0);
            PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.id());
        }
      }

    } else {
      int sampleTime = SAMPLE_TIME_DEFAULT;
      if(serverReceived.has_sampletime()){
        sampleTime = serverReceived.sampletime();
      }
      PLAYER player = PLAYER(new Player(sf::milliseconds(sampleTime)));
      player->id = serverReceived.id();
      player->move(0,0,0);
      PlayersManagerImpl::getInstance().insertPlayer(player);
    } 
  } else {
    if(serverReceived.handshake()){
        if(serverReceived.id() == player::SelfImpl::getInstance().getMyID()){
            player::SelfImpl::getInstance().setConnect(true);
        }
    }
  } 
  
  if(serverReceived.has_audio()){

    int audioSize = serverReceived.audio().size();
    data::buffer audioData(serverReceived.audio().data(), audioSize);

    if (PlayersManagerImpl::getInstance().existPlayer(serverReceived.id()))
    {
        PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.id());
        data::buffer decodedAud = OpusManagerImpl::getInstance().decode(audioData, audioSize);
        player->push(decodedAud);
    } else {
        PlayersManagerImpl::getInstance().insertPlayer(serverReceived.id(), 0, 0, 0);
        PLAYER player = PlayersManagerImpl::getInstance().getPlayer(serverReceived.id());
    }
  }

}

void protocolParser::parse(protocol::Server *serverReceived){
  std::thread(&parserThread, this, *serverReceived).detach();
}

protocolParser::protocolParser(){
}