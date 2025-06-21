#include "dsp/lopass.h"
#include "MLDSPFilters.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
struct Lopass::impl {
    ml::Lopass mFilter;
};
Lopass::Lopass(float sampleRate) : DSPModule(sampleRate) {
    pImpl = new impl();
}
Lopass::~Lopass() {
    delete pImpl;
}
void Lopass::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
    if (!validate_ports("Lopass", num_inputs, inputs, {0, 1, 2}, num_outputs, 1)) return;
    ml::DSPVector vIn(inputs[0]);
    ml::DSPVector vCutoff(inputs[1]);
    ml::DSPVector vQ(inputs[2]);
    // Convert frequency vector to omega vector (frequency / sample_rate)
    // Clamp to prevent instability above Nyquist
    ml::DSPVector vOmega = ml::clamp(vCutoff / mSampleRate, ml::DSPVector(0.0f), ml::DSPVector(0.49f));
    // Convert Q vector to damping parameter vector k = 1/Q
    // Clamp to prevent instability (min Q = 0.1, max Q = 100)
    ml::DSPVector vK = ml::DSPVector(1.0f) / ml::clamp(vQ, ml::DSPVector(0.1f), ml::DSPVector(100.0f));
    // Process with per-sample cutoff frequency and resonance modulation
    ml::DSPVector v = pImpl->mFilter(vIn, vOmega, vK);
    for(int i=0; i<kFloatsPerDSPVector; ++i) {
        outputs[0][i] = v[i];
    }
}
} // namespace madronavm::dsp 