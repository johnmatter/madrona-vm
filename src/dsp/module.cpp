#include "dsp/module.h"

DSPModule::DSPModule(float sampleRate) : mSampleRate(sampleRate) {}

void DSPModule::setSampleRate(float sampleRate) {
  mSampleRate = sampleRate;
}

void DSPModule::configure(size_t numInputs, size_t numOutputs) {
  // Default implementation does nothing.
  (void)numInputs;
  (void)numOutputs;
} 