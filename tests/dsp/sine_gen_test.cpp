#include "catch.hpp"
#include "dsp/sine_gen.h"
#include "MLDSPGens.h" // For kFloatsPerDSPVector
#include <vector>
#include <cmath>
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
TEST_CASE("SineGen DSP Module", "[dsp][sine_gen]") {
    constexpr float sampleRate = 48000.0f;
    SineGen sine_gen(sampleRate);
    SECTION("Produces non-zero output") {
        const int block_sizen = 64;
        // A frequency that is not a multiple of the buffer size / sample rate,
        // to avoid producing all zeros if the phase happens to align with zero crossings.
        std::vector<float> freq_in(block_sizen, 441.0f);
        std::vector<float> out(block_sizen, 0.0f);
        const float* inputs[] = { freq_in.data() };
        float* outputs[] = { out.data() };
        // Process a few blocks to ensure the oscillator has started
        for (int i=0; i<4; ++i) {
            sine_gen.process(inputs, outputs);
        }
        bool all_zero = true;
        for (int i = 0; i < block_sizen; ++i) {
            if (out[i] != 0.0f) {
                all_zero = false;
                break;
            }
        }
        REQUIRE_FALSE(all_zero);
    }
} 