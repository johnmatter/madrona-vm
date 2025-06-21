#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class Bandpass : public DSPModule {
public:
  explicit Bandpass(float sampleRate);
  ~Bandpass() override;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
private:
  struct impl;
  impl* pImpl;
};
} // namespace madronavm::dsp 