#include <thread>

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "SoundCustomBufferRecorder.hpp"

#include "connection.h"
#include "soundmanager.h"
#include "player.h"

namespace soundmanager{

namespace listener{
  void movePos(float x, float y, float z){
      sf::Listener::setPosition(x,y,z);
  }

  void moveRot(float x, float y, float z){
      sf::Listener::setDirection(x,y,z);
  }
}

sf::SoundCustomBufferRecorder recorder::rec;

void recorder::enableRec(){
  if(!rec.recording){
      rec.start(SAMPLE_RATE);
    }
}

void recorder::disableRec(){
  if(rec.recording) {
      rec.stop();
    }
}

void recorder::record(){
  recordMng(&players::self::sendAudio);
}

void recorder::recordMng(void (*func)DEFAULT_BUFFER_ARGS){

  rec.setChannelCount(SAMPLE_CHANNELS);
  rec.setProcessingIntervalOverride(sf::milliseconds(50));
  rec.setListen(DEBUG_AUDIO);
  rec.setProcessingBufferFunction(func);
  rec.start(SAMPLE_RATE);

}

sf::SoundBuffer recorder::recordForTest(){
  sf::SoundBufferRecorder Trec;
  Trec.setChannelCount(SAMPLE_CHANNELS);
  Trec.start(SAMPLE_RATE);
  sf::sleep(sf::milliseconds(10000));
  Trec.stop();
  return Trec.getBuffer();
}

void recorder::start(){
    record();
}

}