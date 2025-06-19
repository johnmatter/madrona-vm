#include "dsp/voice_controller.h"
namespace madronavm::dsp {
VoiceController::VoiceController(float sampleRate)
    : DSPModule(sampleRate), mEventProcessor(sampleRate) {
  mEventProcessor.setPolyphony(kMaxVoices);
  mEventProcessor.setGlideTimeInSeconds(0.0f);
}
void VoiceController::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
  // Add our queued events to the processor's buffer.
  for(const auto& e : mEventQueue) {
    mEventProcessor.addEvent(e);
  }
  mEventQueue.clear();
  // Process the vector. This will handle all event logic.
  // NOTE: The VM doesn't provide a startOffset, so we assume 0 for now.
  // This is sufficient for simple cases but may not be for more advanced sequencing.
  mEventProcessor.processVector(0);
  // Copy the results to our module's output buffers.
  for (int v = 0; v < kMaxVoices; ++v) {
    const auto& voice = mEventProcessor.getVoice(v);
    for (int i = 0; i < kNumOutputsPerVoice; ++i) {
      int output_idx = v * kNumOutputsPerVoice + i;
      if (output_idx < num_outputs) {
        const float* voiceOutput = voice.outputs.getRowDataConst(i);
        float* moduleOutput = outputs[output_idx];
        if (moduleOutput) {
          for (int s = 0; s < kFloatsPerDSPVector; ++s) {
            moduleOutput[s] = voiceOutput[s];
          }
        }
      }
    }
  }
}
void VoiceController::noteOn(int pitch, int velocity, int voice, int time) {
  ml::Event e;
  e.type = ml::kNoteOn;
  e.sourceIdx = pitch;
  e.value1 = pitch;
  e.value2 = static_cast<float>(velocity) / 127.0f;
  e.time = time;
  e.channel = voice;
  mEventQueue.push_back(e);
}
void VoiceController::noteOff(int pitch, int velocity, int voice, int time) {
  ml::Event e;
  e.type = ml::kNoteOff;
  e.sourceIdx = pitch;
  e.value1 = pitch;
  e.value2 = static_cast<float>(velocity) / 127.0f;
  e.time = time;
  e.channel = voice;
  mEventQueue.push_back(e);
}
} // namespace madronavm::dsp 