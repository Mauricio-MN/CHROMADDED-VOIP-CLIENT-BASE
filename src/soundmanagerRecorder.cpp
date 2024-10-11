#include "soundmanagerRecorder.h"

#include <SFML/Audio.hpp>

#include <algorithm>
#include <iterator>
#include <ostream>
#include <queue>
#include <future>
#include <cmath>

#include "soundmanager.h"

#include "opusmanager.h"

#include "replayGain.h"

namespace soundmanager
{

    NetworkBufferRecorder::~NetworkBufferRecorder()
    {
        stop();
        delete opusManager;
    }

    bool NetworkBufferRecorder::onStart()
    {
        if(firstStart){
            if(!listen.load() && debugSound == nullptr){
                listen = false;
                //timeBuffer = 40;
                //atomicSampleCountToProcess = 640;
            }
            opusManager = new OpusManager(getSampleRate());
            volume = 1.0F;
            detectValue = 0.2F;
            detectNeeded = false;
            audioNumber = 0;
            firstStart = false;
            canSendData = false;
            medianVolume = 0;
            actualDetectValue = 0.0f;
            talking = false;
            stopWait = 0;
            firstDownVolumeVoiceDetect = false;

            needApplyEcho = false;
            needApplyReverb = false;
            echoDecay = 0.5f;     // Decaimento do Echo
            reverbDecay = 0.3f;   // Decaimento do Reverb
            echoDelay = 500;        // Atraso do Echo em samples
            reverbDelay = 1000;     // Atraso do Reverb em samples

            echoBuffer.resize(getSampleRate() * 4);
            reverbBuffer.resize(getSampleRate() * 6);
        }
        
        m_samples.clear();
        m_buffer = sf::SoundBuffer();
        recording = true;

        //canRun = true;
        //isRuning = false;
        //asyncProcess = std::thread(&NetworkBufferRecorder::asyncProcessSamples, this);

        return true;
    }

    void NetworkBufferRecorder::onStop()
    {
        /*
        canRun = false;
        asyncProcess.join();
        while(isRuning){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }*/
        recording = false;
        talking = false;
    }

    void sumNumAudio(int& value){
        value++;
        if(value >= 256){
            value = 0;
        }
    }

    void NetworkBufferRecorder::setNeedSendData(bool needSend){
        canSendData = needSend;
    }

    void NetworkBufferRecorder::setNeedDetect(bool isNeed){
        detectNeeded = isNeed;
        actualDetectValue = 0.0f;
    }
    void NetworkBufferRecorder::setDetectValue(float value){
        detectValue = value;
    }
    float NetworkBufferRecorder::getDetectValue(){
        return detectValue.load();
    }

    bool NetworkBufferRecorder::isTalking(){
        return talking.load();
    }

    void audioPlusVolume(std::vector<sf::Int16>& samples, double factor) {
        for (sf::Int16& sample : samples) {
            // Aplicar o aumento de volume, garantindo que os valores permaneçam na faixa correta
            sample = static_cast<sf::Int16>(std::max(std::min(32767.0, static_cast<double>(sample) * factor), -32768.0));
        }
    }

    float procMicVolume(const sf::Int16* samples, std::size_t sampleCount, sf::Int16& lastVolume){
        std::int32_t sum = 0;
        for(int i = 0; i < sampleCount; i++){
            sum += samples[i];
        }
        std::int32_t median = sum / sampleCount;
        if(lastVolume > 10) median = (median + lastVolume) / 2;
        lastVolume = median;
        float result = median / INT16_MAX;
        return result;
    }

    float procMicVolumeAbs(const sf::Int16* samples, std::size_t sampleCount, sf::Int16& lastVolume, float detectValue){
        std::int32_t sum = 0;
        for(int i = 0; i < sampleCount; i++){
            sum += samples[i];
        }
        std::int32_t median = sum / sampleCount;
        float result = median / INT16_MAX;

        if (result < detectValue){
            result = lastVolume / INT16_MAX;
        }

        lastVolume = median;
        return result;
    }

    double dB_to_linear(double dB) {
        return std::pow(10, dB / 10);
    }

    // Constantes para os limites em dB
    const double MIN_DB = 0.0; // Nível mínimo em dB
    const double MAX_DB = 90.0;   // Nível máximo em dB

    // Função para normalizar o valor em dB para um float entre 0.0 e 1.0
    double normalizeDB(double dbValue) {
        if (dbValue < MIN_DB) dbValue = MIN_DB;
        if (dbValue > MAX_DB) dbValue = MAX_DB;
        double result = (dbValue - MIN_DB) / (MAX_DB - MIN_DB);
        return result;
    }

    // Function to calculate energy
    double calculate_energy(const sf::Int16* samples, size_t size) {
        double energy = 0.0;
        for (int i = 0; i < size; i++) {
            energy += samples[i] * samples[i]; // Sum of squares
        }
        return energy / size; // Normalize by number of samples
    }

    // Simple VAD function
    bool voice_activity_detection(const sf::Int16* samples, size_t size, double threshold) {
        double energy = calculate_energy(samples, size);
        return energy > threshold; // If energy exceeds threshold, return true (voice detected)
    }

    //max 4 seconds
    void NetworkBufferRecorder::setEcho(bool isOn, float decay, float delay){
        while(!echoMutex.try_lock()){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        needApplyEcho = isOn;
        echoDecay = decay;
        echoDelay = delay;
        echoMutex.unlock();
    }
    //max 6 seconds
    void NetworkBufferRecorder::setReverb(bool isOn, float decay, float delay){
        while(!reverbMutex.try_lock()){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        needApplyReverb = isOn;
        reverbDecay = decay;
        reverbDelay = delay;
        reverbMutex.unlock();
    }

    // Função para aplicar o Echo
    void NetworkBufferRecorder::applyEcho(sf::Int16* samples, std::size_t sampleCount) {
        echoMutex.lock();
        if(!needApplyEcho){
            echoMutex.unlock();
            return;
        }
        for (std::size_t i = 0; i < sampleCount; ++i) {
            int delayIndex = (i + echoBuffer.size() - echoDelay) % echoBuffer.size();
            sf::Int16 delayedSample = echoBuffer[delayIndex];
            samples[i] = std::clamp((int)samples[i] + (int)(echoDecay * delayedSample), -32768, 32767);
            echoBuffer[i % echoBuffer.size()] = samples[i]; // Atualiza o buffer de Echo
        }
        echoMutex.unlock();
    }

    // Função para aplicar o Reverb
    void NetworkBufferRecorder::applyReverb(sf::Int16* samples, std::size_t sampleCount) {
        reverbMutex.lock();
        if(!needApplyReverb){
            reverbMutex.unlock();
            return;
        }
        for (std::size_t i = 0; i < sampleCount; ++i) {
            int delayIndex = (i + reverbBuffer.size() - reverbDelay) % reverbBuffer.size();
            sf::Int16 delayedSample = reverbBuffer[delayIndex];
            samples[i] = std::clamp((int)samples[i] + (int)(reverbDecay * delayedSample), -32768, 32767);
            reverbBuffer[i % reverbBuffer.size()] = samples[i]; // Atualiza o buffer de Reverb
        }
        reverbMutex.unlock();
    }

    bool NetworkBufferRecorder::onProcessSamples(const sf::Int16* samples, std::size_t sampleCount)
    {
        if(detectNeeded.load()){
            //auto volume = procMicVolume(samples, sampleCount, medianVolume);
            //auto volume = procMicVolumeAbs(samples, sampleCount, medianVolume, detectValue);
            std::vector<int16_t> sampleVct;
            sampleVct.insert(sampleVct.begin(), samples, samples + sampleCount);
            auto volume = normalizeDB(calculateLoudness(sampleVct, getSampleRate()));
            actualDetectValue = volume;
            //if(volume < detectValue){
            if(voice_activity_detection(samples, sampleCount, 10000.0f * detectValue)){
                if(firstDownVolumeVoiceDetect == false){
                    firstDownVolumeVoiceDetect = true;
                    clockVoiceDetect.restart();
                }
                if(clockVoiceDetect.getElapsedTime().asMilliseconds() >= 120){
                    samplesBuff.clear();
                    talking = false;
                    firstDownVolumeVoiceDetect = false;
                    return true;
                }
            }
        } else {
            if(!canSendData.load()){
                samplesBuff.clear();
                talking = false;
                return true;
            }
        }

        talking = true;

        //sf::Clock clock;
        //clock.restart();

        //std::cout << "AudioNumb: " << sampleCount << std::endl;
        int sampleCountToProcess = atomicSampleCountToProcess.load();

        if(sampleCount + samplesBuff.size() < sampleCountToProcess){

            samplesBuff.insert(samplesBuff.end(), samples, samples + sampleCount);
            //std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
            return true;
        }

        std::vector<sf::Int16> audio;
        audio.reserve(sampleCount + samplesBuff.size());

        if(!samplesBuff.empty()){
            audio.insert(audio.begin(), samplesBuff.data(), samplesBuff.data() + samplesBuff.size());
            samplesBuff.clear();
        }
        audio.insert(audio.end(), samples, samples + sampleCount);

        int totalSize = floor( (float)audio.size() / (float)sampleCountToProcess);
        int processedSamples = (sampleCountToProcess * totalSize);
        int resumeSize = audio.size() - processedSamples;
        if(resumeSize > 0){
            samplesBuff.insert(samplesBuff.begin(), audio.begin() + processedSamples, audio.end());
        }
        
        int sampleTime = sampleCountGetTimeMS(sampleCountToProcess, getSampleRate());

        for(int i = 0; i < totalSize; i++){
            int offset = (i * sampleCountToProcess);

            applyEcho(audio.data() + offset, sampleCountToProcess);
            applyReverb(audio.data() + offset, sampleCountToProcess);

            for(int j = offset; j < offset + sampleCountToProcess; j++){
                audio[j] = (sf::Int16)((float)(audio[j]) * (float)getVolume());
            }

            data::buffer encoded = opusManager->encode(audio.data() + offset, sampleCountToProcess);
            (*send)(encoded, sampleTime);

            if(listen.load()){
                audioNumber++;
                if(audioNumber >= 256) audioNumber = 0;
                debugSound->insert(audioNumber.load(), encoded, sampleTime);
                //std::cout << "debugNumb: " << audioNumber.load() << std::endl;
            }
        }
        
        //std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
        return true;
    }

    /*
    void NetworkBufferRecorder::asyncProcessSamples(){
        isRuning = true;
        std::vector<sf::Int16> samples;
        int baseReserve = sampleCountToProcess * 3;
        data::buffer samplesBuff(baseReserve);
        data::buffer audioDebug(sampleCountToProcess);
        int samplesBuff_TrueSize = 0;

        data::AudioQueue localAudioQueue;
        int numb = 0;

        int numbDebugSound = 0;
        while(true){
            std::vector<sf::Int16> samplesToProcess;
            
            std::vector<sf::Int16> audio;

            bool pulled = queueBuffer.pop(audio);
            if(pulled){
                if(samplesBuff_TrueSize > 0){
                    audio.insert(audio.begin(), samplesBuff.getData(), samplesBuff.getData() + samplesBuff_TrueSize);
                    samplesBuff_TrueSize = 0;
                }
                if(audio.size() > sampleCountToProcess){
                    int totalSize = floor( (float)audio.size() / (float)sampleCountToProcess);
                    for(int i = 0; i < totalSize; i++){
                        int offset = (i * sampleCountToProcess);
                        audioDebug.writeover(0, audio.data() + offset, sampleCountToProcess);
                        debugSound.insert(numb, audioDebug);
                        //localAudioQueue.push(numb, lAudio);
                        sumNumAudio(numb);
                    }
                    int processedSamples = (sampleCountToProcess * totalSize);
                    int resumeSize = audio.size() - processedSamples;
                    if(resumeSize > 0){
                        samplesBuff.writeover(0, audio.data() + processedSamples, resumeSize);
                        samplesBuff_TrueSize = resumeSize;
                    }
                }
            } else {
                double sleepTime = ((double)timeBuffer.asMilliseconds() - 5) / 1000.0;
                data::preciseSleep(sleepTime);
                continue;
            }*/

            /*
            bool pulledLAQ = true;
            while(pulledLAQ){
                std::vector<sf::Int16> lAudio;
                pulledLAQ = localAudioQueue.pop(lAudio);
                if(pulledLAQ){


                }
            }*/

            /*
            if(samplesBuff_TrueSize >= sampleCountToProcess){
                samplesToProcess.insert(samplesToProcess.end(), samples.begin(), samples.begin() + sampleCountToProcess);
                samples.erase(samples.begin(), samples.begin() + sampleCountToProcess);
            } else {
                double sleepTime = ((double)timeBuffer.asMilliseconds() - 5) / 1000.0;
                data::preciseSleep(sleepTime);
                continue;
            }

            if(listen == true){
                //addBufferToQueue(&buffer);
            }

            for(auto& sample : samplesToProcess){
                sample *= volume;
            }

            data::buffer encoded = opusManager->encode(samplesToProcess.data(), samplesToProcess.size());

            (*send)(encoded, timeBuffer.asMilliseconds());
            */

    /*
            if(canRun == false){
                break;
            }
        }
        isRuning = false;
    }*/

    void NetworkBufferRecorder::setVolume(float value){
        volume = value;
    }
    float NetworkBufferRecorder::getVolume(){
        return volume.load();
    }

    //unsafe
    void NetworkBufferRecorder::setProcessingIntervalOverride(sf::Time time, int sampleRate){
        sf::Time newTime = sf::milliseconds((time.asMilliseconds() / 2));
        int sampleCountToProcess = atomicSampleCountToProcess.load();
        int newSampleCountToProcess = sampleTimeGetCount(time, sampleRate);
        if(sampleCountToProcess != newSampleCountToProcess){
            if(!recording.load()){
                setProcessingInterval(newTime);
                atomicSampleCountToProcess = newSampleCountToProcess;
            } else {
                atomicSampleCountToProcess = newSampleCountToProcess;
            }
        }
        //timeBuffer = time.asMilliseconds();
    }

    void NetworkBufferRecorder::setProcessingBufferFunction(void (*func)DEFAULT_BUFFER_ARGS ){
        send = func;
    }

    void NetworkBufferRecorder::setListen(bool isToListen){
        if(!isToListen){
            if(listen.load()){
                listen = false;
                bool startAgain = false;
                if(recording){
                    stop();
                    startAgain = true;
                }
                debugSound->stop();
                delete debugSound;
                if(startAgain){
                    start();
                }
            }
        } else {
            if(!listen.load()){
                debugSound = new NetworkAudioStream(sf::milliseconds(20), SAMPLE_CHANNELS, SAMPLE_RATE, SAMPLE_BITS);
                listen = true;
            }
        }
    }

    void NetworkBufferRecorder::doNothingFunctionToBuffers DEFAULT_BUFFER_ARGS{
        return;
    }

}