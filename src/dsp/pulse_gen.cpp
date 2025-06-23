#include "dsp/pulse_gen.h"
#include "MLDSPGens.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
struct PulseGen::Impl {
  ml::PulseGen mOsc;
  Impl() {
    mOsc.clear();
  }
};
PulseGen::PulseGen(float sampleRate) : DSPModule(sampleRate), pImpl(std::make_unique<Impl>()) {}
PulseGen::~PulseGen() = default;
void PulseGen::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  if (!validate_ports("PulseGen", num_inputs, inputs, {0, 1}, num_outputs, 1)) return;
  const float freq = inputs[0][0];
  const float width = inputs[1][0];
  const float sr = mSampleRate;
  // get frequency as cycles/sample  
  ml::DSPVector vFreq(freq / sr);
  // pulse width from 0-1 (0.5 = square wave)
  ml::DSPVector vWidth(width);
  // process one vector of samples
  ml::DSPVector v = pImpl->mOsc(vFreq, vWidth);
  // copy to output
  for(int i=0; i<kFloatsPerDSPVector; ++i) {
    outputs[0][i] = v[i];
  }
}
} // namespace madronavm::dsp 