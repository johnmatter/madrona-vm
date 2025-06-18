#include "catch.hpp"
#include "dsp/sine_gen.h"
#include <vector>
TEST_CASE("SineGen", "[dsp]") {
  const int sampleRate = 44100;
  auto sineGen = SineGen(sampleRate);
  const int bufferSize = 256;
  std::vector<float> freqBuffer(bufferSize, 440.0f);
  std::vector<float> outputBuffer(bufferSize, 0.0f);
  const float* inputs[] = { freqBuffer.data() };
  float* outputs[] = { outputBuffer.data() };
  SECTION("Process") {
    sineGen.process(inputs, outputs);
    // Basic check: ensure output is not all zeros
    bool outputIsNonZero = false;
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
      if (outputBuffer[i] != 0.0f) {
        outputIsNonZero = true;
        break;
      }
    }
    REQUIRE(outputIsNonZero);
  }
} 