#include "dsp/adsr.h"
#include "MLDSPOps.h" // For kFloatsPerDSPVector
#include <algorithm> // for std::max

ADSR::ADSR(float sampleRate) : DSPModule(sampleRate) {
    mADSR.clear();
}

void ADSR::configure(size_t numInputs, size_t numOutputs) {
    mADSR.coeffs = ml::ADSR::calcCoeffs(0.1f, 0.1f, 0.5f, 0.2f, mSampleRate);
}

void ADSR::process(const float** inputs, float** outputs) {
    const float* gateIn = inputs[0];
    const float* attackIn = inputs[1];
    const float* decayIn = inputs[2];
    const float* sustainIn = inputs[3];
    const float* releaseIn = inputs[4];
    float* out = outputs[0];

    // Update coeffs only if they change - for now, we do it every block
    // A more optimized version would check for changes.
    mADSR.coeffs = ml::ADSR::calcCoeffs(attackIn[0], decayIn[0], sustainIn[0], releaseIn[0], mSampleRate);

    // The ml::ADSR object can process a full vector at once.
    ml::DSPVector gateVec(gateIn);
    ml::DSPVector result = mADSR(gateVec);

    // Copy result to the output buffer
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
        out[i] = result[i];
    }
} 