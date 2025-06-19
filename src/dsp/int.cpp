#include "dsp/int.h"
#include "MLDSPMath.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
Int::Int(float sampleRate) : DSPModule(sampleRate), mValue(0) {}
void Int::process(const float **inputs, int num_inputs, float **outputs, int num_outputs) {
    if (!validate_ports("Int", num_inputs, inputs, {}, num_outputs, 1)) return;
    if (num_inputs > 0 && inputs[0]) {
        mValue = static_cast<int>(inputs[0][0]);
    }
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        outputs[0][i] = static_cast<float>(mValue);
    }
}
} // namespace madronavm::dsp 