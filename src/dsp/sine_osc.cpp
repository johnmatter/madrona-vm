// Sine oscillator using madronalib

#include "dsp/sine_osc.h"
#include "MLDSPGens.h"

using namespace ml;

SineOsc::SineOsc(float sampleRate) : DSPModule(sampleRate) {
  mOsc.clear();
}

void SineOsc::process(const float** inputs, float** outputs) {
  const float* freqIn = inputs[0]; // Control-rate frequency
  float* out = outputs[0];         // Audio-rate output

  // Convert frequency to cycles per sample for madronalib
  DSPVector freq(freqIn[0] / mSampleRate);

  // Process one vector
  DSPVector result = mOsc(freq);

  // Copy result to the output buffer
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    out[i] = result[i];
  }
}
