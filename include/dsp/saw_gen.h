#pragma once
#include "dsp/module.h"
#include <memory>
namespace madronavm::dsp {
class SawGen : public DSPModule {
public:
  explicit SawGen(float sampleRate);
  ~SawGen() override;
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
private:
  struct Impl;
  std::unique_ptr<Impl> pImpl;
};
} // namespace madronavm::dsp 