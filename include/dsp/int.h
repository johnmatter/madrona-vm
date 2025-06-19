#pragma once
#include "dsp/module.h"
class Int : public DSPModule {
public:
  explicit Int(float sampleRate);
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
private:
  int mValue;
}; 