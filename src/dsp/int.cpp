#include "dsp/int.h"
#include "MLDSPScalarMath.h"
#include "dsp/validation.h"
using namespace ml;
using namespace madronavm;
Int::Int(float sampleRate) : DSPModule(sampleRate), mValue(0) {}
void Int::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  if (!dsp::validate_ports("Int", num_inputs, inputs, {}, num_outputs, 1)) return;
  // The "value" port at index 0 is optional. If it's connected, update our value.
  if (inputs[0]) {
    mValue = static_cast<int>(inputs[0][0]);
  }
  float* out = outputs[0];
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
      out[i] = static_cast<float>(mValue);
  }
} 