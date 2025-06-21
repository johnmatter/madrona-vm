#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class Lopass : public DSPModule {
public:
  explicit Lopass(float sampleRate);
  ~Lopass() override;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
private:
  struct impl;
  impl* pImpl;
};
} // namespace madronavm::dsp 