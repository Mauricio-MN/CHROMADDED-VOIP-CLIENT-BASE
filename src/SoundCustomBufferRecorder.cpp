////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2022 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SoundCustomBufferRecorder.hpp"
#include <SFML/System/Err.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>

#include <algorithm>
#include <iterator>
#include <ostream>
#include <queue>
#include <future>


namespace sf
{
////////////////////////////////////////////////////////////
SoundCustomBufferRecorder::~SoundCustomBufferRecorder()
{
    // Make sure to stop the recording thread
    stop();
}


////////////////////////////////////////////////////////////
bool SoundCustomBufferRecorder::onStart()
{
    m_samples.clear();
    m_buffer = SoundBuffer();

    recording = true;

    return true;
}


////////////////////////////////////////////////////////////
bool SoundCustomBufferRecorder::onProcessSamples(const Int16* samples, std::size_t sampleCount)
{
    if(processSound){
        if(sampleCount >= sampleCountToProcess){
            //std::copy(samples, samples + sampleCount, std::back_inserter(m_samples));
            sf::SoundBuffer buffer;
            buffer.loadFromSamples(samples, sampleCountToProcess, getChannelCount(), getSampleRate());
            //std::thread(SoundCustomBufferRecorder::addBufferToQueue, std::ref(buffer)).detach();

            //std::async(&SoundCustomBufferRecorder::asyncProcessSamples, this, buffer);
            std::thread(&SoundCustomBufferRecorder::asyncProcessSamples, this, buffer).detach();

        }
    }
    return true;
}

void SoundCustomBufferRecorder::asyncProcessSamples(sf::SoundBuffer buffer){
    if(listen == true){
        addBufferToQueue(&buffer);
    }
    int bufferByte_size = sizeof(const Int16) * buffer.getSampleCount();
    data::buffer buff;
    buff.insertArray((sf::Int16 *)buffer.getSamples(), buffer.getSampleCount());
    (*send)(buff);
}

void SoundCustomBufferRecorder::enableProcessSound(){
    processSound = true;
}

void SoundCustomBufferRecorder::disableProcessSound(){
    processSound = false;
}

void SoundCustomBufferRecorder::addBufferToQueue(sf::SoundBuffer *buffer){
    queueMutex.lock();
    queueBuffer.push(*buffer);
    queueMutex.unlock();
}

SoundBuffer SoundCustomBufferRecorder::getBufferFromQueue(){
    std::lock_guard<std::mutex> guard(queueMutex);
    SoundBuffer buffer = queueBuffer.front();
    queueBuffer.pop();
    return buffer;
}

bool SoundCustomBufferRecorder::bufferQueueIsNotEmpty(){
    std::lock_guard<std::mutex> guard(queueMutex);
    if(queueBuffer.empty()){
        return false;
    }
    return true;
}

void SoundCustomBufferRecorder::cleanQueue(){
    std::lock_guard<std::mutex> guard(queueMutex);
    while (queueBuffer.empty() == false)
    {
        queueBuffer.pop();
    }
}

void SoundCustomBufferRecorder::setProcessingIntervalOverride(sf::Time time){
    setProcessingInterval(time);
    sampleCountToProcess = sampleTimeGetCount(time, getSampleRate());
    timeBuffer = time;
}

void SoundCustomBufferRecorder::setProcessingBufferFunction(void (*func)DEFAULT_BUFFER_ARGS ){
    send = func;
}

void SoundCustomBufferRecorder::setListen(bool isToListen){
    listen = isToListen;
}


////////////////////////////////////////////////////////////
void SoundCustomBufferRecorder::onStop()
{
    recording = false;
    //if (m_samples.empty())
    //    return;

    //if (!m_buffer.loadFromSamples(m_samples.data(), m_samples.size(), getChannelCount(), getSampleRate()))
    //   err() << "Failed to stop capturing audio data" << std::endl;
}


////////////////////////////////////////////////////////////
const SoundBuffer& SoundCustomBufferRecorder::getData() const
{
    return m_buffer;
}

void SoundCustomBufferRecorder::doNothingFunctionToBuffers DEFAULT_BUFFER_ARGS{
    return;
}

} // namespace sf