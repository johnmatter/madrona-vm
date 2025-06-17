#pragma once

#include "dsp/module.h"
#include "MLDSPFilters.h"

class ADSR : public DSPModule {
public:
  explicit ADSR(float sampleRate);
  void process(const float** inputs, float** outputs) override;
  void configure(size_t numInputs, size_t numOutputs) override;

private:
  ml::ADSR mADSR;
}; 