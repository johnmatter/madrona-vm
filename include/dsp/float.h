#pragma once
#include "dsp/module.h"
class Float : public DSPModule {
public:
  explicit Float(float sampleRate);
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
private:
  float mValue;
}; 