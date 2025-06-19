// Sine oscillator using madronalib
#include "dsp/sine_gen.h"
#include "MLDSPGens.h"
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
  if (num_inputs < 1) return; // Should always have a frequency input
  const float freq = inputs[0][0];
  const float sr = mSampleRate;
  // get frequency as cycles/sample
  ml::DSPVector vFreq(freq / sr);
  // process one vector of samples
  ml::DSPVector v = pImpl->mOsc(vFreq);
  // copy to output
  for(int i=0; i<kFloatsPerDSPVector; ++i) {
    outputs[0][i] = v[i];
  }
}
