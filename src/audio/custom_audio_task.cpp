#include "audio/custom_audio_task.h"
#include "../../external/madronalib/external/rtaudio/RtAudio.h"
#include <iostream>
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
    std::cout << "Stream over/underflow detected." << std::endl;
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
    std::cout << "\nNo audio devices found!\n";
    return 0;
  }
  // List available devices (similar to madronalib)
  RtAudio::DeviceInfo info;
  uint32_t devices = mRtAudio->getDeviceCount();
  auto ids = mRtAudio->getDeviceIds();
  std::cout << "[CustomAudioTask] Found: " << devices << " device(s)\n";
  for (uint32_t i = 0; i < devices; ++i) {
    info = mRtAudio->getDeviceInfo(ids[i]);
    std::cout << "\tDevice " << ids[i] << ": " << info.name << std::endl;
    std::cout << "\t\tinputs: " << info.inputChannels << " outputs: " << info.outputChannels << std::endl;
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
    std::cout << "Error opening stream: " << mRtAudio->getErrorText() << std::endl;
    return 0;
  }
  result = mRtAudio->startStream();
  if (result != RTAUDIO_NO_ERROR) {
    std::cout << "Error starting stream: " << mRtAudio->getErrorText() << std::endl;
    return 0;
  }
  std::cout << "Using output device ID: " << oParams.deviceId << std::endl;
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
