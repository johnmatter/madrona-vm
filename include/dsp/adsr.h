#pragma once
#include "dsp/module.h"
#include "MLDSPFilters.h"
namespace madronavm::dsp {
class ADSR : public DSPModule {
public:
  explicit ADSR(float sampleRate);
  ~ADSR() override = default;
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
private:
  ml::ADSR mADSR;
};
} // namespace madronavm::dsp