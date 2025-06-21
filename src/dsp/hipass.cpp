#include "dsp/hipass.h"
#include "MLDSPFilters.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
struct Hipass::impl {
    ml::Hipass mFilter;
};
Hipass::Hipass(float sampleRate) : DSPModule(sampleRate) {
    pImpl = new impl();
}
Hipass::~Hipass() {
    delete pImpl;
}
void Hipass::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
    if (!validate_ports("Hipass", num_inputs, inputs, {0, 1, 2}, num_outputs, 1)) return;
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
    // Since ml::Hipass doesn't have a per-sample operator, we'll calculate coefficients per-sample
    ml::DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n) {
        // Calculate coefficients for this sample
        const float omega = vOmega[n];
        const float k = vK[n];
        pImpl->mFilter.mCoeffs = ml::Hipass::coeffs(omega, k);
        // Process single sample
        ml::DSPVector sampleIn(vIn[n]);
        ml::DSPVector sampleOut = pImpl->mFilter(sampleIn);
        vy[n] = sampleOut[0];
    }
    for(int i=0; i<kFloatsPerDSPVector; ++i) {
        outputs[0][i] = vy[i];
    }
}
} // namespace madronavm::dsp 