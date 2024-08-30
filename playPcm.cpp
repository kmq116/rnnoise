#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#define SAMPLE_RATE 96000
#define FRAMES_PER_BUFFER 480
typedef struct
{
  FILE *file;
} UserData;
static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo *timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData)
{
  UserData *data = (UserData *)userData;
  float *out = (float *)outputBuffer;
  size_t bytesRead = fread(out, sizeof(float), framesPerBuffer, data->file);

  if (bytesRead < framesPerBuffer)
  {
    // End of file reached, stop the stream
    return paComplete;
  }
  return paContinue;
}
int main()
{
  Pa_Initialize();

  UserData userData;
  userData.file = fopen("./48000啸叫 pcm.pcm", "rb");

  if (!userData.file)
  {
    printf("Failed to open PCM file\n");
    return -1;
  }

  PaStreamParameters outputParameters;
  outputParameters.device = Pa_GetDefaultOutputDevice();
  outputParameters.channelCount = 1;       // Mono
  outputParameters.sampleFormat = paInt16; // Float32 format
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = NULL;
  PaStream *stream;
  Pa_OpenStream(&stream, NULL, &outputParameters, SAMPLE_RATE,
                FRAMES_PER_BUFFER, paClipOff, paCallback, &userData);
  if (stream == NULL)
  {
    printf("Failed to open PortAudio stream\n");
    return -1;
  }
  Pa_StartStream(stream);
  printf("Playing PCM file...\n");
  Pa_Sleep(200000); // Play for 5 seconds (adjust as needed)
  Pa_StopStream(stream);
  Pa_CloseStream(stream);
  fclose(userData.file);
  Pa_Terminate();
  return 0;
}