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

#include "smbPitchShift.h"
#include "structure/CircularBuffer.h"
#include "structure/ringbuffer.hpp"
//#include <boost/circular_buffer.hpp>

#define SAMPLE_RATE 16000
#define SAMPLE_BITS 16

#define SAMPLE_TIME_DEFAULT 40

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

    Recorder(int _sampleRate, sf::Time packetTime);

    void reConstruct(int _sampleRate, sf::Time packetTime);

private:
    int sampleRate;
    sf::SoundCustomBufferRecorder rec;

    void initialize(int _sampleRate, sf::Time packetTime);
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
    NetworkAudioStream(sf::Time _sampleTime, int _sampleChannels, int _sampleRate, int _sampleBits) : m_offset(0), m_updateOffset(false), m_circular_buffer(_sampleRate * 10), m_circular_temp(_sampleRate * 10), m_hyper_buffer(_sampleRate * 10)
    {
        // Set the sound parameters
        sampleCount = sampleTimeGetCount(_sampleTime, _sampleRate);
        sampleChannels = _sampleChannels;
        sampleRate = _sampleRate;
        sampleBits = _sampleBits;

        initialize(sampleChannels, sampleRate);
    }

    ////////////////////////////////////////////////////////////
    /// Get audio data from the client until playback is stopped
    ///
    ////////////////////////////////////////////////////////////
    void receive(data::buffer &data)
    {
        // Get waiting audio data
        //std::scoped_lock lock(m_mutex);
        const sf::Int16* samples = reinterpret_cast<const sf::Int16*>(data.getData());
        int samplesCount = (data.size() / sizeof(sf::Int16));
        //std::vector<sf::Int16> vect(samples, samples + data.size());
        //**m_circular_buffer.write(samples, (data.size() / sizeof(sf::Int16)) );
        //**m_hyper_buffer.write(samples, (data.size() / sizeof(sf::Int16)));
        auto avaliable = m_r_buffer.writeAvailable();

        auto writed = 0;

        if(avaliable > 0){
            if(avaliable >= samplesCount){
                writed = m_r_buffer.writeBuff(samples, samplesCount);
            } else {
                writed = m_r_buffer.writeBuff(samples, avaliable);
            }
        }
        if(writed == samplesCount) return;

        int pos = writed;
        int toWrite = samplesCount - writed;
        int tryng = 0;
        auto avaliableNow = m_r_buffer.writeAvailable();

        while(toWrite > 0){

            if(avaliableNow > 0 && avaliableNow < samplesCount){
                writed = m_r_buffer.writeBuff(samples + pos, m_r_buffer.writeAvailable());
                pos += writed;
                toWrite -= writed;
            }

            if(tryng >= SAMPLE_RATE){
                break;
            }
            tryng++;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            avaliableNow = m_r_buffer.writeAvailable();
        }
        //last try
        if(m_r_buffer.writeAvailable() >= toWrite){
            m_r_buffer.writeBuff(samples + pos, toWrite);
        }

        //m_swapSamples.insert(m_swapSamples.end(), samples, samples + (data.size() / sizeof(sf::Int16)) );
        //m_circular_buffer.Push(vect);
        //m_circular_buffer.Push(samples, (data.size() / sizeof(sf::Int16)));
    }

    void insert(sf::SoundBuffer data)
    {
        // Get waiting audio data
        //std::scoped_lock lock(m_mutex);
        const sf::Int16* samples = data.getSamples();
        //std::vector<sf::Int16> vect(samples, samples + (data.getSampleCount() / sizeof(sf::Int16)));
        //**m_circular_buffer.write(samples, data.getSampleCount());
        //**m_hyper_buffer.write((sf::Int16*)samples, data.getSampleCount());

        data::buffer buffer((const char*)samples, data.getSampleCount() * sizeof(sf::Int16));
        receive(buffer);
        //m_circular_buffer.Push(samples, data.getSampleCount());
        //m_swapSamples.insert(m_swapSamples.end(), samples, samples + data.getSampleCount());

    }

private:

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
        m_offset = 0;

        auto avaliable = m_r_buffer.readAvailable();
        if(avaliable > 0){
            auto readed = m_r_buffer.readBuff(m_circular_temp.data(), sampleCount);
            data.samples = m_circular_temp.data();
            data.sampleCount = readed;
            return true;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(40 * 5));
            if(m_r_buffer.readAvailable() <= 0){
                std::this_thread::sleep_for(std::chrono::milliseconds(40 * 5));
                if(m_r_buffer.readAvailable() <= 0){
                    return false;
                }
            }
        }
        return false;
        /*
        size_t readSize = m_hyper_buffer.read(m_circular_temp.data(), m_circular_temp.size());
        if(readSize > 0){
            data.samples = m_circular_temp.data();
            data.sampleCount = readSize;
            return true;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(40 * 5));
            return false;
        }

        int trynum = 5;
        while(trynum > 0){
            bool tryread = m_circular_buffer.read(m_circular_temp.data(), trynum);
            if(tryread){
                data.samples = m_circular_temp.data();
                data.sampleCount = trynum;
                return true;
            }
            trynum--;
        }

        return false;*/

        /*
        for(auto item : temp){
            m_circular_temp.insert(m_circular_temp.end(),item.begin(), item.end());
        }

        data.samples = m_circular_temp.data();
            data.sampleCount = m_circular_temp.size();
        */

        /*if(!buffer.empty()){
            int count = 0;
            while(m_circular_buffer.Size() < SAMPLE_COUNT_40MS * 5){
                sf::sleep(sf::milliseconds(40 * 5));
                count++;
                if(count >= 10){
                    return false;
                }
            }
        }*/

        //m_circular_temp.insert(m_circular_temp.begin(), buffer.begin(), buffer.end());

        /*
        if(!m_circular_temp.empty()){
            data.samples = m_circular_temp.data();
            data.sampleCount = m_circular_temp.size();
            return true;
        } else {
            int count = 0;
            while(m_circular_buffer.Size() < SAMPLE_COUNT_40MS * 5){
                sf::sleep(sf::milliseconds(40 * 5));
                count++;
                if(count >= 10){
                    return false;
                }
            }
        }

        return false;

        int wait = 0;
        int lastSize = m_swapSamples.size();

        if(m_swapSamples.size() <= 0){
            selectedSpeed = 0;
            return false;
        }

        if(selectedSpeed == 0){
            
            while (m_swapSamples.size() < SAMPLE_COUNT_40MS*3){
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
        */
        
        /*
        if((selectedSpeed == 1 && m_swapSamples.size() >= SAMPLE_COUNT_40MS) || m_swapSamples.size() >= SAMPLE_COUNT_40MS*SPEED_AUDIO_COUNT){
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
            } else if(sSampLastSize < SAMPLE_COUNT_40MS*4){
                calcPerc = (SAMPLE_COUNT_40MS * STRETCH_AUDIO_PERCENT / sSampLastSize) / 100.0F;
                gainPitch += calcPerc;
                gainTime -= calcPerc;
            } else if(sSampLastSize > SAMPLE_COUNT_40MS*SPEED_AUDIO_COUNT){
                calcPerc = (SAMPLE_COUNT_40MS * SPEED_AUDIO_PERCENT / sSampLastSize) / 100.0F;
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
        //m_offset = static_cast<std::size_t>(timeOffset.asMilliseconds()) * getSampleRate() * getChannelCount() / 1000;
        m_offset = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate() * getChannelCount());
        /*if(m_circular_temp.size() < 3){
            if(m_circular_buffer.Size() > 2){
                auto temp = m_circular_buffer.Pop(3);
                for(auto item : temp){
                    m_circular_temp.insert(m_circular_temp.end(),item.begin(), item.end());
                }
            }
        }*/
        /*if (!canIgnoreMutex) {
            if (m_tempBuffer.size() - m_offset <= SAMPLE_COUNT_40MS * 2) {
                if (m_swapSamples.size() >= SAMPLE_COUNT_40MS * 2) {
                    std::thread(&NetworkAudioStream::tryIgnoreMutex, this).detach();
                }
            }
        }*/
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
    //boost::circular_buffer<std::vector<std::int16_t>> m_circular_buffer;
    ConcurrentCircularBuffer<sf::Int16> m_circular_buffer;
    hyperBuffer<sf::Int16> m_hyper_buffer;
    jnk0le::Ringbuffer<sf::Int16, 131072> m_r_buffer;
    std::vector<std::int16_t> m_circular_temp;

    int sampleCount;
    int sampleChannels;
    int sampleRate;
    int sampleBits;
};


}

#endif