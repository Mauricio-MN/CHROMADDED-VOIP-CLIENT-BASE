#include "opusmanager.h"

    OpusManager& OpusManagerImpl::getInstance(){
        static OpusManager instance;
        return instance;
    }

    OpusManager::OpusManager(){
        Encerror = 0;
        Decerror = 0;
        encoder = opus_encoder_create(SAMPLE_RATE, SAMPLE_CHANNELS, OPUS_APPLICATION_VOIP, &Encerror);
        decoder = opus_decoder_create(SAMPLE_RATE, SAMPLE_CHANNELS, &Decerror);
    }

    data::buffer OpusManager::encode(sf::Int16 *samples){
        int maxdata = 640*2;
        unsigned char* dataPTR = new unsigned char[maxdata];
        opus_int16* samplePTR = (opus_int16*)(samples);
        int actualLen = opus_encode(encoder, samplePTR, 640, dataPTR, maxdata);

        data::buffer buffer(reinterpret_cast<char*>(dataPTR), actualLen);
        delete[] dataPTR;
        return buffer;
    }

    data::buffer OpusManager::decode(data::buffer &buffer){
        opus_int16* decodedSamples = new opus_int16[640];
        int decodedLen = opus_decode(decoder, reinterpret_cast<const unsigned char*>(buffer.getData()), buffer.size(), decodedSamples, 640, 0);
        
        int bufferLen = sizeof(opus_int16) / sizeof(char) * decodedLen;
        char* outBuffer = new char[bufferLen];

        protocol::tools::transformArrayToBuffer(decodedSamples, decodedLen, outBuffer);
        data::buffer outDataBuffer(outBuffer, bufferLen);
        delete[] outBuffer;
        return outDataBuffer;
    }