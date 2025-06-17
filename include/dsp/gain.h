#pragma once
#include "dsp/module.h"

class Gain : public DSPModule {
public:
  explicit Gain(float sampleRate);
  void process(const float** inputs, float** outputs) override;
}; 