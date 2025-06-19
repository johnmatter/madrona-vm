#pragma once
#include "dsp/module.h"
#include "madronalib.h"
#include "MLAudioTask.h"
#include "MLDSPBuffer.h"
#include <memory>
#include <functional>
// Forward-declare madronalib types
namespace ml {
  class DSPVectorDynamic;
  class AudioContext;
}
class AudioOut : public DSPModule {
public:
  explicit AudioOut(float sampleRate, bool testMode = false);
  ~AudioOut() override;
  // Delete copy and move constructors.
  // This class manages unique hardware resources via unique_ptr, so creating copies might be weird.
  // I believe the compiler wouldn't even generate these in the first place, since we declared `virtual ~DSPModule() = default;` in the base class.
  // So. These are just for future readers of the code.
  // see https://en.wikipedia.org/wiki/Rule_of_three_(C%2B%2B_programming)
  AudioOut(const AudioOut&) = delete;
  AudioOut& operator=(const AudioOut&) = delete;
  void process(const float** inputs, float** outputs) override;
  void setVMCallback(std::function<void(float**, int)> callback);
private:
  void audioCallback(ml::AudioContext* ctx);
  std::unique_ptr<ml::AudioContext> mContext;
  std::unique_ptr<ml::AudioTask> mAudioTask;
  std::function<void(float**, int)> vmCallback_;
  bool mTestMode;
};
