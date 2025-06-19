#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class Mul : public DSPModule {
public:
  explicit Mul(float sampleRate);
  ~Mul() override = default;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
};
} // namespace madronavm::dsp