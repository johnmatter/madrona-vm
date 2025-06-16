// Sine oscillator using madronalib

#include "dsp/osc_sine.h"
#include "MLDSPGens.h"

using namespace ml;

struct SineOscInstance {
  SineGen osc;
  float sampleRate;
};

void* osc_sine_create(float sampleRate) {
  auto* inst = new SineOscInstance();
  inst->sampleRate = sampleRate;
  inst->osc.clear();
  return inst;
}

void osc_sine_process(void* instance, const float* freqIn, float* out) {
  auto* inst = static_cast<SineOscInstance*>(instance);
  
  // Convert frequency to cycles per sample
  DSPVector freq(freqIn[0] / inst->sampleRate);
  
  // Process one vector
  DSPVector result = inst->osc(freq);
  
  // Copy result to output buffer
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    out[i] = result[i];
  }
}

void osc_sine_destroy(void* instance) {
  auto* inst = static_cast<SineOscInstance*>(instance);
  delete inst;
}
