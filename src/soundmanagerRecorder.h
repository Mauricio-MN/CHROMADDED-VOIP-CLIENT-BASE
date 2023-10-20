#ifndef NetworkBufferRecorder_H
#define NetworkBufferRecorder_H

#include <SFML/Audio/SoundRecorder.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

#include "data.h"

inline sf::Time sampleCountGetTime(int sampleCount, int sampleRate = 16000){
    int mill = (int)(sampleCount) * 1000 / sampleRate;
    return sf::milliseconds(mill);
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
    class NetworkBufferRecorder : public sf::SoundRecorder
    {
    public:
        mutable std::mutex queueMutex;
        mutable bool recording = false;

        sf::SoundBuffer getBufferFromQueue();
        bool bufferQueueIsNotEmpty();
        void cleanQueue();

        void setProcessingIntervalOverride(sf::Time time);
        void setProcessingBufferFunction(void (*func)DEFAULT_BUFFER_ARGS );
        void setListen(bool isToListen);
        void addBufferToQueue(sf::SoundBuffer *buffer);

        void setVolume(float value);
        float getVolume();

        ~NetworkBufferRecorder() override;
    protected:
        [[nodiscard]] bool onStart() override;
        [[nodiscard]] bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) override;
        void onStop() override;

    private:
        std::vector<sf::Int16> m_samples;
        sf::SoundBuffer        m_buffer;
        mutable std::queue<sf::SoundBuffer> queueBuffer;
        void asyncProcessSamples(sf::SoundBuffer buffer);
        static void doNothingFunctionToBuffers DEFAULT_BUFFER_ARGS;
        void (*send)DEFAULT_BUFFER_ARGS = &doNothingFunctionToBuffers;
        bool listen = false;
        sf::Time timeBuffer = sf::milliseconds(40);
        int sampleCountToProcess = 640;
        std::atomic<float> volume;
        bool firstStart = true;
    };

}

#endif