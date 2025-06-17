#include "dsp/audio_out.h"
#include "MLDSPMath.h" // for kFloatsPerDSPVector
#include "MLDSPOps.h"
#include <iostream>
// I believe constexpr keeps the scope of these variables local to this file
constexpr int kOutputChannels = 2;
// 16 blocks of buffer
constexpr int kRingBufferFrames = kFloatsPerDSPVector * 16;
AudioOut::AudioOut(float sampleRate) : DSPModule(sampleRate) {
  mRingBuffer.resize(kRingBufferFrames * kOutputChannels);
  mContext = std::make_unique<ml::AudioContext>(0, kOutputChannels, static_cast<int>(sampleRate));
  mAudioTask = std::make_unique<ml::AudioTask>(mContext.get(), audioCallback, this);
  mAudioTask->startAudio();
}
AudioOut::~AudioOut() { mAudioTask->stopAudio(); }
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
    // Buffer is full, drop samples and log.
    // TODO: handle dropped samples more gracefully.
    std::cerr << "AudioOut: Ring buffer full, dropping samples!" << std::endl;
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
