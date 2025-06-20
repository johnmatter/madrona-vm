#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class Threshold : public DSPModule {
public:
  explicit Threshold(float sampleRate);
  ~Threshold() override = default;
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
};
} // namespace madronavm::dsp 