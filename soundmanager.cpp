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
  sf::sleep(sf::milliseconds(5000));
  Trec.stop();
  return Trec.getBuffer();
}

Recorder::Recorder(){
  rec.setChannelCount(SAMPLE_CHANNELS);
  rec.setProcessingIntervalOverride(sf::milliseconds(40));
  rec.setListen(DEBUG_AUDIO);
  rec.setProcessingBufferFunction(player::Self::sendAudio);
  rec.start(SAMPLE_RATE);
}

}