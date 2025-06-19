#include "audio/device_info.h"
#include "../../external/madronalib/external/rtaudio/RtAudio.h"
#include <algorithm>
std::vector<AudioDeviceInfo> AudioDeviceManager::getAvailableDevices() {
  RtAudio rtAudio;
  std::vector<AudioDeviceInfo> devices;
  uint32_t deviceCount = rtAudio.getDeviceCount();
  auto ids = rtAudio.getDeviceIds();
  for (uint32_t i = 0; i < deviceCount; ++i) {
    RtAudio::DeviceInfo info = rtAudio.getDeviceInfo(ids[i]);
    AudioDeviceInfo deviceInfo;
    deviceInfo.id = ids[i];
    deviceInfo.name = info.name;
    deviceInfo.outputChannels = info.outputChannels;
    deviceInfo.inputChannels = info.inputChannels;
    deviceInfo.isDefault = info.isDefaultOutput;
    devices.push_back(deviceInfo);
  }
  return devices;
}
unsigned int AudioDeviceManager::getDefaultOutputDevice() {
  RtAudio rtAudio;
  return rtAudio.getDefaultOutputDevice();
}
unsigned int AudioDeviceManager::getDefaultInputDevice() {
  RtAudio rtAudio;
  return rtAudio.getDefaultInputDevice();
}
unsigned int AudioDeviceManager::findDeviceByName(const std::string& name) {
  auto devices = getAvailableDevices();
  auto it = std::find_if(devices.begin(), devices.end(),
    [&name](const AudioDeviceInfo& device) {
      return device.name == name;
    });
  return (it != devices.end()) ? it->id : 0;
}
bool AudioDeviceManager::isValidDevice(unsigned int deviceId) {
  if (deviceId == 0) return true; // 0 means "use default"
  auto devices = getAvailableDevices();
  return std::any_of(devices.begin(), devices.end(),
    [deviceId](const AudioDeviceInfo& device) {
      return device.id == deviceId;
    });
}
