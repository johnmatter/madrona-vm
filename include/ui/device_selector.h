#pragma once
#include <string>
namespace ui {
class DeviceSelector {
public:
  // Displays a TUI to select an audio device and returns the selected device ID.
  // Returns 0 if the user quits without selecting.
  static unsigned int selectAudioDevice();
};
} // namespace ui 