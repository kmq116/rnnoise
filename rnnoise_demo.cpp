#include <stdlib.h>
#include <stdio.h>
#include <portaudio.h>
#include <string.h>
#include <cmath>
#include <rnnoise.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 480
#define FRAME_SIZE 480 // 添加这一行

static void checkError(PaError err)
{
  if (err != paNoError)
  {
    printf("PortAudio Error: %s\n", Pa_GetErrorText(err));
    exit(EXIT_FAILURE);
  }
}

static inline float max(float a, float b)
{
  return a > b ? a : b;
}

// 添加 rnnoise 状态作为用户数据
struct UserData
{
  DenoiseState *st;
};
static int patestCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
  float *in = (float *)inputBuffer;
  float *out = (float *)outputBuffer;
  UserData *data = (UserData *)userData;

  // 处理音频数据
  // for (unsigned long i = 0; i < framesPerBuffer; i += FRAME_SIZE)
  // {
  //   float input[FRAME_SIZE];
  //   float output[FRAME_SIZE];

  //   // 将输入数据复制到临时缓冲区
  //   for (int j = 0; j < FRAME_SIZE; j++)
  //   {
  //     input[j] = in[i + j];
  //   }

  //   // 应用 rnnoise 降噪
  //   rnnoise_process_frame(data->st, output, input);

  //   // 将处理后的数据复制到输出缓冲区
  //   for (int j = 0; j < FRAME_SIZE; j++)
  //   {
  //     out[i + j] = output[j];
  //   }
  // }

  for (unsigned long i = 0; i < framesPerBuffer * 2; i++)
  {
    out[i] = in[i]; // 直接将输入复制到输出
  }

  int dispSize = 100;
  printf("\r");
  float vol_l = 0;
  float vol_r = 0;
  for (unsigned long i = 0; i < framesPerBuffer * 2; i += 2)
  {

    vol_l = max(vol_l, std::abs(in[i]));
    vol_r = max(vol_r, std::abs(in[i + 1]));
  }

  for (int i = 0; i < dispSize; i++)
  {

    float barProportion = i / (float)dispSize;
    if (barProportion <= vol_l && barProportion <= vol_r)
    {
      printf("█");
    }
    else if (barProportion <= vol_l)
    {
      printf("▀");
    }
    else if (barProportion <= vol_r)
    {
      printf("▄");
    }
    else
    {
      printf(" ");
    }
  }

  fflush(stdout);

  return 0;
}

int main()
{

  // 初始化 rnnoise
  UserData userData;
  userData.st = rnnoise_create(NULL);

  PaError err;
  err = Pa_Initialize();
  checkError(err);
  int numDevices = Pa_GetDeviceCount();
  if (numDevices < 0)
  {
    printf("Error: %s\n", Pa_GetErrorText(err));
    exit(EXIT_FAILURE);
  }
  else if (numDevices == 0)
  {
    printf("No devices found\n");
    exit(EXIT_FAILURE);
  }
  else
  {
    printf("Number of devices: %d\n", numDevices);
  }

  const PaDeviceInfo *deviceInfo;
  for (int i = 0; i < numDevices; i++)
  {
    deviceInfo = Pa_GetDeviceInfo(i);
    printf("Device %d: %s\n", i, deviceInfo->name);
    printf("  Default sample rate: %d\n", deviceInfo->maxInputChannels);
    printf("  Max input channels: %d\n", deviceInfo->maxOutputChannels);
    printf("  Max output channels: %f\n", deviceInfo->defaultSampleRate);
  }

  int device = 0;
  // 输入参数
  PaStreamParameters inputParameters;
  // 输出参数
  PaStreamParameters outputParameters;

  memset(&inputParameters, 0, sizeof(inputParameters));
  inputParameters.device = device;
  inputParameters.channelCount = 1;
  inputParameters.hostApiSpecificStreamInfo = NULL;
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(device)->defaultLowInputLatency;

  memset(&outputParameters, 0, sizeof(outputParameters));
  outputParameters.device = device;
  outputParameters.channelCount = 1; // 改为立体声输出
  outputParameters.hostApiSpecificStreamInfo = NULL;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(device)->defaultLowOutputLatency;

  PaStream *stream;

  err = Pa_OpenStream(&stream, &inputParameters, &outputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER, paNoFlag, patestCallback, &userData);
  checkError(err);
  // 开始流
  err = Pa_StartStream(stream);
  printf("Stream started\n");
  checkError(err);
  Pa_Sleep(10 * 1000);
  // 停止流
  err = Pa_StopStream(stream);
  checkError(err);
  // 关闭流
  err = Pa_CloseStream(stream);
  checkError(err);
  err = Pa_Terminate();
  checkError(err);
  // 清理 rnnoise 资源
  rnnoise_destroy(userData.st);
  return EXIT_SUCCESS;
}