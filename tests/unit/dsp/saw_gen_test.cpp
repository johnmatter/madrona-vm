#include "catch.hpp"
#include "dsp/saw_gen.h"
#include "MLDSPGens.h"
using namespace madronavm::dsp;
TEST_CASE("[saw_gen] Basic functionality", "[saw_gen]") {
  const float sampleRate = 48000.0f;
  SawGen sawGen(sampleRate);
  // Prepare input and output buffers
  float freqBuffer[kFloatsPerDSPVector];
  float outputBuffer[kFloatsPerDSPVector];
  // Fill frequency buffer with 440 Hz
  std::fill(freqBuffer, freqBuffer + kFloatsPerDSPVector, 440.0f);
  // Set up input and output pointers
  const float* inputs[1] = {freqBuffer};
  float* outputs[1] = {outputBuffer};
  // Process one vector
  sawGen.process(inputs, 1, outputs, 1);
  // Check that we got some output (non-zero)
  bool hasOutput = false;
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    if (outputBuffer[i] != 0.0f) {
      hasOutput = true;
      break;
    }
  }
  REQUIRE(hasOutput);
  // Check that output is in reasonable range for a sawtooth wave (-1 to 1)
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    REQUIRE(outputBuffer[i] >= -1.5f);  // Allow some headroom for antialiasing
    REQUIRE(outputBuffer[i] <= 1.5f);
  }
}
TEST_CASE("[saw_gen] Zero frequency produces DC", "[saw_gen]") {
  const float sampleRate = 48000.0f;
  SawGen sawGen(sampleRate);
  float freqBuffer[kFloatsPerDSPVector];
  float outputBuffer[kFloatsPerDSPVector];
  // Zero frequency
  std::fill(freqBuffer, freqBuffer + kFloatsPerDSPVector, 0.0f);
  const float* inputs[1] = {freqBuffer};
  float* outputs[1] = {outputBuffer};
  sawGen.process(inputs, 1, outputs, 1);
  // With zero frequency, output should be relatively stable (near DC)
  // Allow for some initial transient behavior
  bool isRelativelyStable = true;
  float firstSample = outputBuffer[0];
  for (int i = 1; i < kFloatsPerDSPVector; ++i) {
    if (std::abs(outputBuffer[i] - firstSample) > 0.1f) {
      isRelativelyStable = false;
      break;
    }
  }
  REQUIRE(isRelativelyStable);
}
TEST_CASE("[saw_gen] Invalid port configuration", "[saw_gen]") {
  const float sampleRate = 48000.0f;
  SawGen sawGen(sampleRate);
  float buffer[kFloatsPerDSPVector];
  const float* inputs[1] = {buffer};
  float* outputs[1] = {buffer};
  // Test with wrong number of inputs (should handle gracefully)
  sawGen.process(inputs, 0, outputs, 1);  // No inputs
  sawGen.process(inputs, 2, outputs, 1);  // Too many inputs
  // Test with wrong number of outputs
  sawGen.process(inputs, 1, outputs, 0);  // No outputs
  sawGen.process(inputs, 1, outputs, 2);  // Too many outputs
  // These should not crash - validation should handle them gracefully
  REQUIRE(true);  // If we get here without crashing, the test passes
} 