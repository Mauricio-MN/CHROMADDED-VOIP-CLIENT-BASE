#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

// Função para calcular a energia RMS dos samples
double calculateRMS(const std::vector<int16_t>& samples) {
    double sum = 0.0;
    for (auto sample : samples) {
        sum += sample * sample;
    }
    return sqrt(sum / samples.size());
}

// Função para aplicar a curva de ponderação A (simplificada)
double applyAWeighting(double frequency) {
    // Esta é uma aproximação simplificada da curva de ponderação A
    double ra = pow(12194, 2) * pow(frequency, 4) /
                ((frequency * frequency + pow(20.6, 2)) *
                 sqrt((frequency * frequency + pow(107.7, 2)) * (frequency * frequency + pow(737.9, 2))) *
                 (frequency * frequency + pow(12194, 2)));
    return ra;
}

// Função para calcular o nível de loudness ponderado
double calculateLoudness(const std::vector<int16_t>& samples, int sampleRate) {
    double rms = calculateRMS(samples);
    double weightedRMS = 0.0;
    for (int i = 0; i < samples.size(); ++i) {
        double frequency = (i * sampleRate) / samples.size();
        double weighting = applyAWeighting(frequency);
        weightedRMS += samples[i] * samples[i] * weighting;
    }
    weightedRMS = sqrt(weightedRMS / samples.size());
    return 20 * log10(weightedRMS);
}

// Função para ajustar o volume dos samples
void applyGain(std::vector<int16_t>& samples, double gain) {
    double linearGain = pow(10, gain / 20.0);
    for (auto& sample : samples) {
        int32_t adjustedSample = static_cast<int32_t>(sample * linearGain);
        // Clipping
        sample = std::clamp(adjustedSample, static_cast<int32_t>(INT16_MIN), static_cast<int32_t>(INT16_MAX));
    }
}