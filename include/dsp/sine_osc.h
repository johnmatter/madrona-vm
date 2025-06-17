#pragma once

#include "dsp/module.h"
#include "MLDSPGens.h" // For SineGen

class SineOsc : public DSPModule {
public:
  explicit SineOsc(float sampleRate);
  ~SineOsc() override = default;

  void process(const float** inputs, float** outputs) override;

private:
  ml::SineGen mOsc;
};
