#pragma once
#include "dsp/module.h"
#include "audio/device_info.h"
#include <memory>
#include <functional>
#include <vector>
#include <string>
// Forward-declare madronalib types
namespace ml {
  class AudioContext;
  class CustomAudioTask;
}
namespace madronavm {
class AudioOut : public dsp::DSPModule {
public:
  explicit AudioOut(float sampleRate, bool testMode = false,
                    unsigned int deviceId = 0);
  ~AudioOut() override;
  AudioOut(const AudioOut&) = delete;
  AudioOut& operator=(const AudioOut&) = delete;
  AudioOut(AudioOut&&) = delete;
  AudioOut& operator=(AudioOut&&) = delete;
  void process(const float **inputs, int num_inputs, float **outputs,
               int num_outputs) override;
  void setVMCallback(std::function<void(float**, int)> callback);
  static std::vector<AudioDeviceInfo> getAvailableDevices() {
    return AudioDeviceManager::getAvailableDevices();
  }
  static unsigned int getDefaultOutputDevice() {
    return AudioDeviceManager::getDefaultOutputDevice();
  }
  unsigned int getCurrentDevice() const { return mDeviceId; }
private:
  void audioCallback(ml::AudioContext* ctx);
  std::unique_ptr<ml::AudioContext> mContext;
  std::unique_ptr<ml::CustomAudioTask> mCustomAudioTask;
  std::function<void(float**, int)> vmCallback_;
  bool mTestMode;
  unsigned int mDeviceId;
};
} // namespace madronavm
