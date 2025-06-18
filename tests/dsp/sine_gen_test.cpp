#include "catch.hpp"
#include "dsp/sine_gen.h"
#include <vector>
constexpr int sampleRate = 44100;
constexpr int bufferSize = 256;
// Ensure output is normalized
TEST_CASE("SineGen normalized", "[dsp]") {
    auto sineGen = SineGen(sampleRate);
  std::vector<float> freqBuffer(bufferSize, 1.0f);
  std::vector<float> outputBuffer(bufferSize, 0.0f);
  const float* inputs[] = { freqBuffer.data() };
  float* outputs[] = { outputBuffer.data(), outputBuffer.data() };
  sineGen.process(inputs, outputs);
  for (int i = 0; i < bufferSize; ++i) {
    REQUIRE(outputBuffer[i] >= -1.0f);
    REQUIRE(outputBuffer[i] <= 1.0f);
  }
}
// Ensure at least one output sample is nonzero
TEST_CASE("SineGen nonzero", "[dsp]") {
  auto sineGen = SineGen(sampleRate);
  std::vector<float> freqBuffer(bufferSize, 440.0f);
  std::vector<float> outputBuffer(bufferSize, 0.0f);
  const float* inputs[] = { freqBuffer.data() };
  float* outputs[] = { outputBuffer.data() };
  SECTION("Process") {
    sineGen.process(inputs, outputs);
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