#ifndef soundmanager_H // include guard
#define soundmanager_H

#include <thread>
#include <cstring>
#include <cmath>
#include <iostream>
#include <iterator>
#include <mutex>
//#include <bits/stdc++.h>

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>

#include <AL/al.h>
#include <AL/efx.h>

//#include "smbPitchShift.h"

#include "soundmanagerRecorder.h"

#include "smbPitchShift.h"

//#include <soundtouch/SoundTouch.h>

//#include "lib/signalsmith/signalsmith-stretch.h"

#include "protoParse.h"

#define SAMPLE_RATE 16000
#define SAMPLE_BITS 16

#define SAMPLE_TIME_DEFAULT 20

#define SAMPLE_CHANNELS 1

#define STRETCH_AUDIO_PERCENT 10
#define SPEED_AUDIO_PERCENT 10
#define SPEED_AUDIO_COUNT 5
#define DEBUG_AUDIO false

#define WAIT_NEW_PACK_COUNT 5

namespace soundmanager{

  void load_EFX_functions();

namespace listener{
    void movePos(float x, float y, float z);
    void moveDir(float x, float y, float z);
    void setUpVector(float x, float y, float z);
}

class Recorder{
public:
    //Slow but improve CPU, need be called at init or can work in a boxed thread
    void enableRec();
    //Slow but improve CPU, need be called at init or can work in a boxed thread
    void disableRec();

    //Fast but need enableRec one time (to init record data)
    void enableSendData();
    //Fast but need enableRec one time and a "enableSendData" context
    void disableSendData();

    void setListenAudio(bool needListen);

    bool isTalking();

    sf::SoundBuffer recordForTest();

    Recorder();

    ~Recorder();

    Recorder(int _sampleRate, sf::Time _packetTime);

    void reConstruct(int _sampleRate, sf::Time _packetTime);

    int getSampleTime();
    int getSampleCount();

    void setVolume(float volume);
    float getVolume();

    void setNeedDetect(bool isNeed);
    void setDetectValue(float value);
    float getDetectValue();

    bool swapDevice(std::string device);

    std::vector<std::string> getDevices();

    //thread safe
    void setIntervalTime(int milliseconds);

    //max 4 seconds, mutex operation
    void setEcho(bool isOn, float decay, float delay);
    //max 6 seconds, mutex operation
    void setReverb(bool isOn, float decay, float delay);

private:
    int sampleRate;
    soundmanager::NetworkBufferRecorder rec;
    sf::Time packetTime;

    std::atomic<float> detectValue;
    std::atomic<bool> detectNeeded;
    std::atomic<bool> isRecording;
    std::atomic<bool> canSendData;

    std::atomic<bool> needApplyEcho;
    std::atomic<bool> needApplyReverb;
    std::atomic<float> echoDecay;
    std::atomic<float> reverbDecay;
    std::atomic<int> echoDelay;
    std::atomic<int> reverbDelay;

    void initialize(int _sampleRate, sf::Time _packetTime);
};

class RecorderImpl
{
    static bool initialized;
    static int sampleRate;
    static sf::Time packetTime;

    static Recorder* instance;
public:
    static Recorder &getInstance();
    static void fabric(int _sampleRate, sf::Time _packetTime);
    static void close();
};

class NetworkAudioStream : public sf::SoundStream
{
public:
    ////////////////////////////////////////////////////////////
    /// Default constructor
    ///
    ////////////////////////////////////////////////////////////
    NetworkAudioStream(sf::Time _sampleTime, int _sampleChannels, int _sampleRate, int _sampleBits);

    NetworkAudioStream();

    static void enableReverb(float density, float diffusion, float gain, float gainHf, float decayTime, float decayHfRatio);
    static void disableReverb();
    static void setInitAudioDelay(int delayMS);

    //thread safe
    void resizeSampleTime(sf::Time _sampleTime);

    /*
    //thread safe
    int getSampleCount(){
        return sampleCount;
    }

    //thread safe
    int getSampleTime(){
        return sampleTime_MS;
    }*/

    //thread safe
    void insert(int audioNumb, data::buffer& data, int sampleTime);

    void setDelaySamplesPlayData(int samples);

    void clean();

    //Thread safe
    bool isPlaying();

    //thread safe
    /*
    void insert(int audioNumb, std::vector<sf::Int16> data)
    {
        for(auto& frame : data){
            queueBuff.Enqueue(frame);
        }
    }*/

    void updateLossPercent();

private:
    static ALuint effect;
    static ALuint effectSlot;
    static std::atomic_bool effectIsOn;
    static std::mutex effectMutex;

    std::atomic_bool playing;
    std::atomic_int lastSampleTime;
    std::atomic_int lossPercent;
    std::atomic_int sleepGetDataDelaySamples;

    int maxSkyTerribleTime;

    std::vector<bool> crackleInfo;
    int cracklePos;
    int crackleTotal;
    
    std::vector<sf::Int16> interpolate(std::vector<sf::Int16>& a, std::vector<sf::Int16>& b);

    bool canInitDelay;
    int noFailedCount;
    static std::atomic_int32_t initDelayCountMS;
    bool onGetData(sf::SoundStream::Chunk& data) override;

    void onSeek(sf::Time timeOffset) override;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::vector<std::int16_t> m_samples;
    data::AudioQueue audioQueue;
    data::AudioQueue audioQueueFEC;
    data::OpusQueue opusQueue;
    std::size_t               m_offset;

    //signalsmith::stretch::SignalsmithStretch<float> stretch;

    //sf::Time sampleTime;
    //int sampleTime_MS;
    //int sampleCount;
    int sampleChannels;
    int sampleRate;
    int sampleBits;

    OpusManager opusMng;

    int readsTry;
    int samplesStable;

    data::buffer bufferAudio;

    sf::Clock clock;

    std::shared_mutex geralMutexLockData;
    std::mutex opusMutex;
    std::mutex advanceMutex;
};

class OVNetworkAudioStream {
    private:
    NetworkAudioStream* audioStream;
    public:
    OVNetworkAudioStream(){
        audioStream = new NetworkAudioStream();
    }
    OVNetworkAudioStream(sf::Time _sampleTime, int _sampleChannels, int _sampleRate, int _sampleBits){
        audioStream = new NetworkAudioStream(_sampleTime, _sampleChannels, _sampleRate, _sampleBits);
    }
    ~OVNetworkAudioStream(){
        if (audioStream != nullptr){
            delete audioStream;
        }
    }
    
    NetworkAudioStream* get()
        { return audioStream; }
};

}

#endif