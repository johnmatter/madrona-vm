#include "ui/device_selector.h"
#include "audio/device_info.h"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include <iostream>
namespace ui {
unsigned int DeviceSelector::selectAudioDevice() {
  using namespace ftxui;
  auto devices = AudioDeviceManager::getAvailableDevices();
  if (devices.empty()) {
    std::cout << "No audio devices found." << std::endl;
    return 0;
  }
  std::vector<std::string> device_names;
  unsigned int default_device_idx = 0;
  for (size_t i = 0; i < devices.size(); ++i) {
    std::string name = devices[i].name;
    if (devices[i].isDefault) {
      name += " (Default)";
      default_device_idx = i;
    }
    device_names.push_back(name);
  }
  int selected = static_cast<int>(default_device_idx);
  unsigned int selected_id = 0;
  auto radiobox = Radiobox(&device_names, &selected);
  radiobox |= Renderer([&](Element inner) {
    return vbox({
               text(L"Select an audio output device:"),
               separator(),
               inner,
               separator(),
               text(L"Press Enter to confirm or Escape to quit."),
           }) |
           border;
  });
  auto screen = ScreenInteractive::TerminalOutput();
  auto renderer = Renderer(radiobox, [&] {
    return vbox({
        radiobox->Render(),
    });
  });
  auto component = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Return) {
      selected_id = devices[selected].id;
      screen.Exit();
      return true;
    }
    return false;
  });
  screen.Loop(component);
  return selected_id;
}
} // namespace ui 