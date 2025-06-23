#pragma once
#include "dsp/module.h"
#include <memory>
namespace madronavm::dsp {
class PulseGen : public DSPModule {
public:
  explicit PulseGen(float sampleRate);
  ~PulseGen() override;
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
private:
  struct Impl;
  std::unique_ptr<Impl> pImpl;
};
} // namespace madronavm::dsp 