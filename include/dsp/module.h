#pragma once
#include <cstddef>
#include "MLDSPGens.h"
namespace madronavm::dsp {
class DSPModule {
public:
  explicit DSPModule(float sampleRate);
  virtual ~DSPModule() = default;
  // Process one block of audio.
  // inputs: An array of pointers to input buffers.
  // outputs: An array of pointers to output buffers.
  virtual void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) = 0;
protected:
  float mSampleRate;
};
} // namespace madronavm::dsp 