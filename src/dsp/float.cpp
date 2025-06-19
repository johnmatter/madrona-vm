#include "dsp/float.h"
#include "MLDSPScalarMath.h"
using namespace ml;
Float::Float(float sampleRate) : DSPModule(sampleRate), mValue(0.0f) {}
void Float::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  // The "value" port is the first input. If it's connected, update our value.
  if (num_inputs > 0 && inputs[0]) {
    mValue = inputs[0][0];
  }
  if (num_outputs > 0 && outputs[0]) {
    float* out = outputs[0];
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        out[i] = mValue;
    }
  }
} 