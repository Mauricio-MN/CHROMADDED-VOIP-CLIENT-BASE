#include <thread>

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "soundmanagerRecorder.h"

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

bool RecorderImpl::initialized = false;
int RecorderImpl::sampleRate = SAMPLE_RATE;
sf::Time RecorderImpl::packetTime = sf::milliseconds(20);
Recorder* RecorderImpl::instance = nullptr;

Recorder &RecorderImpl::getInstance()
{
  if (initialized)
  {
      return *instance;
  } else {
      perror("fabric soundmanager::Recorder");
  }
}

void RecorderImpl::fabric(int _sampleRate, sf::Time _packetTime)
{
  if(!initialized){
    sampleRate = _sampleRate;
    packetTime = _packetTime;
    instance = new Recorder(sampleRate, packetTime);
    initialized = true;
  }
}

void Recorder::enableRec(){
  rec.start(sampleRate);
}

void Recorder::disableRec(){
  rec.stop();
}

void Recorder::setVolume(float volume){
  rec.setVolume(volume);
}
float Recorder::getVolume(){
  rec.getVolume();
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
  //rec.start(_sampleRate);
  sampleRate = _sampleRate;
}

}