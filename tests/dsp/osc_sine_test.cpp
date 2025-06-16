#include "dsp/osc_sine.h"
#include "catch.hpp"

TEST_CASE("Sine Oscillator Test", "[dsp]") {
  constexpr float sampleRate = 48000.0f;
  constexpr int blockSize = 64;
  float freq[blockSize];
  float out[blockSize];

  for (int i = 0; i < blockSize; ++i) {
    freq[i] = 440.0f;
  }

  void* osc = osc_sine_create(sampleRate);
  REQUIRE(osc != nullptr);

  SECTION("Process a block") {
    osc_sine_process(osc, freq, out);

    for (int i = 0; i < blockSize; ++i) {
      INFO("Sample " << i << ": " << out[i]);
      REQUIRE(out[i] >= -1.0f);
      REQUIRE(out[i] <= 1.0f);
    }
  }

  osc_sine_destroy(osc);
} 