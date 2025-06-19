#include "dsp/gain.h"
#include "catch.hpp"
#include <vector>
TEST_CASE("Gain Test", "[dsp]") {
  constexpr float sampleRate = 48000.0f;
  constexpr int blockSize = 64; // Should match kFloatsPerDSPVector
  std::vector<float> signalIn(blockSize);
  std::vector<float> gainIn(blockSize);
  std::vector<float> out(blockSize);
  // Initialize with a constant signal and gain
  for (int i = 0; i < blockSize; ++i) {
    signalIn[i] = 0.5f; // A constant signal
    gainIn[i] = 0.5f;   // A constant gain
  }
  Gain gainModule(sampleRate);
  // vector.data() returns a pointer to the first element of the vector, so this should be persistent throughout the test sections.
  // That is, we can modify the vectors in the test sections, and the modules will still read from the same locations by way of the `inputs` and `outputs` arrays which contain pointers to the vector data.
  const float* inputs[] = {signalIn.data(), gainIn.data()};
  float* outputs[] = {out.data()};
  SECTION("Process a block with 0.5 gain") {
    gainModule.process(inputs, outputs);
    for (int i = 0; i < blockSize; ++i) {
      INFO("Sample " << i << ": " << out[i]);
      REQUIRE(out[i] == Approx(0.25f));
    }
  }
  SECTION("Process a block with 0.0 gain") {
    for (int i = 0; i < blockSize; ++i) {
      gainIn[i] = 0.0f;
    }
    gainModule.process(inputs, outputs);
    for (int i = 0; i < blockSize; ++i) {
      INFO("Sample " << i << ": " << out[i]);
      REQUIRE(out[i] == Approx(0.0f));
    }
  }
  SECTION("Process a block with 1.0 gain") {
    for (int i = 0; i < blockSize; ++i) {
      gainIn[i] = 1.0f;
    }
    gainModule.process(inputs, outputs);
    for (int i = 0; i < blockSize; ++i) {
      INFO("Sample " << i << ": " << out[i]);
      REQUIRE(out[i] == Approx(0.5f));
    }
  }
  SECTION("Process a block with varying gain") {
    // Create a ramp from 0.0 to 1.0 for the gain
    for (int i = 0; i < blockSize; ++i) {
      gainIn[i] = static_cast<float>(i) / (blockSize - 1);
    }
    gainModule.process(inputs, outputs);
    for (int i = 0; i < blockSize; ++i) {
      float expected = 0.5f * (static_cast<float>(i) / (blockSize - 1));
      INFO("Sample " << i << ": " << out[i] << " -- Expected: " << expected);
      REQUIRE(out[i] == Approx(expected));
    }
  }
}
TEST_CASE("Gain DSP Module", "[dsp][gain]") {
    Gain gain_module(48000.0f);
    SECTION("Applies gain correctly") {
        const int block_size = 64;
        std::vector<float> audio_in(block_size, 0.5f); // Input signal at 0.5
        std::vector<float> gain_in(block_size, 0.5f);  // Gain of 0.5
        std::vector<float> out(block_size);
        const float* inputs[] = { audio_in.data(), gain_in.data() };
        float* outputs[] = { out.data() };
        gain_module.process(inputs, outputs);
        // Expected output is 0.5 (audio) * 0.5 (gain) = 0.25
        for (int i = 0; i < block_size; ++i) {
            REQUIRE(out[i] == Approx(0.25f));
        }
    }
    SECTION("Passes audio through with gain of 1.0") {
        const int block_size = 64;
        std::vector<float> audio_in(block_size);
        for(int i=0; i<block_size; ++i) audio_in[i] = static_cast<float>(i) / block_size;
        std::vector<float> gain_in(block_size, 1.0f); // Gain of 1.0
        std::vector<float> out(block_size);
        const float* inputs[] = { audio_in.data(), gain_in.data() };
        float* outputs[] = { out.data() };
        gain_module.process(inputs, outputs);
        for (int i = 0; i < block_size; ++i) {
            REQUIRE(out[i] == Approx(audio_in[i]));
        }
    }
    SECTION("Produces silence with gain of 0.0") {
        const int block_size = 64;
        std::vector<float> audio_in(block_size, 0.8f);
        std::vector<float> gain_in(block_size, 0.0f); // Gain of 0.0
        std::vector<float> out(block_size, 999.f); // Pre-fill with non-zero values
        const float* inputs[] = { audio_in.data(), gain_in.data() };
        float* outputs[] = { out.data() };
        gain_module.process(inputs, outputs);
        for (int i = 0; i < block_size; ++i) {
            REQUIRE(out[i] == Approx(0.0f));
        }
    }
} 