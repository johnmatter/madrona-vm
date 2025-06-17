#pragma once

#include <cstddef>
#include "MLDSPGens.h"

class DSPModule {
public:
  explicit DSPModule(float sampleRate);
  virtual ~DSPModule() = default;

  // Process one block of audio.
  // inputs: An array of pointers to input buffers.
  // outputs: An array of pointers to output buffers.
  virtual void process(const float** inputs, float** outputs) = 0;

  // Called by the VM when the module is connected or reconfigured within the graph.
  // Modules can override this to perform one-time setup based on the number of
  // connections they have.
  // There's probably a smarter set of parameters to pass here. Maybe a kwargs or dict. We'll figure that out once we have some more modules.
  // numInputs: The number of input connections.
  // numOutputs: The number of output connections.
  virtual void configure(size_t numInputs, size_t numOutputs);

  void setSampleRate(float sampleRate);

protected:
  float mSampleRate;
}; 