#pragma once
#include "dsp/module.h"
namespace madronavm {
class Mul : public DSPModule {
public:
  explicit Mul(float sampleRate);
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
};
} // namespace madronavm 