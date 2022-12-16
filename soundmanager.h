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
#define SAMPLE_MS = 40
#define SAMPLE_CHANNELS 1

#define STRETCH_AUDIO_PERCENT 10
#define SPEED_AUDIO_PERCENT 10
#define SPEED_AUDIO_COUNT 5
#define DEBUG_AUDIO false

#define WAIT_NEW_PACK_COUNT 5

namespace soundmanager{


namespace listener{
    void movePos(float x, float y, float z);
    void moveRot(float x, float y, float z);
}

class Recorder{
public:

    void enableRec();

    void disableRec();

    sf::SoundBuffer recordForTest();

    Recorder();

private:
    sf::SoundCustomBufferRecorder rec;
};

class RecorderImpl
{
public:
    static Recorder &getInstance();
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
    void receive(data::buffer &data)
    {
        // Get waiting audio data
        std::scoped_lock lock(m_mutex);
        const sf::Int16* samples = reinterpret_cast<const sf::Int16*>(data.getData());
        m_swapSamples.insert(m_swapSamples.end(), samples, samples + (data.size() / sizeof(sf::Int16)) );

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
    bool alterPitch = false;
    bool canIgnoreMutex = false;

    int selectedSpeed = 0;
    bool onGetData(sf::SoundStream::Chunk& data) override
    {

        int wait = 0;
        int lastSize = m_swapSamples.size();

        if(m_swapSamples.size() <= 0){
            selectedSpeed = 0;
            return false;
        }

        if(selectedSpeed == 0){
            
            while (m_swapSamples.size() < SAMPLE_COUNT*3){
                wait++;
                if(wait > WAIT_NEW_PACK_COUNT){
                    if(m_swapSamples.size() == 0){
                        return false;
                    }
                    break;
                }
                if(m_swapSamples.size() == lastSize && wait > 2){
                    //slow audio
                } else {
                    lastSize = m_swapSamples.size();
                }
                sf::sleep(sf::milliseconds(40));
            }
            selectedSpeed = 3;
        }
        
        /*
        if((selectedSpeed == 1 && m_swapSamples.size() >= SAMPLE_COUNT) || m_swapSamples.size() >= SAMPLE_COUNT*SPEED_AUDIO_COUNT){
            selectedSpeed = 1;
            alterPitch = true;
            int sSampLastSize = m_swapSamples.size();

            m_tempBuffer.clear();
            
            swapSamples();

            float gainTime = 1.0F;
            float gainPitch = 1.0F;
            float calcPerc = 0;
            bool canContinue = true;

            if(sSampLastSize <= 0){
                selectedSpeed = 0;
                canContinue = false;
            } else if(sSampLastSize < SAMPLE_COUNT*4){
                calcPerc = (SAMPLE_COUNT * STRETCH_AUDIO_PERCENT / sSampLastSize) / 100.0F;
                gainPitch += calcPerc;
                gainTime -= calcPerc;
            } else if(sSampLastSize > SAMPLE_COUNT*SPEED_AUDIO_COUNT){
                calcPerc = (SAMPLE_COUNT * SPEED_AUDIO_PERCENT / sSampLastSize) / 100.0F;
                gainPitch -= calcPerc;
                gainTime += calcPerc;
            } else {
                selectedSpeed = 3;
                canContinue = false;
            }
            if(canContinue){

                setPitch(gainTime);

                float* insamples = new float[m_tempBuffer.size()];
                float* outsamples = new float[m_tempBuffer.size()];

                float copysample;
                for(int i = 0; i < (int)m_tempBuffer.size(); i++){
                    copysample = ((float)m_tempBuffer[i]) / (float)32768.0F;
                    if( copysample > 1.0F ) copysample = 1.0F;
                    if( copysample < -1.0F ) copysample = -1.0F;

                    insamples[i] = copysample;
                    outsamples[i] = copysample;
                }

                smbPitchShift(gainPitch, m_tempBuffer.size(), 128, 4, SAMPLE_RATE, insamples, outsamples);

                for(int i = 0; i < (int)m_tempBuffer.size(); i++){
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
        }
        */

        if(alterPitch){
            alterPitch = false;
            setPitch((float)1.0F);
        }

        m_tempBuffer.clear();

        if(canIgnoreMutex){
            std::swap(m_tempBuffer, m_swapSamples);
            canIgnoreMutex = false;
        } else {
            std::scoped_lock lock(m_mutex);
            std::swap(m_tempBuffer, m_swapSamples);
        }

        m_offset = 0;

         // Fill audio data to pass to the stream
        data.samples     = m_tempBuffer.data();
        data.sampleCount = m_tempBuffer.size();

        return false;
    }


    void tryIgnoreMutex(){
        if(m_mutex.try_lock()){
            canIgnoreMutex = true;
            while(canIgnoreMutex){
                sf::sleep(sf::milliseconds(1));
            }
            m_mutex.unlock();
        }
    }

    ////////////////////////////////////////////////////////////
    /// /see SoundStream::OnSeek
    ///
    ////////////////////////////////////////////////////////////
    void onSeek(sf::Time timeOffset) override
    {
        m_offset = static_cast<std::size_t>(timeOffset.asMilliseconds()) * getSampleRate() * getChannelCount() / 1000;
        if (!canIgnoreMutex) {
            if (m_tempBuffer.size() - m_offset <= SAMPLE_COUNT * 2) {
                if (m_swapSamples.size() >= SAMPLE_COUNT * 2) {
                    std::thread(&NetworkAudioStream::tryIgnoreMutex, this).detach();
                }
            }
        }
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