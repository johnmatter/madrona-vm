#include "dsp/float.h"
#include "MLDSPScalarMath.h"
#include "dsp/validation.h"
using namespace ml;
using namespace madronavm;
Float::Float(float sampleRate) : DSPModule(sampleRate), mValue(0.0f) {}
void Float::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  if (!dsp::validate_ports("Float", num_inputs, inputs, {}, num_outputs, 1)) return;
  // The "value" port at index 0 is optional. If it's connected, update our value.
  if (inputs[0]) {
    mValue = inputs[0][0];
  }
  float* out = outputs[0];
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
      out[i] = mValue;
  }
} 