#include "dsp/bandpass.h"
#include "MLDSPFilters.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
struct Bandpass::impl {
    ml::Bandpass mFilter;
};
Bandpass::Bandpass(float sampleRate) : DSPModule(sampleRate) {
    pImpl = new impl();
}
Bandpass::~Bandpass() {
    delete pImpl;
}
void Bandpass::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
    if (!validate_ports("Bandpass", num_inputs, inputs, {0, 1, 2}, num_outputs, 1)) return;
    ml::DSPVector vIn(inputs[0]);
    ml::DSPVector vCutoff(inputs[1]);
    ml::DSPVector vQ(inputs[2]);
    // Convert frequency vector to omega vector (frequency / sample_rate)
    ml::DSPVector vOmega = vCutoff / mSampleRate;
    // Convert Q vector to damping parameter vector k = 1/Q
    // Clamp to prevent instability (min Q = 0.1, max Q = 100)
    ml::DSPVector vK = ml::DSPVector(1.0f) / ml::clamp(vQ, ml::DSPVector(0.1f), ml::DSPVector(100.0f));
    // Process with per-sample cutoff frequency and Q modulation
    // Since ml::Bandpass doesn't have a per-sample operator, we'll calculate coefficients per-sample
    ml::DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n) {
        // Calculate coefficients for this sample
        const float omega = vOmega[n];
        const float k = vK[n];
        pImpl->mFilter.mCoeffs = ml::Bandpass::coeffs(omega, k);
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