#include "catch.hpp"
#include "dsp/biquad.h"
#include "MLDSPGens.h"
#include <cmath>
using namespace madronavm::dsp;
TEST_CASE("[biquad] Basic filtering functionality", "[biquad]") {
  const float sampleRate = 48000.0f;
  Biquad biquad(sampleRate);
  // Prepare input and output buffers
  float signalBuffer[kFloatsPerDSPVector];
  float cutoffBuffer[kFloatsPerDSPVector];
  float resonanceBuffer[kFloatsPerDSPVector];
  float outputBuffer[kFloatsPerDSPVector];
  // Create a simple test signal (ramp)
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    signalBuffer[i] = (float)i / kFloatsPerDSPVector;
  }
  // Set filter parameters: 1kHz cutoff, moderate Q
  std::fill(cutoffBuffer, cutoffBuffer + kFloatsPerDSPVector, 1000.0f);
  std::fill(resonanceBuffer, resonanceBuffer + kFloatsPerDSPVector, 2.0f);
  // Set up input and output pointers
  const float* inputs[3] = {signalBuffer, cutoffBuffer, resonanceBuffer};
  float* outputs[1] = {outputBuffer};
  // Process one vector
  biquad.process(inputs, 3, outputs, 1);
  // Check that we got some output (should be filtered version of input)
  bool hasOutput = false;
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    if (outputBuffer[i] != 0.0f) {
      hasOutput = true;
      break;
    }
  }
  REQUIRE(hasOutput);
  // Output should be in reasonable range
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    REQUIRE(outputBuffer[i] >= -2.0f);
    REQUIRE(outputBuffer[i] <= 2.0f);
  }
}
TEST_CASE("[biquad] DC blocking behavior", "[biquad]") {
  const float sampleRate = 48000.0f;
  Biquad biquad(sampleRate);
  float signalBuffer[kFloatsPerDSPVector];
  float cutoffBuffer[kFloatsPerDSPVector];
  float resonanceBuffer[kFloatsPerDSPVector];
  float outputBuffer[kFloatsPerDSPVector];
  // DC signal (all ones)
  std::fill(signalBuffer, signalBuffer + kFloatsPerDSPVector, 1.0f);
  // Very low cutoff frequency (should attenuate DC significantly)
  std::fill(cutoffBuffer, cutoffBuffer + kFloatsPerDSPVector, 1.0f);  // 1 Hz
  std::fill(resonanceBuffer, resonanceBuffer + kFloatsPerDSPVector, 1.0f);
  const float* inputs[3] = {signalBuffer, cutoffBuffer, resonanceBuffer};
  float* outputs[1] = {outputBuffer};
  // Process multiple vectors to let filter settle
  for (int v = 0; v < 10; ++v) {
    biquad.process(inputs, 3, outputs, 1);
  }
  // After settling, output should be significantly attenuated compared to input
  float avgOutput = 0.0f;
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    avgOutput += outputBuffer[i];
  }
  avgOutput /= kFloatsPerDSPVector;
  // Should be much less than the input DC level of 1.0
  REQUIRE(std::abs(avgOutput) < 0.5f);
}
TEST_CASE("[biquad] Resonance parameter effect", "[biquad]") {
  const float sampleRate = 48000.0f;
  float signalBuffer[kFloatsPerDSPVector];
  float cutoffBuffer[kFloatsPerDSPVector];
  float resonanceBuffer[kFloatsPerDSPVector];
  float outputBuffer1[kFloatsPerDSPVector];
  float outputBuffer2[kFloatsPerDSPVector];
  // White noise-like signal for testing resonance
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    signalBuffer[i] = (float)(i % 3 - 1) * 0.1f;  // Simple pseudo-random
  }
  std::fill(cutoffBuffer, cutoffBuffer + kFloatsPerDSPVector, 2000.0f);
  const float* inputs[3] = {signalBuffer, cutoffBuffer, resonanceBuffer};
  // Test with low resonance (Q = 0.5)
  Biquad biquad1(sampleRate);
  std::fill(resonanceBuffer, resonanceBuffer + kFloatsPerDSPVector, 0.5f);
  float* outputs1[1] = {outputBuffer1};
  biquad1.process(inputs, 3, outputs1, 1);
  // Test with high resonance (Q = 10)
  Biquad biquad2(sampleRate);
  std::fill(resonanceBuffer, resonanceBuffer + kFloatsPerDSPVector, 10.0f);
  float* outputs2[1] = {outputBuffer2};
  biquad2.process(inputs, 3, outputs2, 1);
  // Both should produce valid output
  bool hasOutput1 = false, hasOutput2 = false;
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    if (outputBuffer1[i] != 0.0f) hasOutput1 = true;
    if (outputBuffer2[i] != 0.0f) hasOutput2 = true;
  }
  REQUIRE(hasOutput1);
  REQUIRE(hasOutput2);
  // High resonance filter should remain stable (no NaN or excessive values)
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    REQUIRE(std::isfinite(outputBuffer2[i]));
    REQUIRE(std::abs(outputBuffer2[i]) < 10.0f);  // Should not blow up
  }
}
TEST_CASE("[biquad] Invalid port configuration", "[biquad]") {
  const float sampleRate = 48000.0f;
  Biquad biquad(sampleRate);
  float buffer[kFloatsPerDSPVector];
  const float* inputs[3] = {buffer, buffer, buffer};
  float* outputs[1] = {buffer};
  // Test with wrong number of inputs
  biquad.process(inputs, 2, outputs, 1);  // Missing resonance input
  biquad.process(inputs, 4, outputs, 1);  // Too many inputs
  // Test with wrong number of outputs
  biquad.process(inputs, 3, outputs, 0);  // No outputs
  biquad.process(inputs, 3, outputs, 2);  // Too many outputs
  // These should not crash - validation should handle them gracefully
  REQUIRE(true);  // If we get here without crashing, the test passes
} 