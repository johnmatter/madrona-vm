#include "dsp/threshold.h"
#include "MLDSPOps.h" // For kFloatsPerDSPVector
#include "dsp/validation.h"
namespace madronavm::dsp {
Threshold::Threshold(float sampleRate) : DSPModule(sampleRate) {
}
void Threshold::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
    if (!validate_ports("Threshold", num_inputs, inputs, {0, 1}, num_outputs, 1)) return;
    const float* signal = inputs[0];      // Input signal
    const float* threshold = inputs[1];   // Threshold value
    float* out = outputs[0];              // Output (0.0 or 1.0)
    // Simple threshold comparison: output 1.0 if signal > threshold, 0.0 otherwise
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        out[i] = (signal[i] > threshold[i]) ? 1.0f : 0.0f;
    }
}
} // namespace madronavm::dsp 