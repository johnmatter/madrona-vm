#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class Gain : public DSPModule {
public:
  explicit Gain(float sampleRate);
  ~Gain() override = default;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
};
} // namespace madronavm::dsp 