#pragma once
#include "MLDSPGens.h"
#include "dsp/module.h"
namespace madronavm::dsp {
class SineGen : public DSPModule {
public:
  explicit SineGen(float sampleRate);
  ~SineGen() override;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
private:
  struct impl;
  impl* pImpl;
};
} // namespace madronavm::dsp
