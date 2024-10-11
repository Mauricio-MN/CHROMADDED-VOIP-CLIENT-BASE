#ifndef REPLAYGAIN_H
#define REPLAYGAIN_H

#include <vector>
#include <stdint.h>

// Função para calcular o nível de loudness ponderado
double calculateLoudness(const std::vector<int16_t>& samples, int sampleRate);

// Função para ajustar o volume dos samples
void applyGain(std::vector<int16_t>& samples, double gain);

#endif