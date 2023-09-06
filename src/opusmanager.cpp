#include "opusmanager.h"
#include "protocolTools.h"

    int OpusManagerImpl::sampleRate = SAMPLE_RATE;
    OpusManager* OpusManagerImpl::instance = nullptr;
    bool OpusManagerImpl::initialized = false;

    OpusManager& OpusManagerImpl::getInstance(){
        assert(initialized && "Error: fabric OpusManagerImpl");
        return *instance;
    }

    void OpusManagerImpl::fabric(int _sampleRate){
        assert(!initialized && "Error: OpusManagerImpl already initialized");
        if(!initialized){
            initialized = true;
            sampleRate = _sampleRate;
            instance = new OpusManager(sampleRate);
        }
    }

    OpusManager::OpusManager(){
        Encerror = 0;
        Decerror = 0;
        encoder = opus_encoder_create(SAMPLE_RATE, SAMPLE_CHANNELS, OPUS_APPLICATION_VOIP, &Encerror);
        decoder = opus_decoder_create(SAMPLE_RATE, SAMPLE_CHANNELS, &Decerror);
    }

    OpusManager::OpusManager(int sampleRate){
        Encerror = 0;
        Decerror = 0;
        encoder = opus_encoder_create(SAMPLE_RATE, SAMPLE_CHANNELS, OPUS_APPLICATION_VOIP, &Encerror);
        decoder = opus_decoder_create(SAMPLE_RATE, SAMPLE_CHANNELS, &Decerror);
    }

    //Default 640 samples only 16000 sample rate
    data::buffer OpusManager::encode(sf::Int16 *samples){
        return encode(samples, 640);
    }
    //Default 640 samples only 16000 sample rate
    data::buffer OpusManager::decode(data::buffer &buffer){
        return decode(buffer, 640);
    }

    data::buffer OpusManager::encode(sf::Int16 *samples, int sampleCount){
        int maxdata = sampleCount*2;
        unsigned char* dataPTR = new unsigned char[maxdata];
        opus_int16* samplePTR = (opus_int16*)(samples);
        int actualLen = opus_encode(encoder, samplePTR, sampleCount, dataPTR, maxdata);

        data::buffer buffer(reinterpret_cast<char*>(dataPTR), actualLen);
        delete[] dataPTR;
        return buffer;
    }

    data::buffer OpusManager::decode(data::buffer &buffer, int sampleCount){
        opus_int16* decodedSamples = new opus_int16[sampleCount];
        int decodedLen = opus_decode(decoder, reinterpret_cast<const unsigned char*>(buffer.getData()), buffer.size(), decodedSamples, sampleCount, 0);
        int bufferLen = decodedLen * sizeof(opus_int16);

        data::buffer outDataBuffer(reinterpret_cast<const char*>(decodedSamples), bufferLen);
        delete[] decodedSamples;
        return outDataBuffer;
    }s