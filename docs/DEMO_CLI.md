## CLI Demonstration Application
### Overview
A command-line interface application that demonstrates the complete real-time modular synthesis pipeline. This CLI serves as both a practical tool for testing patches and a showcase of the system's capabilities.
### Features
```bash
# Basic usage
./madrona-synth patch.json
# With options
./madrona-synth patch.json --sample-rate 48000 --buffer-size 256 --hot-reload
# Interactive mode
./madrona-synth --interactive
```
### Functionality
- **Patch Loading**: Parse and compile JSON patch files
- **Real-Time Audio**: Continuous audio synthesis until quit
- **Hot Reloading**: Watch for patch file changes and reload seamlessly
- **MIDI Input**: Accept MIDI controllers and keyboards
- **Performance Monitoring**: Display CPU usage and audio statistics
- **Interactive Controls**: Runtime parameter adjustment
### Implementation
```cpp
class MadronaSynthCLI {
public:
  int main(int argc, char* argv[]) {
    // 1. Parse command line arguments
    auto config = parseArgs(argc, argv);
    // 2. Initialize audio system
    if (!initializeAudio(config)) {
      std::cerr << "Failed to initialize audio system" << std::endl;
      return 1;
    }
    // 3. Load and compile initial patch
    if (!loadPatch(config.patchFile)) {
      std::cerr << "Failed to load patch: " << config.patchFile << std::endl;
      return 1;
    }
    // 4. Start audio processing
    startAudio();
    // 5. Enter interactive loop
    runInteractiveLoop(config);
    // 6. Clean shutdown
    stopAudio();
    return 0;
  }
private:
  PatchManager patchManager_;
  AudioDeviceManager audioManager_;
  MIDIInputManager midiManager_;
  PerformanceMonitor perfMonitor_;
  bool loadPatch(const std::string& filepath) {
    try {
      std::cout << "Loading patch: " << filepath << std::endl;
      // Parse JSON patch file
      auto graph = patchManager_.parser_.parseFromFile(filepath);
      std::cout << "✓ Parsed " << graph.nodes.size() << " modules, " 
                << graph.connections.size() << " connections" << std::endl;
      // Compile to bytecode
      auto bytecode = patchManager_.compiler_.compile(graph, registry_);
      std::cout << "✓ Compiled to " << bytecode.size() << " words of bytecode" << std::endl;
      // Schedule for real-time loading
      patchManager_.vm_.scheduleNewProgram(std::move(bytecode));
      std::cout << "✓ Patch loaded successfully" << std::endl;
      return true;
    } catch (const std::exception& e) {
      std::cerr << "Error loading patch: " << e.what() << std::endl;
      return false;
    }
  }
  void runInteractiveLoop(const Config& config) {
    std::cout << "\nMadrona Synthesizer Running" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  q - Quit" << std::endl;
    std::cout << "  r - Reload patch" << std::endl;
    std::cout << "  s - Show statistics" << std::endl;
    std::cout << "  m - MIDI device list" << std::endl;
    if (config.hotReload) {
      std::cout << "  Hot reload enabled - watching " << config.patchFile << std::endl;
    }
    std::cout << std::endl;
    char input;
    while (std::cin >> input) {
      switch (input) {
        case 'q':
        case 'Q':
          std::cout << "Shutting down..." << std::endl;
          return;
        case 'r':
        case 'R':
          std::cout << "Reloading patch..." << std::endl;
          loadPatch(config.patchFile);
          break;
        case 's':
        case 'S':
          showStatistics();
          break;
        case 'm':
        case 'M':
          showMIDIDevices();
          break;
        default:
          std::cout << "Unknown command: " << input << std::endl;
          break;
      }
    }
  }
  void showStatistics() {
    auto stats = perfMonitor_.getStats();
    std::cout << "\nPerformance Statistics:" << std::endl;
    std::cout << "  CPU Usage: " << stats.cpuUsage << "%" << std::endl;
    std::cout << "  Audio Latency: " << stats.latencyMs << "ms" << std::endl;
    std::cout << "  Dropouts: " << stats.dropoutCount << std::endl;
    std::cout << "  Sample Rate: " << stats.sampleRate << "Hz" << std::endl;
    std::cout << "  Buffer Size: " << stats.bufferSize << " samples" << std::endl;
    std::cout << "  Active Modules: " << stats.activeModules << std::endl;
    std::cout << std::endl;
  }
};
```
### Configuration Options
```cpp
struct CLIConfig {
  std::string patchFile;
  float sampleRate = 44100.0f;
  int bufferSize = 256;
  bool hotReload = false;
  bool interactive = true;
  std::string audioDevice;
  std::string midiDevice;
  bool showPerformance = false;
  int verboseLevel = 1;
};
CLIConfig parseArgs(int argc, char* argv[]) {
  CLIConfig config;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--sample-rate" && i + 1 < argc) {
      config.sampleRate = std::stof(argv[++i]);
    } else if (arg == "--buffer-size" && i + 1 < argc) {
      config.bufferSize = std::stoi(argv[++i]);
    } else if (arg == "--hot-reload") {
      config.hotReload = true;
    } else if (arg == "--audio-device" && i + 1 < argc) {
      config.audioDevice = argv[++i];
    } else if (arg == "--midi-device" && i + 1 < argc) {
      config.midiDevice = argv[++i];
    } else if (arg == "--performance") {
      config.showPerformance = true;
    } else if (arg == "--help" || arg == "-h") {
      printUsage();
      exit(0);
    } else if (arg[0] != '-') {
      config.patchFile = arg;
    }
  }
  return config;
}
```
### Hot Reload Implementation
```cpp
class FileWatcher {
  std::filesystem::file_time_type lastWriteTime_;
  std::string filepath_;
  std::function<void()> callback_;
  std::thread watchThread_;
  std::atomic<bool> watching_{false};
public:
  FileWatcher(const std::string& filepath, std::function<void()> callback)
    : filepath_(filepath), callback_(callback) {
    if (std::filesystem::exists(filepath)) {
      lastWriteTime_ = std::filesystem::last_write_time(filepath);
    }
  }
  void startWatching() {
    watching_ = true;
    watchThread_ = std::thread([this]() {
      while (watching_) {
        checkForChanges();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    });
  }
  void stopWatching() {
    watching_ = false;
    if (watchThread_.joinable()) {
      watchThread_.join();
    }
  }
private:
  void checkForChanges() {
    if (!std::filesystem::exists(filepath_)) return;
    auto currentTime = std::filesystem::last_write_time(filepath_);
    if (currentTime != lastWriteTime_) {
      lastWriteTime_ = currentTime;
      std::cout << "Patch file changed, reloading..." << std::endl;
      callback_();
    }
  }
};
```
### Example Usage Session
```bash
$ ./madrona-synth examples/simple_patch.json --hot-reload
Loading patch: examples/simple_patch.json
✓ Parsed 3 modules, 2 connections
✓ Compiled to 847 words of bytecode
✓ Patch loaded successfully
Madrona Synthesizer Running
Commands:
  q - Quit
  r - Reload patch
  s - Show statistics
  m - MIDI device list
  Hot reload enabled - watching examples/simple_patch.json
Audio started on device: Built-in Output (44.1kHz, 256 samples)
s
Performance Statistics:
  CPU Usage: 12.3%
  Audio Latency: 5.8ms
  Dropouts: 0
  Sample Rate: 44100Hz
  Buffer Size: 256 samples
  Active Modules: 3
Patch file changed, reloading...
✓ Parsed 4 modules, 3 connections
✓ Compiled to 1053 words of bytecode
✓ Patch loaded successfully
q
Shutting down...
Audio stopped.
```
### Integration with Build System
Add to `CMakeLists.txt`:
```cmake
# CLI Application
add_executable(madrona-synth
  src/cli/main.cpp
  src/cli/cli_config.cpp
  src/cli/file_watcher.cpp
  src/cli/performance_monitor.cpp
  src/cli/audio_device_manager.cpp
  src/cli/midi_input_manager.cpp
)
target_link_libraries(madrona-synth
  madronalib
  ${RTAUDIO_LIBRARIES}
  ${RTMIDI_LIBRARIES}
)
# Install CLI tool
install(TARGETS madrona-synth DESTINATION bin)
```
### Deliverable Value
This CLI application serves multiple purposes:
1. **End-to-End Demonstration**: Shows the complete pipeline working in real-time
2. **Development Tool**: Useful for testing patches during development
3. **Performance Validation**: Demonstrates real-time audio processing capabilities
4. **User Experience**: Provides a concrete way to interact with the system
5. **Integration Testing**: Exercises all major components together
The CLI represents the culmination of the modular architecture, showcasing hot-swappable patches, real-time audio synthesis, and professional-grade performance monitoring in a single, user-friendly application.