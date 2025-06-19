#include "dsp/mul.h"
#include "MLDSPOps.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
Mul::Mul(float sampleRate) : DSPModule(sampleRate) {}
void Mul::process(const float **inputs, int num_inputs, float **outputs, int num_outputs) {
    if (!validate_ports("Mul", num_inputs, inputs, {0, 1}, num_outputs, 1)) return;
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        outputs[0][i] = inputs[0][i] * inputs[1][i];
    }
}
} // namespace madronavm::dsp 