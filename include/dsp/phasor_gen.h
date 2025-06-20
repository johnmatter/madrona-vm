#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class PhasorGen : public DSPModule {
public:
  explicit PhasorGen(float sampleRate);
  ~PhasorGen() override;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
private:
  struct impl;
  impl* pImpl;
};
} // namespace madronavm::dsp 