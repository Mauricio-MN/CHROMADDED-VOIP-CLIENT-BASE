#ifndef SFMLOVERRIDES_H // include guard
#define SFMLOVERRIDES_H

#include <thread>

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "SoundCustomBufferRecorder.hpp"

using sf::SoundCustomBufferRecorder;

#define SAMPLE_RATE 16000
#define SAMPLE_BITS 16
#define SAMPLE_COUNT 800
#define SAMPLE_MS = 50
#define SAMPLE_CHANNELS 1

namespace recorder{

inline std::queue<sf::SoundBuffer> soundBufferQueue;
inline bool isFreeSoundBufferQueue = false;
inline bool recording = false;

void movePos(float x, float y, float z){
    sf::Listener::setPosition(x,y,z);
}

void moveRot(float x, float y, float z){
    sf::Listener::setDirection(x,y,z);
}

// custom audio stream that plays a loaded buffer
class MyStream : public sf::SoundStream
{
public:

    void load(const sf::SoundBuffer& buffer)
    {
        // extract the audio samples from the sound buffer to our own container
        m_samples.assign(buffer.getSamples(), buffer.getSamples() + buffer.getSampleCount());

        // reset the current playing position 
        m_currentSample = 0;

        // initialize the base class
        initialize(buffer.getChannelCount(), buffer.getSampleRate());
    }

    void insert(const sf::SoundBuffer& buffer){
      const sf::Int16 * samples = buffer.getSamples();
      m_samples.insert(m_samples.end(), samples, samples + buffer.getSampleCount());
    }

    void clear(){
      m_samples.clear();
    }

    size_t getSamplesSize(){
      return m_samples.size();
    }

private:

    virtual bool onGetData(Chunk& data)
    {
        // number of samples to stream every time the function is called;
        // in a more robust implementation, it should be a fixed
        // amount of time rather than an arbitrary number of samples
        const int samplesToStream = 400;

        // set the pointer to the next audio samples to be played
        data.samples = &m_samples[m_currentSample];

        // have we reached the end of the sound?
        if (m_currentSample + samplesToStream <= m_samples.size())
        {
            // end not reached: stream the samples and continue
            data.sampleCount = samplesToStream;
            m_currentSample += samplesToStream;

            if(m_samples.size() > 374080){
                m_samples.clear();
                m_currentSample = 0;
                //printf("prob %d \n", m_samples.size());
            }

            return true;
        }
        else
        {
            // end of stream reached: stream the remaining samples and stop playback
            data.sampleCount = m_samples.size() - m_currentSample;
            m_currentSample = m_samples.size();
            //return false;
            return false;
        }
    }

    virtual void onSeek(sf::Time timeOffset)
    {
        // compute the corresponding sample index according to the sample rate and channel count
        m_currentSample = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate() * getChannelCount());
    }

    std::vector<sf::Int16> m_samples;
    std::vector<sf::Int16> tempSamples;
    std::size_t m_currentSample;
};

static void record(SoundCustomBufferRecorder* recorder){
  recorder->setChannelCount(1);
  recorder->setProcessingIntervalOverride(sf::milliseconds(50));
  recorder->setListen(false);
  recorder->setProcessingBufferFunction(&connection::send);
  recorder->start(16000);

  while (true) {

    sf::sleep(sf::milliseconds(50));
    if(recording == true && !recorder->recording){
      recorder->start(16000);
    } else if(recording == false && recorder->recording) {
      recorder->stop();
    }

  }

}

static void startRecorder(){
  MyStream stream;

    //record thread
    SoundCustomBufferRecorder recorder;
    std::thread(record, &recorder).detach();

    printf("recording \n");

    while(recorder.bufferQueueIsNotEmpty() == false){
      sf::sleep(sf::milliseconds(1));
    }

    stream.load(recorder.getBufferFromQueue());

    printf("Playng \n");

    while(true){
      sf::sleep(sf::milliseconds(25));
      if(recorder.bufferQueueIsNotEmpty()){
          stream.insert(recorder.getBufferFromQueue());
      }
      sf::sleep(sf::milliseconds(25));
      if(recorder.bufferQueueIsNotEmpty()){
          stream.insert(recorder.getBufferFromQueue());
      }

      if(stream.getStatus() != MyStream::Playing){
        stream.play();
        printf("size %d \n", stream.getSamplesSize());
      }
    }
}

}

#endif