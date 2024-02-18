#include "opusmanager.h"
#include "soundmanager.h"

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
        opus_int32 useFEC = 1; // 1 para habilitar FEC
        //opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(useFEC));
        //opus_encoder_ctl(encoder, OPUS_SET_FORCE_CHANNELS(OPUS_AUTO)); // Configurar para usar canais mono ou stereo conforme necessário
        //opus_encoder_ctl(encoder, OPUS_SET_DTX(0)); // Desabilitar DTX ao usar FEC
        //opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(10)); // Configurar a porcentagem de perda para acionar FEC
        opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1)); // Habilitar In-Band FEC
        decoder = opus_decoder_create(sampleRate, SAMPLE_CHANNELS, &Decerror);
        opus_decoder_ctl(decoder, OPUS_SET_INBAND_FEC(1)); // Habilitar In-Band FEC
        //opus_decoder_ctl(decoder, OPUS_SET_)
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
        return decodeC((const unsigned char*)buffer.getData(), buffer.size(), sampleCount, 0);
    }

    data::buffer OpusManager::decode(const unsigned char* buffer, int bufferSize, int sampleCount){
        return decodeC(buffer, bufferSize, sampleCount, 0);
    }

    data::buffer OpusManager::decodeC(const unsigned char* buffer, int bufferSize, int sampleCount, int isFECbuff){
        opus_int16* decodedSamples = new opus_int16[sampleCount * 4];
        //std::cout << "DEC: " << sampleCount << std::endl;
        int decodedLen = opus_decode(decoder, buffer, bufferSize, decodedSamples, sampleCount, isFECbuff);
        if(decodedLen < sampleCount){
            data::buffer emptyBuff;
            //OPUS_BAD_ARG
            //OPUS_BUFFER_TOO_SMALL
            //OPUS_INTERNAL_ERROR
            //OPUS_INVALID_PACKET
            //OPUS_UNIMPLEMENTED
            //OPUS_INVALID_STATE
            //OPUS_ALLOC_FAIL
            delete[] decodedSamples;
            return emptyBuff;
        }
        data::buffer outDataBuffer(decodedSamples, decodedLen);
        delete[] decodedSamples;
        return outDataBuffer;
    }

    OpusManager::~OpusManager(){
        opus_decoder_destroy(decoder);
        opus_encoder_destroy(encoder);
    }