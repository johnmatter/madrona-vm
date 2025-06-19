#pragma once
#include "dsp/module.h"
#include "MLEventsToSignals.h"
#include <array>
#include <vector>
class VoiceController : public DSPModule {
public:
  static constexpr size_t kMaxVoices = 8;
  static constexpr size_t kNumOutputsPerVoice = ml::kNumVoiceOutputRows;
  explicit VoiceController(float sampleRate);
  void process(const float** inputs, int num_inputs, float** outputs, int num_outputs) override;
  // Control interface
  void noteOn(int pitch, int velocity, int voice = 0, int time = 0);
  void noteOff(int pitch, int velocity, int voice = 0, int time = 0);
private:
  ml::EventsToSignals mEventProcessor;
  std::vector<ml::Event> mEventQueue;
}; 