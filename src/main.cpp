#include <cstdio>
#include "dsp/osc_sine.h"

int main() {
  constexpr float sampleRate = 48000.0f;
  constexpr int blockSize = 64;
  float freq[blockSize], out[blockSize];
  for (int i = 0; i < blockSize; ++i) freq[i] = 440.0f;

  void* osc = osc_sine_create(sampleRate);
  osc_sine_process(osc, freq, out);

  for (int i = 0; i < blockSize; ++i)
    printf("%f\n", out[i]);

  osc_sine_destroy(osc);
  return 0;
}