#ifndef NetworkBufferRecorder_H
#define NetworkBufferRecorder_H

#include <SFML/Audio/SoundRecorder.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System.hpp>

#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

#include "data.h"
#include "opusmanager.h"

inline sf::Time sampleCountGetTime(int sampleCount, int sampleRate = 16000){
    int mill = (int)(sampleCount) * 1000 / sampleRate;
    return sf::milliseconds(mill);
}

inline int sampleCountGetTimeMS(int sampleCount, int sampleRate = 16000){
    int mill = (int)(sampleCount) * 1000 / sampleRate;
    return mill;
}

inline int sampleTimeGetCount(sf::Time time, int sampleRate = 16000){
    int sampleCount = time.asMilliseconds() * sampleRate / 1000;
    return sampleCount;
}

inline int sampleTimeGetCount(int millsec, int sampleRate = 16000){
    int sampleCount = millsec * sampleRate / 1000;
    return sampleCount;
}

enum audioPacketMillisecond{
    MS10 = 10, MS20 = 20, MS40 = 40
};

#define DEFAULT_BUFFER_ARGS (data::buffer &buffer, int sampleTime)
namespace soundmanager
{
    class NetworkAudioStream;

    //Not thread safe
    class NetworkBufferRecorder : public sf::SoundRecorder
    {
    public:

        void setProcessingIntervalOverride(sf::Time time, int sampleRate);
        void setProcessingBufferFunction(void (*func)DEFAULT_BUFFER_ARGS );
        void setListen(bool isToListen);

        void setVolume(float value);
        float getVolume();

        void setNeedSendData(bool needSend);

        void setNeedDetect(bool isNeed);
        void setDetectValue(float value);
        float getDetectValue();

        bool isTalking();

        //max 4 seconds, mutex operation
        void setEcho(bool isOn, float decay, float delay);
        //max 6 seconds, mutex operation
        void setReverb(bool isOn, float decay, float delay);

        ~NetworkBufferRecorder() override;
    protected:
        [[nodiscard]] bool onStart() override;
        [[nodiscard]] bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) override;
        void onStop() override;

    private:
        sf::Clock clockVoiceDetect;
        bool firstDownVolumeVoiceDetect;

        std::vector<sf::Int16> m_samples;
        std::atomic<float> detectValue;
        std::atomic<bool> detectNeeded;
        std::atomic<bool> canSendData;

        std::atomic<float> actualDetectValue;

        std::atomic_bool recording;
        std::atomic_bool talking;

        std::atomic_int stopWait;

        //std::atomic_bool canRun;
        //std::atomic_bool isRuning;
        //std::thread asyncProcess;

        sf::SoundBuffer        m_buffer;
        std::atomic_uint32_t audioNumber;
        mutable data::AudioQueue queueBuffer;
        std::vector<sf::Int16> samplesBuff;

        sf::Int16 medianVolume;

        //void asyncProcessSamples();
        static void doNothingFunctionToBuffers DEFAULT_BUFFER_ARGS;
        void (*send)DEFAULT_BUFFER_ARGS = &doNothingFunctionToBuffers;
        //std::atomic_int32_t timeBuffer;
        std::atomic_int32_t atomicSampleCountToProcess;
        std::atomic<float> volume;
        bool firstStart = true;
        OpusManager* opusManager;
        std::atomic_bool listen;
        NetworkAudioStream* debugSound = nullptr;

        bool needApplyEcho;
        bool needApplyReverb;
        std::mutex echoMutex;
        std::mutex reverbMutex;
        std::vector<sf::Int16> echoBuffer;
        std::vector<sf::Int16> reverbBuffer;
        float echoDecay;
        float reverbDecay;
        int echoDelay;
        int reverbDelay;
        void applyEcho(sf::Int16* samples, std::size_t sampleCount);
        void applyReverb(sf::Int16* samples, std::size_t sampleCount);
    };

}

#endif