#include "dsp/mul.h"
#include "MLDSPOps.h"
using namespace ml;
using namespace madronavm;
Mul::Mul(float sampleRate) : DSPModule(sampleRate) {}
void Mul::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
    if (num_inputs < 2 || num_outputs < 1) return;
    const float* in1 = inputs[0];
    const float* in2 = inputs[1];
    float* out = outputs[0];
    // Work directly with the buffers instead of constructing DSPVector objects
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        out[i] = in1[i] * in2[i];
    }
} 