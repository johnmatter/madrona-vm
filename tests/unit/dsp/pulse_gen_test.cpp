#include "catch.hpp"
#include "dsp/pulse_gen.h"
#include "MLDSPGens.h"
using namespace madronavm::dsp;
TEST_CASE("[pulse_gen] Basic functionality", "[pulse_gen]") {
  const float sampleRate = 48000.0f;
  PulseGen pulseGen(sampleRate);
  // Prepare input and output buffers
  float freqBuffer[kFloatsPerDSPVector];
  float widthBuffer[kFloatsPerDSPVector];
  float outputBuffer[kFloatsPerDSPVector];
  // Fill frequency buffer with 440 Hz and width with 0.5 (square wave)
  std::fill(freqBuffer, freqBuffer + kFloatsPerDSPVector, 440.0f);
  std::fill(widthBuffer, widthBuffer + kFloatsPerDSPVector, 0.5f);
  // Set up input and output pointers
  const float* inputs[2] = {freqBuffer, widthBuffer};
  float* outputs[1] = {outputBuffer};
  // Process one vector
  pulseGen.process(inputs, 2, outputs, 1);
  // Check that we got some output (non-zero)
  bool hasOutput = false;
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    if (outputBuffer[i] != 0.0f) {
      hasOutput = true;
      break;
    }
  }
  REQUIRE(hasOutput);
  // Check that output is in reasonable range for a pulse wave (-1 to 1)
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    REQUIRE(outputBuffer[i] >= -1.5f);  // Allow some headroom for antialiasing
    REQUIRE(outputBuffer[i] <= 1.5f);
  }
}
TEST_CASE("[pulse_gen] Different pulse widths", "[pulse_gen]") {
  const float sampleRate = 48000.0f;
  PulseGen pulseGen(sampleRate);
  float freqBuffer[kFloatsPerDSPVector];
  float widthBuffer[kFloatsPerDSPVector];
  float outputBuffer1[kFloatsPerDSPVector];
  float outputBuffer2[kFloatsPerDSPVector];
  std::fill(freqBuffer, freqBuffer + kFloatsPerDSPVector, 100.0f);  // Low freq for easier analysis
  const float* inputs[2] = {freqBuffer, widthBuffer};
  float* outputs1[1] = {outputBuffer1};
  float* outputs2[1] = {outputBuffer2};
  // Test with width = 0.2 (narrow pulse)
  std::fill(widthBuffer, widthBuffer + kFloatsPerDSPVector, 0.2f);
  pulseGen.process(inputs, 2, outputs1, 1);
  // Reset oscillator state for second test
  PulseGen pulseGen2(sampleRate);
  // Test with width = 0.8 (wide pulse)
  std::fill(widthBuffer, widthBuffer + kFloatsPerDSPVector, 0.8f);
  pulseGen2.process(inputs, 2, outputs2, 1);
  // Both should have valid output
  bool hasOutput1 = false, hasOutput2 = false;
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    if (outputBuffer1[i] != 0.0f) hasOutput1 = true;
    if (outputBuffer2[i] != 0.0f) hasOutput2 = true;
  }
  REQUIRE(hasOutput1);
  REQUIRE(hasOutput2);
}
TEST_CASE("[pulse_gen] Zero frequency produces DC", "[pulse_gen]") {
  const float sampleRate = 48000.0f;
  PulseGen pulseGen(sampleRate);
  float freqBuffer[kFloatsPerDSPVector];
  float widthBuffer[kFloatsPerDSPVector];
  float outputBuffer[kFloatsPerDSPVector];
  // Zero frequency, any width
  std::fill(freqBuffer, freqBuffer + kFloatsPerDSPVector, 0.0f);
  std::fill(widthBuffer, widthBuffer + kFloatsPerDSPVector, 0.5f);
  const float* inputs[2] = {freqBuffer, widthBuffer};
  float* outputs[1] = {outputBuffer};
  pulseGen.process(inputs, 2, outputs, 1);
  // With zero frequency, output should be relatively stable (near DC)
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
TEST_CASE("[pulse_gen] Invalid port configuration", "[pulse_gen]") {
  const float sampleRate = 48000.0f;
  PulseGen pulseGen(sampleRate);
  float buffer[kFloatsPerDSPVector];
  const float* inputs[2] = {buffer, buffer};
  float* outputs[1] = {buffer};
  // Test with wrong number of inputs
  pulseGen.process(inputs, 1, outputs, 1);  // Missing width input
  pulseGen.process(inputs, 3, outputs, 1);  // Too many inputs
  // Test with wrong number of outputs
  pulseGen.process(inputs, 2, outputs, 0);  // No outputs
  pulseGen.process(inputs, 2, outputs, 2);  // Too many outputs
  // These should not crash - validation should handle them gracefully
  REQUIRE(true);  // If we get here without crashing, the test passes
} 