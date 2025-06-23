#include "dsp/saw_gen.h"
#include "MLDSPGens.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
struct SawGen::Impl {
  ml::SawGen mOsc;
  Impl() {
    mOsc.clear();
  }
};
SawGen::SawGen(float sampleRate) : DSPModule(sampleRate), pImpl(std::make_unique<Impl>()) {}
SawGen::~SawGen() = default;
void SawGen::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  if (!validate_ports("SawGen", num_inputs, inputs, {0}, num_outputs, 1)) return;
  const float sr = mSampleRate;
  // get frequency as cycles/sample, following SineGen pattern
  ml::DSPVector vFreq(inputs[0]);
  vFreq /= sr;
  // process one vector of samples
  ml::DSPVector v = pImpl->mOsc(vFreq);
  // copy to output
  for(int i=0; i<kFloatsPerDSPVector; ++i) {
    outputs[0][i] = v[i];
  }
}
} // namespace madronavm::dsp 