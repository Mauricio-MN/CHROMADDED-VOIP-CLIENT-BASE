#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
//#include "../libs/SDL2/SDL.h"
#include <sys/time.h>
#include <ctime>
#include <math.h>
#include <cmath>
#include <cstdint> 

#include "../libs/AL/al.h"
#include "../libs/AL/alc.h"

#include "cript.h"

#include "audio.h"

#define MY_PI 3.14159265358979323846

bool captureAudio = false;

#define BUFFER_COUNT 3

typedef struct bufferPlusSource {
  ALuint buffers[BUFFER_COUNT];
  ALuint sources[1];
  uint64_t totalFramesToEnqueue;
  uint64_t totalFramesEnqueued;
  uint64_t bufferSizeInFrames;
  
  double duration;
};

typedef struct _AppState {
  bufferPlusSource player[20];

} AppState;

AppState * appState;

bool loopGetBuffer()
{
  const ALCchar *devices;
  const ALCchar *devicesPtr;
  ALCdevice *mainDev;
  ALCcontext *mainContext;
  ALCdevice *captureDev;
  time_t currentTime;
  time_t lastTime;
  ALuint buffer;
  ALuint source;
  ALint playState; 

  ALubyte captureBuffer[1048576];
  ALubyte *captureBufPtr;
  ALint samplesAvailable;
  ALint samplesCaptured;

  appState = new AppState();


  alGenBuffers(BUFFER_COUNT, appState->player[0].buffers);

  alSourceQueueBuffers(appState->player[0].sources[0], BUFFER_COUNT, appState->player[0].buffers);

// Print the list of capture devices 
printf("Available playback devices:\n");

devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER); 
const ALCchar * ptr = devices; 
//while (ptr[0] != NULL)
while (*ptr)
{ 
   printf("   %s\n", ptr); 
   ptr += strlen(ptr) + 1; 
} 

// Open a playback device and create a context first 
printf("Opening playback device:\n"); 
mainDev = alcOpenDevice(NULL); 
if (mainDev == NULL) 
{ 
  printf("Unable to open playback device!\n"); 
  exit(1); 
} 
devices = alcGetString(mainDev, ALC_DEVICE_SPECIFIER); 
printf("   opened device '%s'\n", devices); 
mainContext = alcCreateContext(mainDev, NULL); 
if (mainContext == NULL) 
{ 
  printf("Unable to create playback context!\n"); 
  exit(1); 
} 
printf("   created playback context\n"); 

// Make the playback context current 
alcMakeContextCurrent(mainContext); 
alcProcessContext(mainContext); 

// Print the list of capture devices 

printf("Available capture devices:\n"); 
devices = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER); 
ptr = devices; 

//while (ptr[0] != NULL)
while (*ptr)
{ 
   printf("   %s\n", ptr); 
   ptr += strlen(ptr) + 1; 
}

// Open the default device 
printf("Opening capture device:\n"); 
captureDev = alcCaptureOpenDevice(NULL, 8000, AL_FORMAT_MONO16, 800); 
if (captureDev == NULL) 
{  
  printf("   Unable to open device!\n"); 
  exit(1); 
} 
devices = alcGetString(captureDev, ALC_CAPTURE_DEVICE_SPECIFIER); 
printf("   opened device %s\n", devices); 

//PlayStart
alSourcePlay(source);

// Wait for three seconds to prompt the user 
for (int i = 3; i > 0; i--) 
{ 
  printf("Starting capture in %d...\r", i); 
  fflush(stdout); 
  lastTime = time(NULL); 
  currentTime = lastTime; 
  while (currentTime == lastTime) 
  { 
     currentTime = time(NULL); 
     usleep(100000); 
  } 
} 

printf("Starting capture NOW!\n"); 
fflush(stdout); 
lastTime = currentTime; 

// Capture (roughly) five seconds of audio 
alcCaptureStart(captureDev); 
samplesCaptured = 0; 
captureBufPtr = captureBuffer; 
while (currentTime < (lastTime + 5)) 
{ 
  // Get the number of samples available 
  alcGetIntegerv(captureDev, ALC_CAPTURE_SAMPLES, 1, &samplesAvailable); 

  // Copy the samples to our capture buffer 
  if (samplesAvailable > 0) 
  { 
     alcCaptureSamples(captureDev, captureBufPtr, samplesAvailable); 
     samplesCaptured += samplesAvailable; 
     printf("Captured %d samples (adding %d)\r", samplesCaptured, 
        samplesAvailable); 
     fflush(stdout); 

     // Advance the buffer (two bytes per sample * number of samples) 
     captureBufPtr += samplesAvailable * 2; 
  } 

  // Wait for a bit 
  usleep(10000); 

  // Update the clock 
  currentTime = time(NULL); 
} 
printf("\nPausing capture.\n"); 
alcCaptureStop(captureDev); 

// Wait for three seconds to prompt the user 
for (int i = 3; i > 0; i--) 
{ 
  printf("Resuming capture in %d...\r", i); 
  fflush(stdout); 
  lastTime = time(NULL); 
  currentTime = lastTime; 
  while (currentTime == lastTime) 
  { 
     currentTime = time(NULL); 
     usleep(100000); 
  } 
} 

printf("Resuming capture NOW!\n"); 
fflush(stdout); 
lastTime = currentTime; 

// Capture (roughly) five seconds of audio 
alcCaptureStart(captureDev); 
while (currentTime < (lastTime + 5)) 
{ 
  // Get the number of samples available 
  alcGetIntegerv(captureDev, ALC_CAPTURE_SAMPLES, 1, &samplesAvailable); 

  // Copy the samples to our capture buffer 
  if (samplesAvailable > 0) 
  { 
     alcCaptureSamples(captureDev, captureBufPtr, samplesAvailable); 
     samplesCaptured += samplesAvailable; 
     printf("Captured %d samples (adding %d)\r", samplesCaptured, 
        samplesAvailable); 
     fflush(stdout); 

     // Advance the buffer (two bytes per sample * number of samples) 
     captureBufPtr += samplesAvailable * 2; 
  } 

  // Wait for a bit 
  usleep(10000); 

  // Update the clock 
  currentTime = time(NULL); 
} 

printf("\nDone capturing.\n"); 
alcCaptureStop(captureDev); 

// Play back the captured data 
printf("Starting playback...\n"); 
fflush(stdout); 

// Generate an OpenAL buffer for the captured data 
alGenBuffers(1, &buffer); 
alGenSources(1, &source); 
alBufferData(buffer, AL_FORMAT_MONO16, captureBuffer,samplesCaptured*2, 8000); 
alSourcei(source, AL_BUFFER, buffer); 
alSourcePlay(source); 

// Wait for the source to stop playing 
playState = AL_PLAYING; 
while (playState == AL_PLAYING) 
{ 
  printf("  source %d is playing...\r", source); 
  fflush(stdout); 
  alGetSourcei(source, AL_SOURCE_STATE, &playState); 
  usleep(100000); 
} 
printf("\nDone with playback.\n"); 
fflush(stdout); 

// Shut down OpenAL 
alDeleteSources(1, &source); 
alDeleteBuffers(1, &buffer); 
alcMakeContextCurrent(NULL); 
alcCloseDevice(mainDev); 
alcCaptureCloseDevice(captureDev); 

}


void UpdateSourceLocation(ALuint src) {
  
  struct timeval time_now{};
  gettimeofday(&time_now, nullptr);
  time_t msecs_time = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
  const int ORBIT_SPEED = 1;

  double theta = fmod(msecs_time * ORBIT_SPEED, MY_PI * 2);
  ALfloat x = 3 * cos(theta);
  ALfloat y = 0.5 * sin(theta);
  ALfloat z = 1.0 * sin(theta);
  
  alSource3f(src, AL_POSITION, x, y, z);
  
  return;
}

/*
bool startRecorderBuffer()
{
  // Open a playback device and create a context first
  mainDev = alcOpenDevice(NULL);
  if (mainDev == NULL)
  {
    printf("Unable to open playback device!\n");
    exit(1);
  }
  devices = alcGetString(mainDev, ALC_DEVICE_SPECIFIER);
  printf("   opened device '%s'\n", devices);

  // playback Context
  mainContext = alcCreateContext(mainDev, NULL);

  if (mainContext == NULL)
  {
    printf("Unable to create playback context!\n");
    exit(1);
  }
  printf("   created playback context\n");

  // Make the playback context current
  alcMakeContextCurrent(mainContext);
  alcProcessContext(mainContext);

  // Open the default device
  printf("Opening capture device:\n");
  captureDev = alcCaptureOpenDevice(NULL, 8000, AL_FORMAT_MONO16, 800);
  if (captureDev == NULL)
  {
    printf("   Unable to open device!\n");
    exit(1);
  }
  devices = alcGetString(captureDev, ALC_CAPTURE_DEVICE_SPECIFIER);
  printf("   opened device %s\n", devices);

  currentTime = time(NULL);

  lastTime = currentTime;

  // Capture (roughly) five seconds of audio

  alcCaptureStart(captureDev);
  samplesCaptured = 0;
  captureBufPtr = captureBuffer;

  while (currentTime < (lastTime + 5))
  {
    // Get the number of samples available
    alcGetIntegerv(captureDev, ALC_CAPTURE_SAMPLES, 1, &samplesAvailable);

    // Copy the samples to our capture buffer
    if (samplesAvailable > 0)
    {
      alcCaptureSamples(captureDev, captureBufPtr, samplesAvailable);
      samplesCaptured += samplesAvailable;
      printf("Captured %d samples (adding %d)\r", samplesCaptured,
             samplesAvailable);
      fflush(stdout);

      // Advance the buffer (two bytes per sample * number of samples)
      captureBufPtr += samplesAvailable * 2;
    }

    // Wait for a bit
    usleep(10000);

    // Update the clock
    currentTime = time(NULL);
  }

  printf("\nPausing capture.\n");
  alcCaptureStop(captureDev);

  // Generate an OpenAL buffer for the captured data
  alGenBuffers(1, &buffer[0]);
  alGenSources(1, &source[0]);
  alBufferData(buffer[0], AL_FORMAT_MONO16, captureBuffer, samplesCaptured * 2, 8000);
  alSourcei(source[0], AL_BUFFER, buffer[0]);
  alSourcePlay(source[0]);

  // Wait for the source to stop playing
  playState[0] = AL_PLAYING;
  while (playState[0] == AL_PLAYING)
  {
    printf("  source %d is playing...\r", source);
    fflush(stdout);
    alGetSourcei(source[0], AL_SOURCE_STATE, &playState[0]);
    usleep(100000);
  }
  printf("\nDone with playback.\n");
  fflush(stdout);

  // Shut down OpenAL
  alDeleteSources(1, &source[0]);
  alDeleteBuffers(1, &buffer[0]);
  alcMakeContextCurrent(NULL);
  alcCloseDevice(mainDev);
  alcCaptureCloseDevice(captureDev);
}

*/