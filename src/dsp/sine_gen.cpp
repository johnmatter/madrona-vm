#include "dsp/sine_gen.h"
#include "MLDSPGens.h"
#include "dsp/validation.h"
namespace madronavm::dsp {
  struct SineGen::impl {
    ml::SineGen mOsc;
  };
  SineGen::SineGen(float sampleRate) : DSPModule(sampleRate) {
    pImpl = new impl();
  }
  SineGen::~SineGen() {
    delete pImpl;
  }
  void SineGen::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
    if (!validate_ports("SineGen", num_inputs, inputs, {0}, num_outputs, 1)) return;
    const float sr = mSampleRate;
    // get frequency as cycles/sample, now supporting time-varying input
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
