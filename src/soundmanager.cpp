#include <thread>

#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "soundmanager.h"

#include "soundmanagerRecorder.h"

#include "player.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/efx-creative.h>
#include <AL/efx-presets.h>

namespace soundmanager{

    std::atomic_bool efxOn = {false};

    LPALGENEFFECTS alGenEffects;
    LPALDELETEEFFECTS alDeleteEffects;
    LPALEFFECTI alEffecti;
    LPALEFFECTF alEffectf;
    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
    LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;

    void load_EFX_functions() {
        
        alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
        alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
        alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
        alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
        alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
        alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
        alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");

        if (!alGenEffects || !alDeleteEffects || !alEffecti || !alEffectf || !alGenAuxiliaryEffectSlots || !alDeleteAuxiliaryEffectSlots || !alAuxiliaryEffectSloti) {
            fprintf(stderr, "Failed to load EFX functions.\n");
            exit(1);
        }

        efxOn = true;
    }

  std::atomic_int32_t NetworkAudioStream::initDelayCountMS(SAMPLE_TIME_DEFAULT * 2);

  namespace listener{
    void movePos(float x, float y, float z){
        sf::Listener::setPosition(x,y,z);
    }

    void moveDir(float x, float y, float z){
        sf::Listener::setDirection(x,y,z);
    }

    void setUpVector(float x, float y, float z){
        sf::Listener::setUpVector(x,y,z);
    }
  }

bool RecorderImpl::initialized = false;
int RecorderImpl::sampleRate = SAMPLE_RATE;
sf::Time RecorderImpl::packetTime = sf::milliseconds(20);
Recorder* RecorderImpl::instance = nullptr;

Recorder &RecorderImpl::getInstance()
{
  if (initialized)
  {
      return *instance;
  } else {
      perror("fabric soundmanager::Recorder");
  }
}

void RecorderImpl::fabric(int _sampleRate, sf::Time _packetTime)
{
  if(!initialized){
    sampleRate = _sampleRate;
    packetTime = _packetTime;
    instance = new Recorder(sampleRate, packetTime);
    initialized = true;
  }
}

void RecorderImpl::close(){
    delete instance;
    initialized = false;
}

void Recorder::enableRec(){
  if(isRecording == false) {
    rec.setProcessingIntervalOverride(packetTime, sampleRate);
    rec.start(sampleRate);
    isRecording = true;
  }
}

void Recorder::disableRec(){
  if(!detectNeeded.load()) {
    rec.stop();
    isRecording = false;
  }
}

void Recorder::enableSendData(){
  rec.setNeedSendData(true);
}

void Recorder::disableSendData(){
  rec.setNeedSendData(false);
}

void Recorder::setListenAudio(bool needListen){
  rec.setListen(needListen);
}

bool Recorder::isTalking(){
    return rec.isTalking();
}

void Recorder::setVolume(float volume){
  rec.setVolume(volume);
}
float Recorder::getVolume(){
  return rec.getVolume();
}

//max 4 seconds, mutex operation
void Recorder::setEcho(bool isOn, float decay, float delay){
    needApplyEcho = isOn;
    echoDecay = decay;
    echoDelay = delay;
    rec.setEcho(isOn, decay, delay);
}
//max 6 seconds, mutex operation
void Recorder::setReverb(bool isOn, float decay, float delay){
    needApplyReverb = isOn;
    reverbDecay = decay;
    reverbDelay = delay;
    rec.setReverb(isOn, decay, delay);
}

void Recorder::setNeedDetect(bool isNeed){
  rec.setNeedDetect(isNeed);
  detectNeeded = isNeed;
  if(isNeed){
    enableRec();
  }
}
void Recorder::setDetectValue(float value){
  rec.setDetectValue(value);
  detectValue = value;
}

float Recorder::getDetectValue(){
  return detectValue.load();
}

void Recorder::setIntervalTime(int milliseconds){
  rec.setProcessingIntervalOverride(sf::milliseconds(milliseconds), sampleRate);
}

sf::SoundBuffer Recorder::recordForTest(){
  rec.stop();

  sf::SoundBufferRecorder Trec;
  Trec.setChannelCount(SAMPLE_CHANNELS);
  Trec.start(SAMPLE_RATE);
  sf::sleep(sf::milliseconds(10000));
  Trec.stop();
  return Trec.getBuffer();
}

Recorder::~Recorder(){
  
}

Recorder::Recorder(){
  Recorder(SAMPLE_RATE, sf::milliseconds(20));
}

Recorder::Recorder(int _sampleRate, sf::Time _packetTime){
  detectValue = 0.2f;
  detectNeeded = false;
  isRecording = false;
  canSendData = false;

  needApplyEcho = false;
  needApplyReverb = false;
  echoDecay = 0;
  reverbDecay = 0;
  echoDelay = 0;
  reverbDelay = 0;

  initialize(_sampleRate, _packetTime);
}

void Recorder::reConstruct(int _sampleRate, sf::Time _packetTime){
  rec.stop();
  initialize(_sampleRate, _packetTime);
  
  rec.setDetectValue(detectValue);
  rec.setNeedDetect(detectNeeded);
  if(isRecording){
    isRecording = false;
    enableRec();
  }
  rec.setEcho(needApplyEcho, echoDecay, echoDelay);
  rec.setReverb(needApplyReverb, reverbDecay, reverbDelay);
}

int Recorder::getSampleTime(){
  return packetTime.asMilliseconds();
}
int Recorder::getSampleCount(){
  return sampleTimeGetCount(packetTime,sampleRate);
}

bool Recorder::swapDevice(std::string device){
  if (!rec.setDevice(device))
  {
    return false;
  }
  return true;
}

std::vector<std::string> Recorder::getDevices(){
  std::vector<std::string> availableDevices = sf::SoundRecorder::getAvailableDevices();
  return availableDevices;
}

void Recorder::initialize(int _sampleRate, sf::Time _packetTime){
  packetTime = _packetTime;
  sampleRate = _sampleRate;

  rec.setChannelCount(SAMPLE_CHANNELS);
  rec.setProcessingIntervalOverride(packetTime, sampleRate);
  rec.setListen(DEBUG_AUDIO);
  rec.setProcessingBufferFunction(player::Self::sendAudio);
}

    NetworkAudioStream::NetworkAudioStream(sf::Time _sampleTime, int _sampleChannels, int _sampleRate, int _sampleBits) : m_offset(0), opusMng(_sampleRate)
    {
        playing = false;
        canInitDelay = true;
        sampleChannels = _sampleChannels;
        sampleRate = _sampleRate;
        sampleBits = _sampleBits;
        noFailedCount = 0;
        samplesStable = 0;
        lastSampleTime = 0;
        sleepGetDataDelaySamples = 0;

        readsTry = 0;

        lossPercent = 0;

        crackleInfo.resize(256);
        for(auto crackle : crackleInfo){
            crackle = false;
        }
        cracklePos = 0;
        crackleTotal = 0;

        maxSkyTerribleTime = 512;

        initialize(sampleChannels, sampleRate);
    }

    NetworkAudioStream::NetworkAudioStream(){
        NetworkAudioStream(sf::milliseconds(40), 1, 16000, 16);
    }

    void NetworkAudioStream::setInitAudioDelay(int delayMS){
        initDelayCountMS = delayMS;
    }
    
    ALuint NetworkAudioStream::effect;
    ALuint NetworkAudioStream::effectSlot;
    std::atomic_bool NetworkAudioStream::effectIsOn = {false};
    std::mutex NetworkAudioStream::effectMutex;

    void NetworkAudioStream::enableReverb(float density, float diffusion, float gain, float gainHf, float decayTime, float decayHfRatio){
        if(!efxOn.load()) return;

        effectMutex.lock();
        if(!effectIsOn.load()){
            ALCcontext* aLcontext = alcGetCurrentContext();
            ALCdevice* aLdevice = alcGetContextsDevice(aLcontext);

            if (!alcIsExtensionPresent(aLdevice, "ALC_EXT_EFX")) {
                fprintf(stderr, "EFX extension not available\n");
                return;
            }

            alGenEffects(1, &effect);
            alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

            // Configura os parâmetros do reverb (ajuste conforme necessário)
            alEffectf(effect, AL_REVERB_DENSITY, density);
            alEffectf(effect, AL_REVERB_DIFFUSION, diffusion);
            alEffectf(effect, AL_REVERB_GAIN, gain);
            alEffectf(effect, AL_REVERB_GAINHF, gainHf);
            alEffectf(effect, AL_REVERB_DECAY_TIME, decayTime);
            alEffectf(effect, AL_REVERB_DECAY_HFRATIO, decayHfRatio);

            alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, 0.1f);
            alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, 0.02f);
            alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, 1.0f);
            alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, 0.03f);
            alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, 0.994f);
            alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, 0.0f);
            alEffecti(effect, AL_REVERB_DECAY_HFLIMIT, AL_TRUE);

            
            // Cria um slot de efeito auxiliar e anexa o efeito
            alGenAuxiliaryEffectSlots(1, &effectSlot);
            alAuxiliaryEffectSloti(effectSlot, AL_EFFECTSLOT_EFFECT, effect);
            effectIsOn = true;
        }
        effectMutex.unlock();
    }

    void NetworkAudioStream::disableReverb(){
        if(!efxOn.load()) return;

        // Limpeza
        effectMutex.lock();
        if(effectIsOn.load()){
            alDeleteEffects(1, &effect);
            alDeleteAuxiliaryEffectSlots(1, &effectSlot);
            effectIsOn = false;
        }
        effectMutex.unlock();
    }

    //thread safe
    void NetworkAudioStream::resizeSampleTime(sf::Time _sampleTime){
        /*
        geralMutexLockData.lock();
        bool needStart = false;
        if(getStatus() == sf::SoundSource::Playing){
            stop();
            needStart = true;
        }
        sampleTime = _sampleTime;
        sampleTime_MS = sampleTime.asMilliseconds();
        sampleCount = sampleTimeGetCount(_sampleTime, sampleRate);
        if(needStart) play();
        geralMutexLockData.unlock();*/
    }

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
    void NetworkAudioStream::insert(int audioNumb, data::buffer& data, int sampleTime)
    {
        int sampleCountTime = opusQueue.relativeActualCountSamples();
        if(sampleCountTime == 0){
            opusQueue.setReadPos(audioNumb);
        }

        //sf::Clock clock;
        //clock.restart();
        //std::cout << "Audio: " << audioNumb << std::endl;
        opusQueue.push(audioNumb, sampleTimeGetCount(sampleTime, sampleRate), data.getVector());
        lastSampleTime = sampleTime;

        playing = true;

        /*
        if(getStatus() != sf::SoundSource::Playing){
            geralMutexLockData.lock();
            if(getStatus() != sf::SoundSource::Playing){
                std::cout << "PLAY!" << std::endl;
                play();
            }
            geralMutexLockData.unlock();
        }*/

        return;

        if(data.size() > 0){
            opusMutex.lock();
                int sampleCount = sampleTimeGetCount(sampleTime, sampleRate);
                data::buffer decodedFec = opusMng.decodeC((const unsigned char*)data.getData(), data.size(), sampleCount, 1);
                //data::buffer decodedFec;
                data::buffer decoded = opusMng.decode(data, sampleCount);
            opusMutex.unlock();

            int readPos = audioQueue.getReadPos();
            int enqueued = audioNumb - readPos;
            int frameInit = audioNumb;
            if(audioNumb < 20 && readPos > 240){
                enqueued = (audioNumb + 256) - readPos;
                frameInit = (audioNumb + 256);
            }
            
            if(enqueued >= 4){
                advanceMutex.lock();
                int sampleCountInQueue = audioQueue.getSampleCountInFrame(readPos, audioNumb);
                std::cout << "SC in queue: " << sampleCountInQueue << std::endl;
                if(sampleCountGetTimeMS(sampleCountInQueue, sampleRate) > initDelayCountMS.load() * 3){
                    audioQueue.clearFrames(readPos, audioNumb - 2);
                    audioQueue.setReadPos(audioNumb - 1);
                    std::cout << "Audio advance" << std::endl;
                }
                advanceMutex.unlock();
            }

            if(decodedFec.size() > 0){
                int fecAudioNumb = audioNumb - 1;
                if(fecAudioNumb < 0) fecAudioNumb += 256;
                if(!audioQueue.slotState(fecAudioNumb) && fecAudioNumb > audioQueue.getLastPoped()){
                    std::vector<sf::Int16> bufferFec((sf::Int16*)decodedFec.getData(), (sf::Int16*)decodedFec.getData() + (decodedFec.size() / sizeof(sf::Int16)));
                    audioQueueFEC.push(fecAudioNumb, bufferFec);
                }
            }
            if(decoded.size() > 0){
                std::vector<sf::Int16> buffer((sf::Int16*)decoded.getData(), (sf::Int16*)decoded.getData() + (decoded.size() / sizeof(sf::Int16)));
                audioQueue.push(audioNumb, buffer);
            }
        }

        if(getStatus() != sf::SoundSource::Playing){
            geralMutexLockData.lock();
            if(getStatus() != sf::SoundSource::Playing){
                play();
            }
            geralMutexLockData.unlock();
        }
        //std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
    }

    void NetworkAudioStream::clean(){
        audioQueue.hyperClean();
    }

    bool NetworkAudioStream::isPlaying(){
        return playing.load();
    }

    //thread safe
    /*
    void insert(int audioNumb, std::vector<sf::Int16> data)
    {
        for(auto& frame : data){
            queueBuff.Enqueue(frame);
        }
    }*/

    void NetworkAudioStream::updateLossPercent(){
        lossPercent = ((lossPercent * 1.5) + ProtocolParser::getLossPercent()) / 3.5;
    }
    
    std::vector<sf::Int16> NetworkAudioStream::interpolate(std::vector<sf::Int16>& a, std::vector<sf::Int16>& b){
        int itSize = a.size();
        if(b.size() < itSize){
            itSize = b.size();
        }
        std::vector<sf::Int16> result(itSize);

        // Fator de Interpolação (varia de 0 a 1)
        float fatorDeInterpolacao = 0.0F;
        float sumFator = 1.0F / (float)(itSize);

        // Interpolação linear para cada amostra
        for (int i = 0; i < itSize; ++i) {
            float amostraInterpolada = a[i] + (b[i] - a[i]) * fatorDeInterpolacao;
            result[i] = amostraInterpolada;
            fatorDeInterpolacao += sumFator;
        }
        return result;
    }

    void NetworkAudioStream::setDelaySamplesPlayData(int samples){
      sleepGetDataDelaySamples = samples;
    }

    bool NetworkAudioStream::onGetData(sf::SoundStream::Chunk& data)
    {
        //setPitch(1.0f);
        //sf::Clock clock;
        //clock.restart();

        if (playing.load() == false){
          int sleepSamples = sleepGetDataDelaySamples.load();
          if (sleepSamples <= 0) sleepSamples = 320; //20ms
          m_samples.resize(sleepSamples);
          for(auto& sample : m_samples){
              sample = 0;
          }
          data.samples = m_samples.data();
          data.sampleCount = m_samples.size();
          return true;
        }

        int initDelayCountMS_temp = initDelayCountMS.load();

        int echoInitDelaySamplesMS = sampleCountGetTimeMS(sleepGetDataDelaySamples.load(), sampleRate);
        
        if(canInitDelay){
            std::cout << "i" << std::endl;
            
            //std::this_thread::sleep_for(std::chrono::milliseconds(sampleTime.asMilliseconds() * 10));
            std::this_thread::sleep_for(std::chrono::milliseconds((initDelayCountMS_temp * 2) + echoInitDelaySamplesMS));
            canInitDelay = false;

            int completeInitTrys = 0;
            //int sampleCountTime = sampleCountGetTimeMS(opusQueue.relativeCompleteCountSamples(), sampleRate);
            int sampleCountTime = sampleCountGetTimeMS(opusQueue.relativeActualCountSamples(), sampleRate);
            std::cout << "SampleTime: " << sampleCountTime << " of " << initDelayCountMS_temp << std::endl;

            while(sampleCountTime < (initDelayCountMS_temp + lastSampleTime)){

                if(ProtocolParser::getTrotling() <= 12){
                    if (lastSampleTime >= 201){
                    std::cout << "Network good break type 1" << std::endl;
                    break;
                    }
                }

                std::cout << "SampleTime: " << sampleCountTime << " of " << initDelayCountMS_temp << std::endl;
                completeInitTrys++;
                //std::this_thread::sleep_for(std::chrono::milliseconds(sampleTime.asMilliseconds()));
                std::this_thread::sleep_for(std::chrono::milliseconds(initDelayCountMS_temp));
                if(completeInitTrys > 2){
                    std::cout << "break sound init" << std::endl;
                    break;
                }

                //sampleCountTime = sampleCountGetTimeMS(opusQueue.relativeCompleteCountSamples(), sampleRate);
                sampleCountTime = sampleCountGetTimeMS(opusQueue.relativeActualCountSamples(), sampleRate);
            }
        }

        if(cracklePos == 0){
            int crackles = 0;
            for(auto crackle : crackleInfo){
                if(crackle) crackles++;
            }
            crackleTotal = crackles * 100 / 256;
        }

        int readPosActual = opusQueue.getReadPos();
        int lastPush = opusQueue.getLastPush();
        int sampleCountInQueue = opusQueue.getSampleCountInFrame(readPosActual, lastPush);
        int sampleTimeInQueue = sampleCountGetTimeMS(sampleCountInQueue, sampleRate);
        int maxSampleTimeInQueue = (initDelayCountMS_temp * 4) + sampleCountGetTimeMS(opusQueue.getSampleCountInFrame(readPosActual), sampleRate);
        maxSampleTimeInQueue = maxSampleTimeInQueue + (maxSampleTimeInQueue*20/100); // + 20%
        maxSampleTimeInQueue = maxSampleTimeInQueue + (maxSampleTimeInQueue*10/100); // + 10%
        maxSampleTimeInQueue = maxSampleTimeInQueue/2;
        maxSampleTimeInQueue += lastSampleTime;
        maxSampleTimeInQueue += echoInitDelaySamplesMS;
        
        //std::cout << "Audio Queue S: " << sampleCountInQueue << std::endl;
        //std::cout << "Audio Queue N: " << readPosActual << std::endl;
        //std::cout << "Audio Queue P: " << lastPush << std::endl;
        
        updateLossPercent();
        if(lossPercent > 70){
            maxSkyTerribleTime = 1000/2;
        } else if(lossPercent > 50){
            maxSkyTerribleTime = 860/2;
        } else if (lossPercent > 30){
            maxSkyTerribleTime = 740/2;
        } else if (lossPercent > 20){
            maxSkyTerribleTime = 620/2;
        } else if (lossPercent > 10){
            maxSkyTerribleTime = 540/2;
        } else if (lossPercent > 5){
            maxSkyTerribleTime = 512/2;
        } else if (lossPercent > 2){
            maxSkyTerribleTime = 420/2;
        } else {
            if(crackleTotal >= 10){
                maxSkyTerribleTime += 60;
                std::cout << "Terrible craclink, max NOW: " << maxSkyTerribleTime << std::endl;
            }
        }

        bool skipCrackle = true;

        if(samplesStable > (maxSampleTimeInQueue * 2)){
            samplesStable = maxSampleTimeInQueue;
        }

        if((sampleTimeInQueue > maxSampleTimeInQueue && sampleTimeInQueue > 200/2) || sampleTimeInQueue > maxSkyTerribleTime){

            if(samplesStable == 0){
                samplesStable = sampleTimeInQueue;
            }

            int maxSampleRelativeToStable = samplesStable + (samplesStable*20/100); //20%+

            if(sampleTimeInQueue > maxSampleRelativeToStable || sampleTimeInQueue > maxSkyTerribleTime){
                samplesStable = 0;

                int removeSamplesCount = 0;

                while(removeSamplesCount < maxSampleTimeInQueue){
                    std::vector<char> bufferOpus;
                    removeSamplesCount += opusQueue.pop(bufferOpus);
                }

                int lastPoped = opusQueue.getLastPoped();
                std::vector<char> bufferOpus;
                int sampleCount = opusQueue.pop(bufferOpus);
                data::buffer decoded;
                std::vector<sf::Int16> decodedSF;

                std::cout << "Rem: " << removeSamplesCount << std::endl;
                skipCrackle = false;
            }
        }

        if(skipCrackle){
            crackleInfo[cracklePos] = false;
        } else {
            crackleInfo[cracklePos] = true;
        }
        cracklePos++;
        if(cracklePos >= 256) cracklePos = 0;

        std::vector<sf::Int16> a;
        m_samples.swap(a);

        int lastPoped = opusQueue.getLastPoped();
        std::vector<char> bufferOpus;
        int sampleCount = opusQueue.pop(bufferOpus);
        data::buffer decoded;
        std::vector<sf::Int16> decodedSF;
        if(sampleCount > 0){
                //decodedFec = opusMng.decodeC((const unsigned char*)bufferOpus.data(), bufferOpus.size(), sampleCount, 1);
                decoded = opusMng.decode((const unsigned char*)bufferOpus.data(), bufferOpus.size(), sampleCount);
                sf::Int16 * decRI = reinterpret_cast<sf::Int16*>(decoded.getData());
                decodedSF.insert(decodedSF.end(), decRI, decRI + (decoded.size() / sizeof(sf::Int16)));
        } else {
            //get next frame from FEC
            int readPos = opusQueue.getReadPos();
            if(opusQueue.slotState(readPos)){
                sampleCount = opusQueue.preview(bufferOpus);
                decoded = opusMng.decodeC((const unsigned char*)bufferOpus.data(), bufferOpus.size(), sampleCount, 1);
                sf::Int16 * decRI = reinterpret_cast<sf::Int16*>(decoded.getData());
                decodedSF.insert(decodedSF.end(), decRI, decRI + (decoded.size() / sizeof(sf::Int16)));
            }
            if(sampleCount > 0){
                std::cout << "FEC: " << readPos << std::endl;

                //try recover this frame by interpolate
                int readCompare = readPos - 2;
                if(readCompare < 0) readCompare += 256;
                if(lastPoped == readCompare){
                    m_samples = interpolate(a,decodedSF);
                }
            } else {
                std::cout << "End " << std::endl;
            }
        }

        if(sampleCount > 0){
            m_samples.insert(m_samples.end(), decodedSF.begin(), decodedSF.end());
            readsTry = 0;
            data.samples = m_samples.data();
            data.sampleCount = m_samples.size();
            std::cout << "Exec " << m_samples.size() << std::endl;
            //std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
            return true;
        } else {
            readsTry++;
            if(readsTry == 1){
                int sampleCountDelay = sampleTimeGetCount(lastSampleTime, sampleRate);
                m_samples.resize(sampleCountDelay);
                for(auto& sample : m_samples){
                    sample = 0;
                }
                data.samples = m_samples.data();
                data.sampleCount = m_samples.size();
                return true;
            }
            //std::this_thread::sleep_for(std::chrono::milliseconds(readsTry));
            std::cout << "LOST: " << readsTry << std::endl;
            if(readsTry >= 3 && readsTry <= 4){
              canInitDelay = true;
            }
            if(readsTry > 6){
                std::cout << "ERROR LOST" << std::endl;
                //opusQueue.hyperClean();
                opusQueue.clearFrames(0,255);
                canInitDelay = true;
                readsTry = 0;
                m_samples.resize(320);
                for(auto& sample : m_samples){
                    sample = 0;
                }
                data.samples = m_samples.data();
                data.sampleCount = m_samples.size();
                playing = false;
                return true;
            }
            int sampleCountDelay = sampleTimeGetCount(lastSampleTime / readsTry, sampleRate);
            if (sampleCountDelay < 320) sampleCountDelay = 320;
            m_samples.resize(sampleCountDelay);
            data.samples = m_samples.data();
            data.sampleCount = m_samples.size();
            //std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
            return true;
        }

        return true;
        
        /*
        sf::Clock clock;
        clock.restart();
        
        if(canInitDelay){
            std::cout << "i" << std::endl;
            
            //std::this_thread::sleep_for(std::chrono::milliseconds(sampleTime.asMilliseconds() * 10));
            std::this_thread::sleep_for(std::chrono::milliseconds(initDelayCountMS_temp));
            canInitDelay = false;

            int completeInitTrys = 0;
            while(sampleCountGetTimeMS(audioQueue.relativeCompleteCountSamples(), sampleRate) < 100){
                completeInitTrys++;
                //std::this_thread::sleep_for(std::chrono::milliseconds(sampleTime.asMilliseconds()));
                std::this_thread::sleep_for(std::chrono::milliseconds(initDelayCountMS_temp / (completeInitTrys + 1)));
                if(completeInitTrys > 2){
                    std::cout << "break sound init" << std::endl;
                    break;
                }
            }
        }

        std::vector<sf::Int16> a;
        m_samples.swap(a);

        int fecNumb = audioQueue.getReadPos();
        bool success = audioQueue.pop(m_samples);

        //FEC
        if(!success){
            audioQueueFEC.setReadPos(fecNumb);
            success = audioQueueFEC.pop(m_samples);
            if(success) std::cout << "FEC: " << fecNumb << std::endl;
        }

        if(!success){
            //std::cout << "ERROR LOST : " << readsTry << std::endl;
            readsTry++;
            //Interpolate FEC + NEXT
            if(readsTry == 1){
                std::vector<sf::Int16> b;

                int readPos = audioQueue.getReadPos();
                bool successPreview = false;
                if(audioQueue.slotState(readPos)){
                    successPreview = audioQueue.preview(b);
                } else if (audioQueueFEC.slotState(readPos)){
                    audioQueueFEC.setReadPos(readPos);
                    successPreview = audioQueueFEC.preview(b);
                    if(successPreview) std::cout << "FEC_Preview: " << readPos << std::endl;
                }
                if(successPreview){
                     m_samples = interpolate(a,b);
                        readsTry = 0;
                        data.samples = m_samples.data();
                        data.sampleCount = m_samples.size();
                        return true;
                } else if(sampleCountGetTimeMS(audioQueue.relativeCompleteCountSamples(), sampleRate) > 0){
                    return true;
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(readsTry));
            }
            std::cout << "LOST: " << readsTry << std::endl;
            if(readsTry > 6){
                std::cout << "ERROR LOST" << std::endl;
                audioQueue.hyperClean();
                canInitDelay = true;
                readsTry = 0;
                return false;
            }
            int sampleCount = sampleTimeGetCount(initDelayCountMS_temp, sampleRate);
            m_samples.resize(sampleCount * readsTry);
            data.samples = m_samples.data();
            data.sampleCount = m_samples.size();
            std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
            return true;
        }

        if(success){
            readsTry = 0;
            data.samples = m_samples.data();
            data.sampleCount = m_samples.size();
            std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
            return true;
        }
        std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
        return true;*/

        /*

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
        */

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
    }

    void NetworkAudioStream::onSeek(sf::Time timeOffset)
    {
        m_offset = static_cast<std::size_t>(timeOffset.asMilliseconds()) * getSampleRate() * getChannelCount() / 1000;
    }

}