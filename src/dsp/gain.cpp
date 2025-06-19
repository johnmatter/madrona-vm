#include "dsp/gain.h"
#include "MLDSPOps.h"
#include "dsp/validation.h"
using namespace ml;
using namespace madronavm;
Gain::Gain(float sampleRate) : DSPModule(sampleRate) {}
void Gain::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  if (!dsp::validate_ports("Gain", num_inputs, inputs, {0}, num_outputs, 1)) return;
  const float* signalIn = inputs[0];
  float* signalOut = outputs[0];
  // The 'gain' input at index 1 is optional.
  if (inputs[1]) {
    const float* gainIn = inputs[1];
    // Work directly with the buffers
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        signalOut[i] = signalIn[i] * gainIn[i];
    }
  } else {
    // If gain input is not connected, act as a pass-through (gain of 1.0)
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        signalOut[i] = signalIn[i];
    }
  }
} 