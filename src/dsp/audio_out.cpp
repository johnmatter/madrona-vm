#include "dsp/audio_out.h"
#include "MLDSPMath.h" // for kFloatsPerDSPVector
#include "MLDSPOps.h"
#include "MLAudioContext.h"
#include "audio/custom_audio_task.h"
#include "audio/device_info.h"
#include <iostream>
#include <vector>
namespace madronavm {
/*
 * Audio Device Selection Implementation Notes:
 *
 * madronalib's AudioTask hardcodes device selection to use getDefaultOutputDevice().
 * To add device selection without modifying madronalib code, we use a CustomAudioTask
 * that replicates the essential functionality of ml::AudioTask but with device selection.
 *
 * This approach:
 * - Maintains compatibility with existing AudioOut interface
 * - Allows specifying audio output device via constructor parameter
 * - Provides static methods to enumerate and query available devices
 * - Uses the same RtAudio and SignalProcessBuffer infrastructure as madronalib
 */
using namespace ml;
constexpr int kOutputChannels = 2;
AudioOut::AudioOut(float sampleRate, bool testMode, unsigned int deviceId)
    : dsp::DSPModule(sampleRate), mTestMode(testMode), mDeviceId(deviceId) {
  if (!mTestMode) {
    // Create context and custom audio task with device selection
    mContext = std::make_unique<ml::AudioContext>(0, kOutputChannels, static_cast<int>(sampleRate));
    // Create custom audio task
    mCustomAudioTask = std::make_unique<ml::CustomAudioTask>(
        mContext.get(),
        [this](ml::AudioContext* ctx) {
          this->audioCallback(ctx);
        },
        deviceId);
    mCustomAudioTask->startAudio();
  }
}
AudioOut::~AudioOut() {
  if (mCustomAudioTask) {
    mCustomAudioTask->stopAudio();
  }
}
void AudioOut::process(const float** inputs, int num_inputs, float** outputs, int num_outputs) {
    if (mTestMode) {
        // In test mode, just copy inputs to the output pointers provided
        // by the VM. This allows testing the VM's output.
        if (num_inputs > 0 && inputs[0] && outputs[0]) {
            std::memcpy(outputs[0], inputs[0], kFloatsPerDSPVector * sizeof(float));
        }
        if (num_inputs > 1 && inputs[1] && outputs[1]) {
            std::memcpy(outputs[1], inputs[1], kFloatsPerDSPVector * sizeof(float));
        }
        return;
    }
    // For the real audio task, the CustomAudioTask callback will pull data,
    // so this process method doesn't need to do anything here.
    // However, the VM expects a concrete implementation.
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
} // namespace madronavm
