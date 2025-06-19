#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class Float : public DSPModule {
public:
  explicit Float(float sampleRate);
  ~Float() override = default;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
  void setValue(float value) { mValue = value; }
private:
  float mValue;
};
} // namespace madronavm::dsp 