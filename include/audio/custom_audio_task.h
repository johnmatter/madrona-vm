#pragma once
#include "MLAudioContext.h"
#include "MLSignalProcessBuffer.h"
#include <functional>
#include <memory>
// Forward declare RtAudio to avoid including the large header
class RtAudio;
typedef unsigned int RtAudioStreamStatus;
namespace ml {
/**
 * CustomAudioTask: A device-aware audio task that extends madronalib's capabilities
 *
 * This class provides the same functionality as ml::AudioTask but with the ability
 * to specify a specific audio output device. It wraps RtAudio directly and uses
 * madronalib's SignalProcessBuffer for processing.
 */
class CustomAudioTask {
public:
  using AudioCallback = std::function<void(ml::AudioContext*)>;
  CustomAudioTask(ml::AudioContext* ctx, AudioCallback callback, unsigned int deviceId = 0);
  ~CustomAudioTask();
  // Start audio processing with the specified device
  int startAudio();
  // Stop audio processing
  void stopAudio();
  // Get the currently selected device ID
  unsigned int getDeviceId() const { return mDeviceId; }
  // Check if audio is currently running
  bool isRunning() const;
private:
  static int rtAudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                           double streamTime, RtAudioStreamStatus status, void* userData);
  std::unique_ptr<RtAudio> mRtAudio;
  ml::AudioContext* mContext;
  AudioCallback mCallback;
  std::unique_ptr<ml::SignalProcessBuffer> mBuffer;
  unsigned int mDeviceId;
  // Constants
  static constexpr int kMaxBlockSize = 4096;
  static constexpr int kRtAudioCallbackFrames = 512;
};
} // namespace ml
