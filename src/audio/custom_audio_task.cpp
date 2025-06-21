#include "audio/custom_audio_task.h"
#include "../../external/madronalib/external/rtaudio/RtAudio.h"
#include "common/embedded_logging.h"
#include <algorithm>
namespace ml {
CustomAudioTask::CustomAudioTask(ml::AudioContext* ctx, AudioCallback callback, unsigned int deviceId)
  : mContext(ctx), mCallback(callback), mDeviceId(deviceId) {
  mRtAudio = std::make_unique<RtAudio>();
  mBuffer = std::make_unique<ml::SignalProcessBuffer>(ctx->inputs.size(), ctx->outputs.size(), kMaxBlockSize);
}
CustomAudioTask::~CustomAudioTask() {
  stopAudio();
}
int CustomAudioTask::rtAudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                    double /*streamTime*/, RtAudioStreamStatus status, void* userData) {
  auto* task = static_cast<CustomAudioTask*>(userData);
  if (status) {
    MADRONA_AUDIO_LOG_WARN("Stream underflow: status=0x%02X", (uint32_t)status);
  }
  // Set up input and output pointers (non-interleaved)
  constexpr size_t kMaxIOChannels = 64;
  const float* inputs[kMaxIOChannels];
  float* outputs[kMaxIOChannels];
  const float* pInputBuffer = reinterpret_cast<const float*>(inputBuffer);
  float* pOutputBuffer = reinterpret_cast<float*>(outputBuffer);
  size_t nIns = std::min(kMaxIOChannels, task->mContext->inputs.size());
  size_t nOuts = std::min(kMaxIOChannels, task->mContext->outputs.size());
  for (size_t i = 0; i < nIns; ++i) {
    inputs[i] = pInputBuffer + i * nBufferFrames;
  }
  for (size_t i = 0; i < nOuts; ++i) {
    outputs[i] = pOutputBuffer + i * nBufferFrames;
  }
  // Process through the buffer using a lambda
  task->mBuffer->process(inputs, outputs, nBufferFrames, task->mContext,
    [](ml::AudioContext* ctx, void* state) {
      auto* audioTask = static_cast<CustomAudioTask*>(state);
      audioTask->mCallback(ctx);
    }, task);
  return 0;
}
int CustomAudioTask::startAudio() {
  if (mRtAudio->getDeviceCount() < 1) {
    MADRONA_AUDIO_LOG_WARN("No audio devices found");
    return 0;
  }
  // List available devices (similar to madronalib)
  RtAudio::DeviceInfo info;
  uint32_t devices = mRtAudio->getDeviceCount();
  auto ids = mRtAudio->getDeviceIds();
  MADRONA_AUDIO_LOG_INFO("Found %u audio devices", devices);
  for (uint32_t i = 0; i < devices; ++i) {
    info = mRtAudio->getDeviceInfo(ids[i]);
    MADRONA_AUDIO_LOG_INFO("Device %u: channels=0x%08X", 
                           ids[i], (info.inputChannels << 16) | info.outputChannels);
  }
  mRtAudio->showWarnings(true);
  int nInputs = mContext->inputs.size();
  int nOutputs = mContext->outputs.size();
  int sampleRate = mContext->sampleRate;
  unsigned int bufferFrames = kRtAudioCallbackFrames;
  // Set up RtAudio stream params with specified device
  RtAudio::StreamParameters oParams;
  oParams.deviceId = (mDeviceId == 0) ? mRtAudio->getDefaultOutputDevice() : mDeviceId;
  oParams.nChannels = static_cast<unsigned int>(nOutputs);
  oParams.firstChannel = 0;
  RtAudio::StreamParameters* iParams = nullptr;
  if (nInputs > 0) {
    static RtAudio::StreamParameters inputParams;
    inputParams.deviceId = mRtAudio->getDefaultInputDevice();
    inputParams.nChannels = static_cast<unsigned int>(nInputs);
    inputParams.firstChannel = 0;
    iParams = &inputParams;
  }
  RtAudio::StreamOptions options;
  options.flags |= RTAUDIO_NONINTERLEAVED;
  RtAudioErrorType result = mRtAudio->openStream(&oParams, iParams, RTAUDIO_FLOAT32,
                                               sampleRate, &bufferFrames, &rtAudioCallback,
                                               this, &options);
  if (result != RTAUDIO_NO_ERROR) {
    MADRONA_AUDIO_LOG_WARN("Error opening stream: code=%u", (uint32_t)result);
    return 0;
  }
  result = mRtAudio->startStream();
  if (result != RTAUDIO_NO_ERROR) {
    MADRONA_AUDIO_LOG_WARN("Error starting stream: code=%u", (uint32_t)result);
    return 0;
  }
  MADRONA_AUDIO_LOG_INFO("Using output device ID: %u", oParams.deviceId);
  return 1;
}
void CustomAudioTask::stopAudio() {
  if (mRtAudio && mRtAudio->isStreamRunning()) {
    mRtAudio->stopStream();
  }
  if (mRtAudio && mRtAudio->isStreamOpen()) {
    mRtAudio->closeStream();
  }
}
bool CustomAudioTask::isRunning() const {
  return mRtAudio && mRtAudio->isStreamRunning();
}
} // namespace ml
