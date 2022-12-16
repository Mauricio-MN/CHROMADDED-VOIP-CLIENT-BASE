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
    data::buffer decode(data::buffer &buffer);

    OpusManager();

};

class OpusManagerImpl{
    public:
    static OpusManager& getInstance();
};

#endif