#include "opusmanager.h"

    OpusManager::OpusManager(){
        Encerror = 0;
        Decerror = 0;
        encoder = opus_encoder_create(SAMPLE_RATE, SAMPLE_CHANNELS, OPUS_APPLICATION_VOIP, &Encerror);
        decoder = opus_decoder_create(SAMPLE_RATE, SAMPLE_CHANNELS, &Decerror);
        /*opus_encoder_ctl(encoder, OPUS_SET_BITRATE(OPUS_AUTO));
        opus_encoder_ctl(encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_MEDIUMBAND));
        opus_encoder_ctl(encoder, OPUS_SET_VBR(1));
        opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));
        opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1));
        opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(5));*/
    }

    OpusManager::OpusManager(int sampleRate){
        Encerror = 0;
        Decerror = 0;
        encoder = opus_encoder_create(sampleRate, SAMPLE_CHANNELS, OPUS_APPLICATION_VOIP, &Encerror);
        decoder = opus_decoder_create(sampleRate, SAMPLE_CHANNELS, &Decerror);
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

        data::buffer outDataBuffer(decodedSamples, decodedLen);
        delete[] decodedSamples;
        return outDataBuffer;
    }

    data::buffer OpusManager::decode(const unsigned char* buffer, int bufferSize, int sampleCount){
        opus_int16* decodedSamples = new opus_int16[sampleCount];
        int decodedLen = opus_decode(decoder, buffer, bufferSize, decodedSamples, sampleCount, 0);
        if(decodedLen < sampleCount){
            std::cout << "ERR" << std::endl;
        }
        data::buffer outDataBuffer(decodedSamples, decodedLen);
        delete[] decodedSamples;
        return outDataBuffer;
    }

    OpusManager::~OpusManager(){
        opus_decoder_destroy(decoder);
        opus_encoder_destroy(encoder);
    }