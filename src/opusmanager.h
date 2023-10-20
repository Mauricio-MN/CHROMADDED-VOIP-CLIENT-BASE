#ifndef OPUSMANAGER_H // include guard
#define OPUSMANAGER_H

#include "data.h"
#include "soundmanager.h"
#include "SFML/Audio.hpp"

#include <opus/opus.h>

class OpusManager{
private:
    int Encerror;
    int Decerror;
    OpusEncoder* encoder;
    OpusDecoder* decoder;
public:

    data::buffer encode(sf::Int16 *samples);
    data::buffer encode(sf::Int16 *samples, int sampleCount);
    data::buffer decode(data::buffer &buffer);
    data::buffer decode(data::buffer &buffer, int sampleCount);
    data::buffer decode(const unsigned char* buffer, int bufferSize, int sampleCount);

    OpusManager();
    OpusManager(int sampleRate);

    ~OpusManager();

};

#endif