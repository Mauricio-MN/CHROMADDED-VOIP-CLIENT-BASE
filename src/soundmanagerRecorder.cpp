#include "soundmanagerRecorder.h"

#include <SFML/Audio.hpp>

#include <algorithm>
#include <iterator>
#include <ostream>
#include <queue>
#include <future>

namespace soundmanager
{

    NetworkBufferRecorder::~NetworkBufferRecorder()
    {
        stop();
    }

    bool NetworkBufferRecorder::onStart()
    {
        m_samples.clear();
        m_buffer = sf::SoundBuffer();
        setProcessingIntervalOverride(timeBuffer);
        recording = true;

        return true;
    }

    bool NetworkBufferRecorder::onProcessSamples(const sf::Int16* samples, std::size_t sampleCount)
    {
        if(sampleCount >= sampleCountToProcess){
            sf::SoundBuffer buffer;
            buffer.loadFromSamples(samples, sampleCountToProcess, getChannelCount(), getSampleRate());
            NetworkBufferRecorder::asyncProcessSamples(buffer);
        }
        
        return true;
    }

    void NetworkBufferRecorder::asyncProcessSamples(sf::SoundBuffer buffer){
        if(listen == true){
            addBufferToQueue(&buffer);
        }
        int bufferByte_size = sizeof(const sf::Int16) * buffer.getSampleCount();
        data::buffer buff(buffer.getSamples(), buffer.getSampleCount());

        for (std::size_t i = 0; i < buffer.getSampleCount(); ++i) {
            reinterpret_cast<sf::Int16*>(buff.getData())[i] *= volume;
        }

        (*send)(buff, timeBuffer.asMilliseconds());
    }

    void NetworkBufferRecorder::addBufferToQueue(sf::SoundBuffer *buffer){
        queueMutex.lock();
        queueBuffer.push(*buffer);
        queueMutex.unlock();
    }

    void NetworkBufferRecorder::setVolume(float value){
        
    }
    float NetworkBufferRecorder::getVolume(){

    }

    sf::SoundBuffer NetworkBufferRecorder::getBufferFromQueue(){
        std::lock_guard<std::mutex> guard(queueMutex);
        sf::SoundBuffer buffer = queueBuffer.front();
        queueBuffer.pop();
        return buffer;
    }

    bool NetworkBufferRecorder::bufferQueueIsNotEmpty(){
        std::lock_guard<std::mutex> guard(queueMutex);
        if(queueBuffer.empty()){
            return false;
        }
        return true;
    }

    void NetworkBufferRecorder::cleanQueue(){
        std::lock_guard<std::mutex> guard(queueMutex);
        while (queueBuffer.empty() == false)
        {
            queueBuffer.pop();
        }
    }

    void NetworkBufferRecorder::setProcessingIntervalOverride(sf::Time time){
        setProcessingInterval(time);
        sampleCountToProcess = sampleTimeGetCount(time, getSampleRate());
        timeBuffer = time;
    }

    void NetworkBufferRecorder::setProcessingBufferFunction(void (*func)DEFAULT_BUFFER_ARGS ){
        send = func;
    }

    void NetworkBufferRecorder::setListen(bool isToListen){
        listen = isToListen;
    }

    void NetworkBufferRecorder::onStop()
    {
        recording = false;
    }

    void NetworkBufferRecorder::doNothingFunctionToBuffers DEFAULT_BUFFER_ARGS{
        return;
    }

}