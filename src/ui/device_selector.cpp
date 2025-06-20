#include "ui/device_selector.h"
#include "audio/device_info.h"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include <iostream>
namespace ui {
using namespace ftxui;
  // Display a component nicely with a title on the left.
  Component Wrap(std::string name, Component component) {
    return Renderer(component, [name, component] {
      return hbox({
        text(name) | size(WIDTH, EQUAL, 8),
        separator(),
        component->Render() | xflex,
      }) |
      xflex;
    });
  }
unsigned int DeviceSelector::selectAudioDevice() {
  auto devices = AudioDeviceManager::getAvailableDevices();
  if (devices.empty()) {
    std::cout << "No audio devices found." << std::endl;
    return 0;
  }
  // get device names and identify default device
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
  // https://arthursonzogni.com/FTXUI/doc/examples_2component_2gallery_8cpp-example.html
  int selected = 0;
  auto screen = ScreenInteractive::TerminalOutput();
  auto radiobox = Radiobox(&device_names, &selected);
  radiobox = Wrap("Select a device", radiobox);
  std::string button_label = "Accept";
  std::function<void()> on_button_clicked_;
  auto accept = Button(&button_label, screen.ExitLoopClosure());
  accept = Wrap("", accept);
  auto layout = Container::Vertical({
      radiobox,
      accept,
  });
  auto component = Renderer(layout, [&] {
    return vbox({
      radiobox->Render(),
      separator(),
      accept->Render(),
    }) |
    xflex | size(WIDTH, GREATER_THAN, 40) | border;
  });
  screen.Loop(component);
  return devices[selected].id;
}
} // namespace ui
