#include "dsp/sine_osc.h"
#include "catch.hpp"

TEST_CASE("Sine Oscillator Test", "[dsp]") {
  constexpr float sampleRate = 48000.0f;
  constexpr int blockSize = 64; // Should match kFloatsPerDSPVector
  float freq[blockSize];
  float out[blockSize];

  for (int i = 0; i < blockSize; ++i) {
    freq[i] = 440.0f;
  }

  SineOsc osc(sampleRate);

  SECTION("Output is normalized") {
    const float* inputs[] = {freq};
    float* outputs[] = {out};
    osc.process(inputs, outputs);

    for (int i = 0; i < blockSize; ++i) {
      INFO("Sample " << i << ": " << out[i]);
      REQUIRE(out[i] >= -1.0f && out[i] <= 1.0f);
    }
  }
} 