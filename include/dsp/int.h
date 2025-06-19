#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class Int : public DSPModule {
public:
  explicit Int(float sampleRate);
  ~Int() override = default;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
  void setValue(int value) { mValue = value; }
private:
  int mValue;
};
} // namespace madronavm::dsp 