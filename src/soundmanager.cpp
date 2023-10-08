#include <thread>

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "SoundCustomBufferRecorder.hpp"

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

Recorder &RecorderImpl::getInstance()
{
  static Recorder instance;
  return instance;
}

void Recorder::enableRec(){
  rec.enableProcessSound();
}

void Recorder::disableRec(){
  rec.disableProcessSound();
}

sf::SoundBuffer Recorder::recordForTest(){
  rec.stop();

  sf::SoundBufferRecorder Trec;
  Trec.setChannelCount(SAMPLE_CHANNELS);
  Trec.start(SAMPLE_RATE);
  sf::sleep(sf::milliseconds(10000));
  Trec.stop();
  return Trec.getBuffer();
}

Recorder::Recorder(){
  Recorder(SAMPLE_RATE, sf::milliseconds(20));
}

Recorder::Recorder(int _sampleRate, sf::Time packetTime){
  initialize(_sampleRate, packetTime);
}

void Recorder::reConstruct(int _sampleRate, sf::Time packetTime){
  rec.stop();
  initialize(_sampleRate, packetTime);
}

int Recorder::getSampleTime(){
  return packetTime.asMilliseconds();
}
int Recorder::getSampleCount(){
  return sampleTimeGetCount(packetTime,sampleRate);
}

void Recorder::initialize(int _sampleRate, sf::Time packetTime){
  rec.setChannelCount(SAMPLE_CHANNELS);
  rec.setProcessingIntervalOverride(packetTime);
  rec.setListen(DEBUG_AUDIO);
  rec.setProcessingBufferFunction(player::Self::sendAudio);
  rec.start(_sampleRate);
  sampleRate = _sampleRate;
}

}