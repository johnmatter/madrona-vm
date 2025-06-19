#pragma once
#include "dsp/module.h"
namespace madronavm::dsp {
class Add : public DSPModule {
public:
  explicit Add(float sampleRate);
  ~Add() override = default;
  void process(const float **inputs, int num_inputs, float **outputs, int num_outputs) override;
};
} // namespace madronavm::dsp 