#include <iostream>
#include <vector>
#include "dsp/audio_out.h"
#include "audio/device_info.h"
int main() {
  std::cout << "=== Audio Device Selection Demo ===" << std::endl;
  // List all available audio devices using AudioDeviceManager
  std::cout << "\nAvailable audio devices:" << std::endl;
  auto devices = AudioDeviceManager::getAvailableDevices();
  for (size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];
    std::cout << "  Device " << device.id << ": " << device.name;
    if (device.isDefault) {
      std::cout << " (DEFAULT)";
    }
    std::cout << std::endl;
    std::cout << "    Inputs: " << device.inputChannels
              << ", Outputs: " << device.outputChannels << std::endl;
  }
  // Get default device using AudioDeviceManager
  unsigned int defaultDevice = AudioDeviceManager::getDefaultOutputDevice();
  std::cout << "\nDefault output device ID: " << defaultDevice << std::endl;
  // Demonstrate device validation
  if (!devices.empty()) {
    unsigned int testId = devices[0].id;
    bool isValid = AudioDeviceManager::isValidDevice(testId);
    std::cout << "Device " << testId << " is " << (isValid ? "valid" : "invalid") << std::endl;
  }
  // Demonstrate device search by name
  if (!devices.empty()) {
    const auto& firstDevice = devices[0];
    unsigned int foundId = AudioDeviceManager::findDeviceByName(firstDevice.name);
    std::cout << "Found device '" << firstDevice.name << "' with ID: " << foundId << std::endl;
  }
  // Create AudioOut instances with different devices
  std::cout << "\nCreating AudioOut with default device..." << std::endl;
  {
    AudioOut audioOutDefault(48000.0f, true);  // test mode = true
    std::cout << "  Current device: " << audioOutDefault.getCurrentDevice() << std::endl;
  }
  // Select a specific device (if available)
  if (devices.size() > 1) {
    unsigned int selectedDevice = devices[1].id;  // Pick second device
    std::cout << "\nCreating AudioOut with device " << selectedDevice << "..." << std::endl;
    {
      AudioOut audioOutSpecific(48000.0f, true, selectedDevice);  // test mode = true
      std::cout << "  Current device: " << audioOutSpecific.getCurrentDevice() << std::endl;
    }
  }
  std::cout << "\nDemo completed successfully!" << std::endl;
  return 0;
}
