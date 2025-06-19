#pragma once
#include <vector>
#include <string>
// Structure to hold audio device information
struct AudioDeviceInfo {
  unsigned int id;
  std::string name;
  unsigned int outputChannels;
  unsigned int inputChannels;
  bool isDefault;
};
// Audio device enumeration utilities
class AudioDeviceManager {
public:
  // Get list of all available audio devices
  static std::vector<AudioDeviceInfo> getAvailableDevices();
  // Get the system default output device ID
  static unsigned int getDefaultOutputDevice();
  // Get the system default input device ID
  static unsigned int getDefaultInputDevice();
  // Find a device by name (returns 0 if not found)
  static unsigned int findDeviceByName(const std::string& name);
  // Check if a device ID is valid
  static bool isValidDevice(unsigned int deviceId);
};
