#include "dsp/audio_out.h"
#include "MLDSPMath.h" // for kFloatsPerDSPVector
#include "MLDSPOps.h"
#include "MLAudioContext.h"
#include <iostream>
#include <vector>
using namespace ml;
// I believe constexpr keeps the scope of these variables local to this file
constexpr int kOutputChannels = 2;
AudioOut::AudioOut(float sampleRate, bool testMode)
    : DSPModule(sampleRate), mTestMode(testMode) {
  if (!mTestMode) {
    // Only start real audio in non-test mode
    mContext = std::make_unique<ml::AudioContext>(0, kOutputChannels,
                                                 static_cast<int>(sampleRate));
    mAudioTask = std::make_unique<ml::AudioTask>(
        mContext.get(),
        [](ml::AudioContext* ctx, void *state) {
          static_cast<AudioOut *>(state)->audioCallback(ctx);
        },
        this);
    mAudioTask->startAudio();
  }
}
AudioOut::~AudioOut() {
  if (mAudioTask) {
    mAudioTask->stopAudio();
  }
}
void AudioOut::process(const float **inputs, float **outputs) {
  // This module is now just a driver for the audio callback.
  // The VM will handle writing to the final output buffers.
  // The graph compiler should route the inputs to this module
  // to the main output buffers of the VM.
}
void AudioOut::setVMCallback(std::function<void(float **, int)> callback) {
  vmCallback_ = callback;
}
void AudioOut::audioCallback(ml::AudioContext* ctx) {
  if (vmCallback_) {
    std::vector<float *> out_buffers;
    auto& outputs = ctx->outputs;
    out_buffers.reserve(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
      out_buffers.push_back(outputs[i].getBuffer());
    }
    vmCallback_(out_buffers.data(), kFloatsPerDSPVector);
  } else {
    // Fill with silence if no callback is set.
    auto& outputs = ctx->outputs;
    for (size_t i = 0; i < outputs.size(); ++i) {
      outputs[i] = 0.f;
    }
  }
}
