/* Copyright (c) 2018 Gregor Richards
 * Copyright (c) 2017 Mozilla */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include "rnnoise.h"
#include <portaudio.h>
#include <cstring>

#define FRAME_SIZE 480
#define SAMPLE_RATE 96000 // 修改为PCM文件的实际采样率

// PortAudio 回调函数
static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo *timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData)
{
  FILE *f = (FILE *)userData;
  short *out = (short *)outputBuffer;

  size_t read = fread(out, sizeof(short), framesPerBuffer, f);
  if (read < framesPerBuffer)
  {
    // 文件结束，用0填充剩余部分
    memset(out + read, 0, (framesPerBuffer - read) * sizeof(short));
    return paComplete;
  }
  return paContinue;
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "用法: %s <PCM文件>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "rb");
  if (!f)
  {
    fprintf(stderr, "无法打开文件 %s\n", argv[1]);
    return 1;
  }

  PaStream *stream;
  PaError err;

  // 初始化 PortAudio
  err = Pa_Initialize();
  if (err != paNoError)
    goto error;

  // 打开 PortAudio 流
  err = Pa_OpenDefaultStream(&stream,
                             0,       // 无输入通道
                             1,       // 单声道输出
                             paInt16, // 16位整数输出
                             SAMPLE_RATE,
                             FRAME_SIZE, // 每次回调的帧数
                             paCallback,
                             f); // 传递文件指针作为用户数据
  if (err != paNoError)
    goto error;

  // 启动流
  err = Pa_StartStream(stream);
  if (err != paNoError)
    goto error;

  // 等待流完成
  while (Pa_IsStreamActive(stream) == 1)
  {
    Pa_Sleep(100);
  }

  // 停止并关闭流
  err = Pa_StopStream(stream);
  if (err != paNoError)
    goto error;
  err = Pa_CloseStream(stream);
  if (err != paNoError)
    goto error;

  fclose(f);

  // 终止 PortAudio
  Pa_Terminate();
  return 0;

error:
  if (f)
    fclose(f);
  Pa_Terminate();
  fprintf(stderr, "PortAudio 错误: %s\n", Pa_GetErrorText(err));
  return 1;
}
