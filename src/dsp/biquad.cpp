#include "dsp/biquad.h"
#include "MLDSPFilters.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
struct Biquad::Impl {
  ml::Lopass mFilter;
  Impl() {
    mFilter.clear();
  }
};
Biquad::Biquad(float sampleRate) : DSPModule(sampleRate), pImpl(std::make_unique<Impl>()) {}
Biquad::~Biquad() = default;
void Biquad::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  if (!validate_ports("Biquad", num_inputs, inputs, {0, 1, 2}, num_outputs, 1)) return;
  const float* signal = inputs[0];
  const float cutoff = inputs[1][0];
  const float resonance = inputs[2][0];
  const float sr = mSampleRate;
  // convert cutoff frequency to omega (normalized frequency)
  ml::DSPVector vOmega(cutoff / sr);
  // convert resonance to k (damping parameter = 1/Q)
  // resonance input is expected to be Q-like (higher = more resonant)
  // k = 1/Q, so we need to invert and clamp to prevent instability
  float k = ml::max(1.0f / ml::max(resonance, 0.1f), 0.01f);
  ml::DSPVector vK(k);
  // load input signal
  ml::DSPVector vSignal(signal);
  // process one vector of samples
  ml::DSPVector v = pImpl->mFilter(vSignal, vOmega, vK);
  // copy to output
  for(int i=0; i<kFloatsPerDSPVector; ++i) {
    outputs[0][i] = v[i];
  }
}
} // namespace madronavm::dsp 