#include "dsp/voice_controller.h"

VoiceController::VoiceController(float sampleRate)
    : DSPModule(sampleRate), mEventProcessor(sampleRate) {
  mEventProcessor.setPolyphony(kMaxVoices);
  mEventProcessor.setGlideTimeInSeconds(0.0f);
}

void VoiceController::process(const float** inputs, float** outputs) {
  process(inputs, outputs, 0);
}

void VoiceController::process(const float** inputs, float** outputs, int startOffset) {
  // madronalib's event processor needs to be told when a new block is starting.
  // However, EventsToSignals::processVector handles this internally by calling
  // beginProcess and endProcess on the voices.

  // Add our queued events to the processor's buffer.
  for(const auto& e : mEventQueue) {
    mEventProcessor.addEvent(e);
  }
  mEventQueue.clear();

  // Process the vector. This will handle all event logic.
  mEventProcessor.processVector(startOffset);

  // Copy the results to our module's output buffers.
  for (int v = 0; v < kMaxVoices; ++v) {
    const auto& voice = mEventProcessor.getVoice(v);
    for (int i = 0; i < kNumOutputsPerVoice; ++i) {
      const float* voiceOutput = voice.outputs.getRowDataConst(i);
      float* moduleOutput = outputs[v * kNumOutputsPerVoice + i];
      if (moduleOutput) {
        for (int s = 0; s < kFloatsPerDSPVector; ++s) {
          moduleOutput[s] = voiceOutput[s];
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