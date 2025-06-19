# Real-Time Re-Patchable Modular System Architecture
## Executive Summary
This document outlines the architecture for transforming the Madrona VM into a real-time re-patchable modular system comparable to eurorack synthesizers. The design addresses the fundamental timing issues identified in the current architecture while enabling dynamic patch loading, real-time parameter control, and lock-free communication between threads.
**This architecture builds upon and extends our existing `DSPModule` system, VM, and compiler infrastructure.**
## Integration with Existing Architecture
### Current System Overview
Our existing system already has the core components for a modular system:
```cpp
// Current DSPModule base class (include/dsp/module.h)
class DSPModule {
public:
  explicit DSPModule(float sampleRate);
  virtual ~DSPModule() = default;
  virtual void process(const float** inputs, float** outputs) = 0;
  virtual void configure(size_t numInputs, size_t numOutputs);
protected:
  float mSampleRate;
};
// Current VM (include/vm/vm.h) 
class VM {
public:
  explicit VM(const ModuleRegistry& registry, float sampleRate = 44100.0f, bool testMode = false);
  void load_program(std::vector<uint32_t> new_bytecode);
  void process(const float **inputs, float **outputs, int num_frames);
private:
  std::map<uint32_t, std::unique_ptr<DSPModule>> m_module_instances;
  // ... other members
};
```
### Existing Modules
We already have functional modules:
- `SineGen` - Sine wave oscillator
- `Gain` - Amplitude control
- `AudioOut` - Audio output with ring buffer
- `ADSR` - Envelope generator  
- `VoiceController` - Voice management
### What We're Adding
The new architecture **extends** rather than replaces the existing system:
1. **Audio-Driven Processing** - Fix timing issues by making audio thread drive VM
2. **Lock-Free Threading** - Make the VM truly real-time safe
3. **Hot-Swappable Patches** - Allow dynamic reconfiguration without audio interruption
## Core Principles
### 1. Real-Time Safe Audio Thread
- **Never blocks**: No mutexes, file I/O, or memory allocation
- **Deterministic timing**: Processes exactly one audio block per callback
- **Lock-free communication**: Uses atomic operations and lock-free data structures
- **Graceful degradation**: Handles patch changes without audio dropouts
### 2. Lock-Free Multi-Threading
- **Audio Thread**: Executes DSP graph at sample rate
- **Control Thread**: Handles UI, patch loading, parameter changes
- **Event Thread**: Processes MIDI, OSC, and timed events
- **Parser Thread**: Compiles patches in background
### 3. Hot-Swappable Patches
- **Atomic graph replacement**: Swap entire compiled graphs at block boundaries
- **Crossfade transitions**: Smooth audio when changing patches
- **Parameter preservation**: Maintain module states across patch changes
- **Rollback capability**: Revert to previous patch if compilation fails
## System Architecture
### Threading Model - Built on Existing Components
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Control       │    │     Event       │    │     Audio       │
│   Thread        │    │     Thread      │    │     Thread      │
│                 │    │                 │    │                 │
│ • UI Events     │    │ • MIDI Input    │    │ • VM::process() │
│ • Patch Loading │───▶│ • OSC Messages  │───▶│ • DSPModule[]   │
│ • File I/O      │    │ • Timers        │    │ • AudioOut      │
│ • Compilation   │    │ • Automation    │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                    ┌─────────────────┐
                    │ Existing Parser │
                    │   Thread        │
                    │                 │
                    │ • PatchGraph    │
                    │ • Compiler      │
                    │ • Bytecode Gen  │
                    │ • ModuleRegistry│
                    └─────────────────┘
```
### Lock-Free Communication Extensions
We add lock-free communication **on top of** the existing VM:
```cpp
// NEW: Lock-free queues for real-time communication
template<typename T, size_t Capacity>
class LockFreeQueue {
  std::atomic<size_t> head_{0};
  std::atomic<size_t> tail_{0};
  std::array<T, Capacity> buffer_;
public:
  bool try_push(const T& item);
  bool try_pop(T& item);
};
// Control → Audio messages
LockFreeQueue<PatchSwapCommand, 8> patchQueue;
// Audio → Control messages  
LockFreeQueue<AudioStatus, 64> statusQueue;
```
### Enhanced Compilation Pipeline
Our existing parser and compiler get real-time extensions:
```cpp
// EXISTING: PatchParser (include/parser/parser.h)
class PatchParser {
  // Already has parseFromJson, etc.
  // ADD: Real-time validation
  bool validateRealTimeConstraints(const PatchGraph& graph);
};
// EXISTING: Compiler (include/compiler/compiler.h) 
class Compiler {
  // Already generates bytecode
  // ADD: Memory layout optimization
  MemoryLayout optimizeForRealTime(const PatchGraph& graph);
};
// NEW: Thread-safe graph swapping
class AtomicGraphSlot {
  std::atomic<std::vector<uint32_t>*> currentBytecode_{nullptr};
  std::atomic<std::vector<uint32_t>*> nextBytecode_{nullptr};
public:
  void scheduleSwap(std::vector<uint32_t>* newBytecode);
  std::vector<uint32_t>* maybeSwap(); // Called from VM::process()
};
```
## Enhanced Module System
### Module System - Works Perfectly As-Is
Our existing `DSPModule` system needs no changes for hot-swappable patches:
```cpp
// EXISTING: DSPModule interface is perfect
class DSPModule {
public:
  explicit DSPModule(float sampleRate);
  virtual ~DSPModule() = default;
  virtual void process(const float** inputs, float** outputs) = 0;
  virtual void configure(size_t numInputs, size_t numOutputs);
protected:
  float mSampleRate;
};
// All existing modules work immediately:
// - SineGen ✓ - Already inherits from DSPModule, works perfectly  
// - Gain ✓ - Already handles audio-rate control
// - AudioOut ✓ - Just needs ring buffer removal
// - ADSR ✓ - Already provides envelope control
// - VoiceController ✓ - Already handles MIDI and polyphony
```
## Concrete Implementation Plan
### Phase 1: Fix Audio Timing (1 week)
**Goal**: Solve the immediate ring buffer overflow issue
**Files to modify:**
- `include/dsp/audio_out.h` - Add VM callback mechanism
- `src/dsp/audio_out.cpp` - Remove ring buffer, use direct callback  
- `include/vm/vm.h` - Add `processBlock()` method
- `examples/simple_vm_demo.cpp` - Use audio-driven processing
```cpp
// Step 1.1: AudioOut direct callback
class AudioOut : public DSPModule {
  std::function<void(float**, int)> vmCallback_;
public:
  void setVMCallback(std::function<void(float**, int)> callback);
  // Remove ring buffer completely
};
// Step 1.2: VM audio-driven processing  
void VM::processBlock(float** outputs, int blockSize) {
  this->process(nullptr, outputs, blockSize);
}
```
### Phase 2: Hot-Swappable Patches (1-2 weeks)
**Goal**: Enable real-time patch changes without audio dropouts
**Files to modify:**
- `include/vm/vm.h` - Add atomic bytecode swapping  
- `src/vm/vm.cpp` - Implement thread-safe program loading
```cpp
// Thread-safe program swapping
class VM {
  std::atomic<std::vector<uint32_t>*> currentBytecode_;
  std::atomic<std::vector<uint32_t>*> nextBytecode_;
public:
  void scheduleNewProgram(std::vector<uint32_t> bytecode);
  bool trySwapProgram(); // Called from audio thread
};
```
### Thread Coordination Architecture
**Goal**: Coordinate VM/parser/compiler threads for seamless patch updates
```cpp
// Complete backend pipeline for hot-swappable patches
class PatchManager {
  Parser parser_;
  Compiler compiler_;  
  VM vm_;
  std::thread compilationThread_;
  std::atomic<bool> compilationInProgress_{false};
public:
  // Load new patch without interrupting audio
  void loadPatch(const std::string& patchPath) {
    if (!compilationInProgress_.exchange(true)) {
      compilationThread_ = std::thread([this, patchPath]() {
        auto graph = parser_.parseFromFile(patchPath);
        auto bytecode = compiler_.compile(graph);
        vm_.scheduleNewProgram(std::move(bytecode));
        compilationInProgress_.store(false);
      });
    }
  }
};
```
### Seamless Patch Updates
**Audio never stops:**
- Audio thread continues processing current patch
- Background thread compiles new patch  
- Atomic swap occurs at block boundary
- No dropouts, clicks, or interruptions
**All existing modules work unchanged:**
- `SineGen`, `Gain`, `ADSR`, `VoiceController` require no modifications
- Audio-rate control provides all needed functionality
- Existing patch format and module connections work perfectly
## Testing Strategy
To ensure the new architecture is robust, performant, and correct, we will implement a multi-layered testing strategy.
### 1. Unit Tests
These tests will validate individual components in isolation, focusing on thread-safety and correctness.
```cpp
class RealtimeComponentTests : public ::testing::Test {
public:
  void testLockFreeQueue_SPSCEmpty() {
    // Single producer, single consumer
    LockFreeQueue<int, 16> q;
    int val;
    ASSERT_FALSE(q.try_pop(val));
    ASSERT_TRUE(q.try_push(123));
    ASSERT_TRUE(q.try_pop(val));
    ASSERT_EQ(val, 123);
  }
  void testLockFreeQueue_MPMCContention() {
    // Multiple producers, multiple consumers
    // Launch several threads to push and pop concurrently
    // Verify that the total number of pushed items equals popped items
    // and no data corruption occurs.
  }
  void testAtomicGraphSlot() {
    // Test the bytecode swapping mechanism
    AtomicGraphSlot slot;
    auto* bytecode1 = new std::vector<uint32_t>{1, 2, 3};
    auto* bytecode2 = new std::vector<uint32_t>{4, 5, 6};
    slot.scheduleSwap(bytecode1);
    // Simulate audio thread swapping the graph
    auto* old_graph = slot.maybeSwap(); // Should return nullptr
    ASSERT_EQ(slot.getCurrentGraph(), bytecode1);
    // Schedule another swap
    slot.scheduleSwap(bytecode2);
    // Swap again
    old_graph = slot.maybeSwap();
    ASSERT_EQ(old_graph, bytecode1); // Should return the old graph for cleanup
    ASSERT_EQ(slot.getCurrentGraph(), bytecode2);
    // Cleanup
    delete old_graph;
    delete slot.getCurrentGraph();
  }
};
```
### 2. Integration Tests
These tests ensure that different parts of the system work together as expected.
```cpp
class RealtimeIntegrationTests : public ::testing::Test {
public:
  void testAudioDrivenVM() {
    // Verify that the audio callback correctly drives VM processing.
    // Use a mock VM that increments a counter on each process call.
    // Run the audio stream for a fixed duration and check if the
    // counter matches the expected number of blocks.
  }
  void testHotSwapWithoutGlitches() {
    // - Load and run a simple patch (e.g., 440Hz sine wave).
    // - Capture the audio output to a buffer.
    // - In a separate thread, load a new patch (e.g., 880Hz sine wave).
    // - Schedule the swap.
    // - Continue capturing audio output.
    // - Analyze the output buffer for discontinuities or clicks at the
    //   swap point.
    // - Verify the frequency correctly changes after the swap.
  }
  void testBackgroundCompilation() {
    // - Start audio processing with an initial patch.
    // - Use the PatchManager to load a new patch in the background.
    // - Monitor the audio thread to ensure it continues running smoothly
    //   without stuttering while the background compilation is active.
    // - Verify that the new patch is correctly loaded and swapped after
    //   compilation finishes.
  }
};
```
### 3. Real-Time Stress & Performance Tests
These tests push the system to its limits to check for stability and performance under load.
```cpp
class RealtimeStressTests : public ::testing::Test {
public:
  void testForAudioDropouts() {
    // - Run a complex patch for an extended period (e.g., 5 minutes).
    // - Monitor the RtAudioStreamStatus in the audio callback.
    // - Assert that no `RTAUDIO_INPUT_OVERFLOW` or 
    //   `RTAUDIO_OUTPUT_UNDERFLOW` flags are ever set.
  }
  void testForMemoryLeaks() {
    // - In a loop, load a patch, let it run for a few seconds,
    //   then schedule a swap to a different patch.
    // - Repeat this process hundreds of times.
    // - Monitor the process's memory usage to ensure it remains stable
    //   and does not grow indefinitely, which would indicate a memory leak
    //   from module instances or bytecode not being cleaned up.
  }
};
```
### 4. Audio Quality Assurance
These tests verify the correctness of the audio output itself.
```cpp
class AudioQualityTests : public ::testing::Test {
public:
  void testBitForBitReproducibility() {
    // For a deterministic patch (no noise generators, etc.), the output
    // should be identical every time.
    // - Generate audio for a fixed number of samples.
    // - Compare the output to a pre-recorded "golden" file.
    // - Assert that the files are bit-for-bit identical.
  }
  void testCrossfadeQuality() {
    // If we implement crossfading on patch swaps:
    // - Create a test that swaps between two simple, constant tones.
    // - Analyze the audio during the swap to ensure a smooth,
    //   equal-power crossfade curve is applied, with no zipper noise.
  }
};
```
## Success Metrics
### Performance Targets
- **Audio Latency**: < 5ms round-trip (128 samples @ 44.1kHz)
- **CPU Usage**: < 50% for complex patches on modest hardware
- **Memory Usage**: < 100MB for typical patches
- **Patch Load Time**: < 100ms for hot swapping
### Reliability Targets
- **Zero Audio Dropouts**: During normal operation
- **Graceful Degradation**: When system is overloaded
- **Stable Memory Usage**: No memory leaks during long sessions
- **Deterministic Timing**: Consistent audio block processing
### Functionality Targets
- **Module Library**: 50+ high-quality modules
- **Polyphony**: 16+ voices simultaneous
- **Automation**: Per-parameter LFO and envelope modulation
- **External Control**: Full MIDI and OSC integration
This architecture provides the foundation for a professional-grade real-time modular synthesizer system that can compete with established platforms while leveraging the power of the Madrona VM and madronalib ecosystem.