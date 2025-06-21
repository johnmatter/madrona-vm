# Embedded Systems Analysis: Madrona VM for Eurorack Modules
## Executive Summary
This document provides a comprehensive analysis of the current madrona-vm implementation and its suitability for deployment on embedded systems, specifically targeting Eurorack synthesizer modules powered by ESP32, Teensy 4.1, and STM32 microcontrollers. Rather than abandoning the core madronalib dependency, we propose strategic adaptations to preserve Randy Jones' SIMD-optimized DSP work while making it suitable for embedded deployment.
## Eurorack Context and Requirements
### Eurorack Module Specifications
| Specification | Requirement | Embedded Impact |
|---------------|-------------|-----------------|
| Power Supply | ±12V, +5V rails | Low-power design critical |
| Current Draw | <150mA per rail typical | Power budget constraints |
| Form Factor | 3U rack height, HP width | PCB size limitations |
| Audio Quality | 16-bit minimum, 48kHz+ | Real-time processing required |
| CV Range | ±5V, ±10V typical | ADC/DAC requirements |
| Gate/Trigger | 0V/+5V digital signals | GPIO configuration |
| User Interface | Knobs, buttons, LEDs | Limited I/O pins |
### Target Platform Capabilities
| Platform | RAM | Flash | CPU | Audio Capability | FPU Performance |
|----------|-----|-------|-----|------------------|-----------------|
| ESP32 | 320KB SRAM | 4MB | Dual-core 240MHz | I2S DAC/ADC support | Limited (software emulated) |
| Teensy 4.1 | 1MB RAM | 8MB Flash | 600MHz ARM Cortex-M7 | Audio Library, excellent FPU | Excellent (hardware FPU) |
| STM32F4/F7 | 192KB-1MB SRAM | 1MB-2MB Flash | 180-216MHz ARM Cortex-M4/M7 | SAI/I2S audio interfaces | Good (hardware FPU) |
| RP2040 | 264KB SRAM | 2MB Flash | Dual-core 133MHz ARM Cortex-M0+ | I2S support, PIO for audio | Poor (no hardware FPU) |
## Critical Point: preserve madronalib's core value
The DSPVector SIMD architecture and Randy's extensive DSP algorithm collection are the primary advantage of this project. Rather than replacing madronalib, we need to adapt it strategically for embedded deployment.
### madronalib Memory Analysis
```cpp
// From MLDSPMath.h:
constexpr size_t kFloatsPerDSPVectorBits = 6;
constexpr size_t kFloatsPerDSPVector = 1 << kFloatsPerDSPVectorBits;  // = 64 floats
```
DSPVector Memory Usage:
- Single DSPVector: `64 floats × 4 bytes = 256 bytes`
- With SIMD alignment: `~270 bytes per DSPVector`
- Typical DSP module: 2-4 DSPVectors = `~1KB per module instance`
This is actually reasonable for higher-end embedded platforms!
## Current Implementation Analysis
### 1. Virtual Machine (src/vm/vm.cpp)
#### Strengths for Embedded
- Bytecode interpretation: Deterministic execution time
- Static opcode dispatch: Efficient switch-based VM
- Modular architecture: Can disable unused modules
#### Current Issues for Embedded
```cpp
// PROBLEM: Dynamic memory allocation
std::vector<uint32_t> m_bytecode;
std::map<std::string, std::unique_ptr<dsp::DSPModule>> m_modules;
// SOLUTION: Static allocation
uint32_t m_bytecode[MAX_BYTECODE_SIZE];
DSPModuleRegistry m_modules;  // Fixed-size array
```
#### Embedded Adaptation Strategy
1. Pre-allocate bytecode buffer: Replace `std::vector` with fixed-size array
2. Static module registry: Pre-allocated array of module instances
3. Stack-based VM registers: Replace dynamic DSPVector allocation
4. Compile-time module selection: Only include needed DSP modules
### 2. Compiler (src/compiler/compiler.cpp)
#### Current State
- Desktop-focused: Assumes unlimited memory for compilation
- JSON parsing: Heavy nlohmann::json dependency
- Dynamic graph building: Uses STL containers extensively
#### Embedded Adaptation Strategy
```cpp
// CURRENT (unsuitable for embedded):
nlohmann::json patch_json = nlohmann::json::parse(patch_text);
std::vector<Module> modules;
std::map<std::string, Connection> connections;
// EMBEDDED SOLUTION:
struct EmbeddedPatch {
  Module modules[MAX_MODULES];           // 32 modules max
  Connection connections[MAX_CONNECTIONS]; // 128 connections max
  uint8_t module_count;
  uint8_t connection_count;
};
// Compile patches on desktop, store as binary blobs
class EmbeddedCompiler {
  static EmbeddedPatch* compile_to_binary(const char* json_text);
  static void store_patch_to_flash(const EmbeddedPatch* patch, uint32_t slot);
};
```
### 3. madronalib Integration - The Heart of the System
#### Why madronalib is Essential
1. SIMD Optimization: Years of hand-tuned SSE/NEON code
2. Algorithm Collection: Oscillators, filters, effects, envelopes
3. Audio Quality: Professional-grade DSP implementations
4. Performance: Vectorized operations crucial for real-time audio
#### madronalib Embedded Challenges & Solutions
##### Challenge 1: Memory Usage
```cpp
// CURRENT: Each DSPVector uses 256 bytes + alignment
ml::DSPVector oscillator;  // 256 bytes
ml::DSPVector filter;      // 256 bytes
ml::DSPVector envelope;    // 256 bytes
ml::DSPVector vca;         // 256 bytes
// Total: a couple KB per voice
```
SOLUTION: Selective DSPVector Usage
```cpp
// Use smaller buffer sizes for embedded
#ifdef EMBEDDED_TARGET
constexpr size_t kFloatsPerDSPVectorBits = 4;  // 16 floats instead of 64
constexpr size_t kFloatsPerDSPVector = 1 << kFloatsPerDSPVectorBits;  // = 16
#endif
// Or use scalar processing for simple modules
class EmbeddedSineGen : public DSPModule {
  float phase = 0.0f;
  float phase_increment = 0.0f;
  void process(const float** inputs, int num_inputs,
               float** outputs, int num_outputs) override {
    // Process sample-by-sample instead of vector-by-vector
    for (int i = 0; i < BUFFER_SIZE; i++) {
      outputs[0][i] = sinf(phase);
      phase += phase_increment;
      if (phase >= TWO_PI) phase -= TWO_PI;
    }
  }
};
```
##### Challenge 2: SIMD Availability
Teensy 4.1: Excellent ARM NEON support
STM32: ARM NEON on higher-end models
```cpp
// Adaptive SIMD usage
#ifdef ARM_NEON_AVAILABLE
  #define USE_MADRONALIB_FULL
#else
  #define USE_MADRONALIB_SCALAR
#endif
```
##### Challenge 3: Dynamic Allocation in madronalib
```cpp
// PROBLEM: madronalib uses some dynamic allocation
std::vector<float> delayBuffer;  // In delay lines
std::map<Symbol, Value> parameters;  // In parameter system
// SOLUTION: Static embedded wrappers
template<int MAX_DELAY_SAMPLES>
class EmbeddedDelay {
  float m_buffer[MAX_DELAY_SAMPLES];
  int m_write_pos = 0;
  // ... static implementation
};
```
### 4. Parser (src/parser/parser.cpp)
#### Current Issues
- nlohmann::json dependency: 100KB+ library
- Dynamic string processing: Heap allocations
- Runtime parsing: Too slow for embedded
#### Embedded Solution: Ahead-of-Time Compilation
```cpp
// Desktop tool: Convert JSON patches to binary format
struct BinaryPatch {
  uint16_t magic;           // 0x1234
  uint16_t version;         // Format version
  uint8_t module_count;     // Number of modules
  uint8_t connection_count; // Number of connections
  BinaryModule modules[MAX_MODULES];
  BinaryConnection connections[MAX_CONNECTIONS];
  uint32_t bytecode_size;
  uint32_t bytecode[MAX_BYTECODE_SIZE];
};
// Embedded: Load pre-compiled binary patches
class EmbeddedPatchManager {
  static bool load_patch_from_flash(uint8_t patch_id, BinaryPatch* patch);
  static bool load_patch_from_sd(const char* filename, BinaryPatch* patch);
};
```
### 5. Audio System Integration
#### Current Desktop Audio (src/audio/custom_audio_task.cpp)
```cpp
// Uses RtAudio - unsuitable for embedded
RtAudio audio;
audio.openStream(&outputParams, nullptr, RTAUDIO_FLOAT32,
                 sampleRate, &bufferFrames, &audioCallback);
```
#### Embedded Audio Solutions
##### ESP32 Audio
```cpp
#include "driver/i2s.h"
class ESP32AudioTask {
  static void audio_task(void* parameter) {
    int16_t samples[BUFFER_SIZE];
    while (true) {
      // Process DSP
      vm.process_audio_block(samples, BUFFER_SIZE);
      // Output via I2S
      i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
    }
  }
};
```
##### Teensy 4.1 Audio
```cpp
#include <Audio.h>
// Use Teensy Audio Library for optimal performance
AudioInputI2S input;
AudioOutputI2S output;
AudioConnection patchCord1(input, 0, madronaVM, 0);
AudioConnection patchCord2(madronaVM, 0, output, 0);
class TeensyMadronaNode : public AudioStream {
  virtual void update(void) override {
    audio_block_t* block = receiveWritable(0);
    vm.process_audio_block(block->data, AUDIO_BLOCK_SAMPLES);
    transmit(block, 0);
    release(block);
  }
};
```
##### STM32 Audio
```cpp
// Use STM32 SAI (Serial Audio Interface)
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai) {
  vm.process_audio_block(audio_buffer, BUFFER_SIZE);
}
```
##### RP2040 Audio
```cpp
// Use RP2040 PIO for I2S audio
#include "hardware/pio.h"
#include "pico/audio_i2s.h"
class RP2040AudioTask {
  static void audio_callback(void) {
    int16_t samples[BUFFER_SIZE];
    vm.process_audio_block(samples, BUFFER_SIZE);
    audio_i2s_dma_write(samples, BUFFER_SIZE);
  }
};
```
## Embedded Architecture Proposal
### Tiered Deployment Strategy
#### Tier 1: Teensy 4.1 (Premium Platform)
- Full madronalib: keep DSPVector at 64 floats
- SIMD optimized: leverage ARM Cortex-M7 FPU + NEON
- Complex patches, High polyphony
```cpp
// Teensy configuration
#define MADRONA_EMBEDDED_TIER1
#define MADRONA_MAX_MODULES 16
#define MADRONA_MAX_VOICES 8
#define MADRONA_BUFFER_SIZE 64
// Keep full madronalib DSPVector size
```
#### Tier 2: STM32F7 (Balanced Platform)
- Reduced madronalib: DSPVector at 32 floats
- Selective SIMD: use NEON where available
- Medium complexity patches, limited polyphony
```cpp
// STM32F7 configuration
#define MADRONA_EMBEDDED_TIER2
#define MADRONA_MAX_MODULES 8
#define MADRONA_MAX_VOICES 4
#define MADRONA_BUFFER_SIZE 32
#define kFloatsPerDSPVectorBits 5  // 32 floats
```
#### Tier 3: ESP32 - **LIMITED VIABILITY**
- **WARNING**: ESP32 lacks hardware FPU, making floating-point DSP very expensive
- Scalar processing only: No DSPVector usage for complex DSP
- Extremely basic patches: Simple oscillators and basic filters only
```cpp
// ESP32 configuration - PROTOTYPE ONLY
#define MADRONA_EMBEDDED_TIER3
#define MADRONA_MAX_MODULES 2  // Reduced from 4
#define MADRONA_MAX_VOICES 1
#define MADRONA_BUFFER_SIZE 64  // Larger buffers to reduce CPU load
#define MADRONA_SCALAR_ONLY     // No DSPVector usage
```
#### Tier 4: RP2040 - **NOT RECOMMENDED**
- Cortex-M0+ has no hardware FPU - all floating point is software emulated
- Software FPU operations are 10-100x slower than hardware
- Large buffers don't help: 128-sample buffers create 8ms latency - unusable for real-time audio
- Platform fundamentally unsuitable for floating-point DSP
```cpp
// RP2040 - ABANDON THIS PLATFORM FOR AUDIO DSP
// Even with compromises, performance and latency are poor:
// - 128 samples @ 16kHz = 8ms latency (probably unusable)
// - Software FPU makes even simple DSP operations prohibitively expensive
// - Better to focus resources on viable platforms (STM32, Teensy)
```
### Memory Layout Strategy
```cpp
// Static memory allocation for embedded
struct EmbeddedMadronaVM {
  // Pre-allocated DSP modules (no dynamic allocation)
  SineGen sine_generators[4];
  SVFilter filters[4];
  ADSR envelopes[4];
  // Static bytecode buffer
  uint32_t bytecode[MAX_BYTECODE_SIZE];
  // Audio processing buffers
  float audio_input[BUFFER_SIZE];
  float audio_output[BUFFER_SIZE];
  // DSPVector pool (reused across modules)
  ml::DSPVector vector_pool[MAX_CONCURRENT_VECTORS];
  // Module registry
  DSPModule* module_registry[MAX_MODULES];
};
// Total memory footprint: ~32KB for Tier 1, ~16KB for Tier 2, ~8KB for Tier 3
```
## Implementation Roadmap
### Phase 1: madronalib Embedded Adaptation (4 weeks)
1. **Create embedded-specific madronalib config**
   - Configurable DSPVector sizes
   - Optional SIMD based on platform
   - Static memory allocators
2. **Develop embedded DSP module wrappers**
   - Replace dynamic allocation with static pools
   - Add platform-specific optimizations
   - Maintain API compatibility
3. **Create tiered platform configurations**
   - Tier 1/2/3 build configurations
   - Memory usage optimization per tier
### Phase 2: VM and Compiler Adaptation (3 weeks)
1. **Static VM implementation**
   - Replace STL containers with static arrays
   - Pre-allocate all memory at startup
   - Add compile-time module selection
2. **Binary patch format**
   - Desktop compiler for AOT compilation
   - Efficient binary patch loading
   - Flash storage management
### Phase 3: Platform-Specific Audio Integration (3 weeks)
1. **Teensy 4.1 integration**
   - Teensy Audio Library compatibility
   - Optimized for ARM Cortex-M7
2. **ESP32 integration**
   - I2S audio interface
   - FreeRTOS task integration
3. **STM32 integration**
   - SAI/I2S audio interface
   - HAL compatibility
### Phase 4: Eurorack Hardware Integration (4 weeks)
1. **CV input/output**
   - ADC/DAC for CV signals
   - Voltage scaling and protection
2. **User interface**
   - Rotary encoders for parameter control
   - LED feedback and status display
3. **Power management**
   - Efficient power supply design
   - Current consumption optimization
## Performance Targets
### Audio Performance
| Platform | Sample Rate | Buffer Size | Latency | CPU Usage | Notes |
|----------|------------|-------------|---------|-----------|-------|
| **Teensy 4.1** | 44.1kHz | 32 samples | 0.73ms | <40% | Excellent FPU performance |
| **STM32F7** | 44.1kHz | 32 samples | 0.73ms | <65% | Good FPU, realistic target |
| **ESP32** | 22kHz | 64 samples | 2.9ms | <85% | **Software FPU - very limited** |
### Memory Usage Targets
| Platform | Total RAM | VM Usage | madronalib Usage | Free RAM | Viability |
|----------|-----------|----------|------------------|----------|-----------|
| **Teensy 4.1** | 1MB | 32KB | 128KB | 864KB | **EXCELLENT** |
| **STM32F7** | 512KB | 16KB | 64KB | 432KB | **GOOD** |
| **ESP32** | 320KB | 8KB | 16KB | 296KB | **MARGINAL** - scalar only |
## Risk Mitigation
### High Risk
1. **madronalib Memory Usage**: If DSPVector memory usage exceeds platform limits
   - **Mitigation**: Implement scalar fallbacks for memory-constrained modules
2. **Real-time Performance**: Audio dropouts due to excessive processing
   - **Mitigation**: Tiered complexity limits and performance monitoring
3. **SIMD Compatibility**: Platform differences in SIMD support
   - **Mitigation**: Automatic fallback to scalar implementations
### Medium Risk
1. **Power Consumption**: Exceeding Eurorack power budget
   - **Mitigation**: Dynamic clock scaling and sleep modes
2. **Flash Storage**: Limited space for patches and samples
   - **Mitigation**: External SD card storage and patch compression
## Platform Recommendations
### 1. Initial Prototyping Platform: **Teensy 4.1**
**Rationale:**
- **Excellent FPU performance**: ARM Cortex-M7 with double-precision FPU
- **Mature ecosystem**: Proven Audio Library and development tools
- **Generous resources**: 1MB RAM, 600MHz CPU provides significant headroom
- **Easy debugging**: USB-based development and debugging
- **Fast iteration**: Minimal hardware design required
**Specifications:**
- Sample Rate: 44.1kHz
- Buffer Size: 32 samples (0.73ms latency)
- Expected CPU usage: <40% for moderate complexity patches
- Memory usage: ~160KB for VM + madronalib
### 2. Production Platform: **Custom STM32H7 Board**
**Rationale:**
- **Excellent price/performance**: STM32H743 or H753 at ~$15-20 in production
- **Superior FPU**: ARM Cortex-M7 with double-precision FPU
- **Optimal power efficiency**: Much better than Teensy 4.1 for battery/Eurorack use
- **Customizable**: Design exactly what's needed for Eurorack integration
- **Available resources**: 1MB RAM, up to 480MHz CPU
**Specifications:**
- Sample Rate: 44.1kHz or 48kHz
- Buffer Size: 32 samples (0.73ms latency)
- Expected CPU usage: <50% for complex patches
- Current draw: <100mA @ 5V, <50mA @ ±12V (achievable with proper design)
**Alternative Production Option: STM32F7**
- Lower cost option (~$8-12) with slightly reduced performance
- Still excellent for monophonic or simple polyphonic synthesis
- 216MHz ARM Cortex-M7 with single-precision FPU
### Power Consumption Analysis
| Platform | 5V Current | +12V Current | -12V Current | Total Power |
|----------|------------|--------------|--------------|-------------|
| **Teensy 4.1** | 150mA | 20mA | 5mA | ~1.05W |
| **STM32H7 Custom** | 80mA | 15mA | 5mA | ~0.58W |
| **STM32F7 Custom** | 60mA | 12mA | 5mA | ~0.44W |
*Note: ESP32 and RP2040 excluded due to inadequate DSP performance*
## Conclusion
Only platforms with hardware FPUs are viable for this madronalib-based project. Strategy should focus on:
1. Teensy 4.1 for rapid prototyping and premium modules
2. STM32H7 for production modules requiring high performance
3. STM32F7 for cost-optimized production modules
The original premise of preserving madronalib's SIMD architecture is sound, but requires platforms with adequate floating-point performance. ESP32 and RP2040 should be excluded from consideration unless willing to completely redesign the DSP algorithms for fixed-point arithmetic.
