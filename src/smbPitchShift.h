#ifndef SMBPITCHSHIFT_H // include guard
#define SMBPITCHSHIFT_H

#define MAX_FRAME_LENGTH 8192

void smbPitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata);

#endif