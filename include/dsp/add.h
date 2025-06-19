#pragma once
#include "dsp/module.h"
namespace madronavm {
class Add : public DSPModule {
public:
  explicit Add(float sampleRate);
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
};
} // namespace madronavm 