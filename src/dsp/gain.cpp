#include "dsp/gain.h"
#include "MLDSPOps.h"
using namespace ml;
Gain::Gain(float sampleRate) : DSPModule(sampleRate) {}
void Gain::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  if (num_inputs < 2) return; // Gain needs signal and gain inputs
  const float* signalIn = inputs[0];
  const float* gainIn = inputs[1];
  float* signalOut = outputs[0];
  DSPVector signalVec(signalIn);
  DSPVector gainVec(gainIn);
  DSPVector result = signalVec * gainVec;
  // Copy result to the output buffer
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    signalOut[i] = result[i];
  }
} 