#include "dsp/float.h"
#include "MLDSPMath.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
Float::Float(float sampleRate) : DSPModule(sampleRate), mValue(0.0f) {}
void Float::process(const float **inputs, int num_inputs, float **outputs, int num_outputs) {
    if (!validate_ports("Float", num_inputs, inputs, {}, num_outputs, 1)) return;
    if (num_inputs > 0 && inputs[0]) {
        mValue = inputs[0][0];
    }
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        outputs[0][i] = mValue;
    }
}
} // namespace madronavm::dsp 