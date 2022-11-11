#ifndef soundmanager_H // include guard
#define soundmanager_H

#include <thread>
#include <cstring>
#include <iostream>
#include <iterator>
#include <mutex>
#include <bits/stdc++.h>

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "SoundCustomBufferRecorder.hpp"

#include "connection.h"
#include "protocol.h"
#include "smbPitchShift.h"

#define SAMPLE_RATE 16000
#define SAMPLE_BITS 16
#define SAMPLE_COUNT 640
#define SAMPLE_MS = 50
#define SAMPLE_CHANNELS 1

#define STRETCH_AUDIO_PERCENT 5
#define DEBUG_AUDIO false

namespace soundmanager{


namespace listener{
    void movePos(float x, float y, float z);
    void moveRot(float x, float y, float z);
}

class recorder{
public:

    static void recordMng(void (*func)DEFAULT_BUFFER_ARGS);

    static void init();

    static void enableRec();

    static void disableRec();

    static sf::SoundBuffer recordForTest();

private:
    static sf::SoundCustomBufferRecorder rec;
};

class NetworkAudioStream : public sf::SoundStream
{
public:
    ////////////////////////////////////////////////////////////
    /// Default constructor
    ///
    ////////////////////////////////////////////////////////////
    NetworkAudioStream() : m_offset(0), m_updateOffset(false)
    {
        // Set the sound parameters
        initialize(SAMPLE_CHANNELS, SAMPLE_RATE);
    }

    ////////////////////////////////////////////////////////////
    /// Get audio data from the client until playback is stopped
    ///
    ////////////////////////////////////////////////////////////
    void receive(data::buffer* data)
    {
        // Get waiting audio data
        std::scoped_lock lock(m_mutex);
        const sf::Int16* samples = reinterpret_cast<const sf::Int16*>(data->getBuffer());
        m_swapSamples.insert(m_swapSamples.end(), samples, samples + (data->size / sizeof(sf::Int16)) );

    }

    void insert(sf::SoundBuffer data)
    {
        // Get waiting audio data
        std::scoped_lock lock(m_mutex);
        const sf::Int16* samples = data.getSamples();
        m_swapSamples.insert(m_swapSamples.end(), samples, samples + data.getSampleCount());

    }

private:

    void swapSamples(){
        std::scoped_lock lock(m_mutex);
        m_tempBuffer.insert( m_tempBuffer.end(), m_swapSamples.data(), m_swapSamples.data() + SAMPLE_COUNT );
        m_swapSamples.erase( m_swapSamples.begin(), m_swapSamples.begin() + SAMPLE_COUNT );
    }
    ////////////////////////////////////////////////////////////
    /// /see SoundStream::OnGetData
    ///
    ////////////////////////////////////////////////////////////
    int failCount = 0;
    bool onGetData(sf::SoundStream::Chunk& data) override
    {
        

        if((m_swapSamples.size() < SAMPLE_COUNT*2) && m_swapSamples.size() >= SAMPLE_COUNT){
            m_tempBuffer.clear();
            
            swapSamples();

            float gainTime = 1.0F;
            float gainPitch = STRETCH_AUDIO_PERCENT / 100.0F;
            failCount++;
            if(failCount > 1){
                gainPitch += failCount / 200.0F;
                if(failCount > 10) failCount = 10;
            }
            gainTime -= gainPitch;

            if(getPitch() > gainTime + 0.01F || getPitch() < gainTime - 0.01F){
                //SFML pitch change stretches the time, later the pitch is re-altered without time change.
                setPitch(gainTime);
            }

            float* insamples = new float[m_tempBuffer.size()];
            float* outsamples = new float[m_tempBuffer.size()];

            float copysample;
            for(int i = 0; i < m_tempBuffer.size(); i++){
                copysample = ((float)m_tempBuffer[i]) / (float)32768.0F;
                if( copysample > 1.0F ) copysample = 1.0F;
                if( copysample < -1.0F ) copysample = -1.0F;

                insamples[i] = copysample;
                outsamples[i] = copysample;
            }

            smbPitchShift(1.0F + gainPitch, m_tempBuffer.size(), 128, 4, SAMPLE_RATE, insamples, outsamples);

            for(int i = 0; i < m_tempBuffer.size(); i++){
                outsamples[i] = (outsamples[i] * 32768.0F);
                if( outsamples[i] > 32767.0F ) outsamples[i] = 32767.0F;
                if( outsamples[i] < -32768.0F ) outsamples[i] = -32768.0F;
                m_tempBuffer[i] = (sf::Int16) outsamples[i];
            }

            delete []outsamples;
            delete []insamples;
            m_offset = 0;

            // Fill audio data to pass to the stream
            data.samples     = m_tempBuffer.data();
            data.sampleCount = m_tempBuffer.size();

            return true;
        }

        if(getPitch() < 0.99F){
            setPitch(1.0F);
        }

        failCount = 0;

        m_tempBuffer.clear();

        while (m_swapSamples.size() < SAMPLE_COUNT*3){
            sf::sleep(sf::milliseconds(40));
        }

        {
            std::scoped_lock lock(m_mutex);
            std::swap(m_tempBuffer, m_swapSamples);
        }

        m_offset = 0;

         // Fill audio data to pass to the stream
        data.samples     = m_tempBuffer.data();
        data.sampleCount = m_tempBuffer.size();

        return true;
    }

    ////////////////////////////////////////////////////////////
    /// /see SoundStream::OnSeek
    ///
    ////////////////////////////////////////////////////////////
    void onSeek(sf::Time timeOffset) override
    {
        m_offset = static_cast<std::size_t>(timeOffset.asMilliseconds()) * getSampleRate() * getChannelCount() / 1000;
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::recursive_mutex      m_mutex;
    std::vector<std::int16_t> m_samples;
    std::vector<std::int16_t> m_swapSamples;
    std::vector<std::int16_t> m_tempBuffer;
    std::size_t               m_offset;
    bool                      m_updateOffset;
    bool                      m_updateOffsetRCV;
};


}

#endif