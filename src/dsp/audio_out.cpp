#include "dsp/audio_out.h"
#include "MLDSPMath.h" // for kFloatsPerDSPVector
#include "MLDSPOps.h"
#include <iostream>
// I believe constexpr keeps the scope of these variables local to this file
constexpr int kOutputChannels = 2;
// 16 blocks of buffer
constexpr int kRingBufferFrames = kFloatsPerDSPVector * 16;
AudioOut::AudioOut(float sampleRate, bool testMode) 
  : DSPModule(sampleRate), mTestMode(testMode), mDroppedSampleCount(0) {
  mRingBuffer.resize(kRingBufferFrames * kOutputChannels);
  if (!mTestMode) {
    // Only start real audio in non-test mode
    mContext = std::make_unique<ml::AudioContext>(0, kOutputChannels, static_cast<int>(sampleRate));
    mAudioTask = std::make_unique<ml::AudioTask>(mContext.get(), audioCallback, this);
    mAudioTask->startAudio();
  }
}
AudioOut::~AudioOut() { 
  if (mAudioTask) {
    mAudioTask->stopAudio(); 
  }
}
void AudioOut::process(const float **inputs, float **outputs) {
  const float *left = inputs[0];
  const float *right = inputs[1];
  constexpr int samples = kFloatsPerDSPVector * kOutputChannels;
  if (mRingBuffer.getWriteAvailable() >= samples) {
    float interleaved[samples];
    for (int i = 0; i < kFloatsPerDSPVector; ++i) {
      interleaved[i * 2] = left[i];
      interleaved[i * 2 + 1] = right[i];
    }
    mRingBuffer.write(interleaved, samples);
  } else {
    // Buffer is full, drop samples
    mDroppedSampleCount += samples;
    if (mTestMode) {
      // In test mode, only log occasionally to avoid spam
      if (mDroppedSampleCount % (samples * 100) == 0) {
        std::cerr << "AudioOut: Test mode - dropped " << mDroppedSampleCount << " samples total" << std::endl;
      }
    } else {
      // In real mode, log each drop
      std::cerr << "AudioOut: Ring buffer full, dropping samples!" << std::endl;
    }
  }
}
void AudioOut::audioCallback(ml::AudioContext *ctx, void *state) {
  auto *self = static_cast<AudioOut *>(state);
  constexpr int frames = kFloatsPerDSPVector;
  constexpr int samples = frames * kOutputChannels;
  if (self->mRingBuffer.getReadAvailable() >= samples) {
    float interleaved[samples];
    self->mRingBuffer.read(interleaved, samples);
    auto& outL = ctx->outputs[0];
    auto& outR = ctx->outputs[1];
    for (int i = 0; i < frames; ++i) {
      outL[i] = interleaved[i * 2];
      outR[i] = interleaved[i * 2 + 1];
    }
  } else {
    std::cerr << "AudioOut: Buffer underrun!" << std::endl;
    // TODO: handle buffer underrun more gracefully—e.g. fadeout?
    // Fill with silence.
    ctx->outputs[0] = 0.f;
    ctx->outputs[1] = 0.f;
  }
}
