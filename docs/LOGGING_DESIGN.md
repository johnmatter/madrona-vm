# Embedded Logging System Design Document
## Executive Summary
This document outlines the refactoring of madrona-vm's current iostream-based logging system to use a lightweight, embedded-optimized logging framework designed for deployment on resource-constrained devices such as ESP32, Teensy 4.1, and STM32 microcontrollers. The current system uses scattered `std::cout` and `std::cerr` calls which are unsuitable for embedded deployment due to lack of proper log levels, excessive memory usage, and blocking I/O operations that compromise real-time audio performance.
## Embedded System Constraints
### Target Platform Specifications
| Platform | RAM | Flash | CPU |
|----------|-----|-------|-----|------------------|
| **ESP32** | 320KB SRAM | 4MB | Dual-core 240MHz |
| **Teensy 4.1** | 1MB RAM | 8MB Flash | 600MHz ARM Cortex-M7 |
| **STM32F4** | 192KB SRAM | 1MB Flash | 180MHz ARM Cortex-M4 |
### Critical Embedded Constraints
1. **Memory Limitations**: Total RAM measured in KB, not MB!
2. **No File System**: Many targets lack traditional file systems
3. **No Dynamic Allocation**: Heap allocation forbidden in audio callbacks
4. **No Standard Library**: Limited or no `std::string`, `std::iostream`
5. **Single-threaded**: Many embedded systems lack threading
6. **Power Consumption**: Battery-powered devices require efficiency
7. **Serial Communication**: Primary debug interface is UART/USB serial
## Current State Analysis
### Current Logging Usage Patterns
Current logging falls into these categories:
#### 1. **Informational Output** (using `std::cout`)
- **Audio Device Discovery**: `src/audio/custom_audio_task.cpp`
- **Demo/Example Progress**: `examples/simple_vm_demo.cpp`
- **Integration Test Status**: `tests/integration/integration_test.cpp`
#### 2. **Error Reporting** (using `std::cerr`)
- **VM Bytecode Errors**: `src/vm/vm.cpp`
- **DSP Module Validation**: `include/dsp/validation.h`
- **File I/O Errors**: Test files and examples
#### 3. **Real-time Audio Notifications**
- **Stream Underflow**: `src/audio/custom_audio_task.cpp`
### Problems with Current Approach for Embedded
1. **Excessive Memory Usage**: `std::iostream` overhead unsuitable for embedded
2. **Blocking I/O**: Serial output can block for milliseconds
3. **Dynamic Memory**: String formatting allocates heap memory
4. **No Embedded Transports**: Cannot output to SPI, I2C, wireless
5. **Power Inefficient**: Continuous serial output drains battery
6. **No Compile-time Filtering**: All logging code compiled into binary
## Design Goals
### Primary Objectives (Embedded-Focused)
1. **Zero Heap Allocation**: All logging uses stack or static memory
2. **Deterministic Performance**: Logging never blocks real-time audio
3. **Minimal Memory Footprint**: <2KB RAM overhead total
4. **Compile-time Filtering**: Dead code elimination for release builds
5. **Multiple Transports**: Serial, SPI, I2C, wireless, shared memory
### Secondary Objectives
1. **Power Efficiency**: Configurable low-power modes
2. **Circular Buffer Storage**: No file system dependency
3. **Binary Logging**: Compact encoding for bandwidth-limited transports
4. **Embedded Debugging**: Integration with debuggers, logic analyzers
5. **Cross-platform**: Common API across all embedded targets
## Proposed Solution: Lightweight Embedded Logger
### Why Not spdlog?
spdlog is **unsuitable for embedded systems** due to:
1. **Large Memory Footprint**: ~50KB+ code, dynamic allocations
2. **File System Dependency**: Requires filesystem API not available on bare metal
3. **Threading Model**: Assumes full threading support
4. **Standard Library Usage**: Heavy use of STL containers and streams
5. **Performance Unpredictability**: Formatting/allocation can take milliseconds
### Embedded Architecture Overview
```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│                 Madrona Embedded Logger                     │
│  - Compile-time log level filtering                         │
│  - Zero-allocation macros                                   │
│  - Lock-free circular buffer (audio-safe)                  │
├─────────────────────────────────────────────────────────────┤
│                    Transport Layer                          │
│  - Serial (UART/USB)   - SPI/I2C                          │
│  - WiFi/Bluetooth      - Shared Memory                     │
│  - Logic Analyzer      - Debugger ITM                      │
└─────────────────────────────────────────────────────────────┘
```
## Implementation Plan
### Phase 1: Core Embedded Infrastructure
#### 1.1 Embedded Logger Header (`include/common/embedded_logging.h`)
```cpp
#pragma once
#include <stdint.h>
#include <cstdio>
// Platform-specific includes
#ifdef ARDUINO
#include "Arduino.h"
#elif defined(ESP32)
#include "esp_log.h"
#elif defined(STM32)
#include "stm32_hal.h"
#endif
namespace madronavm::logging {
enum class LogLevel : uint8_t {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
  CRITICAL = 5,
  OFF = 6
};
// Compile-time log level (set by build system)
#ifndef MADRONA_LOG_LEVEL
#define MADRONA_LOG_LEVEL 2  // INFO level default
#endif
// Transport types
enum class Transport : uint8_t {
  SERIAL = 0,     // UART/USB serial
  SPI = 1,        // SPI interface
  I2C = 2,        // I2C interface  
  WIRELESS = 3,   // WiFi/Bluetooth
  CIRCULAR = 4,   // In-memory circular buffer
  ITM = 5         // ARM ITM (Instrumentation Trace Macrocell)
};
// Log entry structure (packed for binary logging)
struct __attribute__((packed)) LogEntry {
  uint32_t timestamp_us;  // Microsecond timestamp
  uint16_t module_id;     // Module identifier
  uint8_t level;          // Log level
  uint8_t format_id;      // Format string ID (for binary logging)
  uint32_t arg1;          // First argument (integer/pointer)
  uint32_t arg2;          // Second argument  
};
// Configuration structure (const, no heap allocation)
struct LogConfig {
  LogLevel min_level = LogLevel::INFO;
  Transport transport = Transport::SERIAL;
  uint16_t buffer_size = 256;           // Circular buffer entries
  bool binary_mode = false;             // Binary vs text logging
  bool power_save = false;              // Reduce power consumption
  uint32_t baud_rate = 115200;          // Serial baud rate
};
// Initialize logging system (called once at startup)
void initialize(const LogConfig& config = LogConfig{});
// Core logging functions (inline for performance)
void log_entry(LogLevel level, uint16_t module_id, const char* format, 
               uint32_t arg1 = 0, uint32_t arg2 = 0);
// Get current microsecond timestamp (platform-specific)
uint32_t get_timestamp_us();
// Flush pending log entries (call from main loop)
void flush();
} // namespace madronavm::logging
// Compile-time log level filtering macros
#define MADRONA_LOG_ENABLED(level) ((level) >= MADRONA_LOG_LEVEL)
// Zero-allocation logging macros with compile-time filtering
#define MADRONA_LOG_TRACE(module, format, ...) \
  do { if (MADRONA_LOG_ENABLED(0)) \
    madronavm::logging::log_entry(madronavm::logging::LogLevel::TRACE, \
                                  module, format, ##__VA_ARGS__); } while(0)
#define MADRONA_LOG_DEBUG(module, format, ...) \
  do { if (MADRONA_LOG_ENABLED(1)) \
    madronavm::logging::log_entry(madronavm::logging::LogLevel::DEBUG, \
                                  module, format, ##__VA_ARGS__); } while(0)
#define MADRONA_LOG_INFO(module, format, ...) \
  do { if (MADRONA_LOG_ENABLED(2)) \
    madronavm::logging::log_entry(madronavm::logging::LogLevel::INFO, \
                                  module, format, ##__VA_ARGS__); } while(0)
#define MADRONA_LOG_WARN(module, format, ...) \
  do { if (MADRONA_LOG_ENABLED(3)) \
    madronavm::logging::log_entry(madronavm::logging::LogLevel::WARN, \
                                  module, format, ##__VA_ARGS__); } while(0)
#define MADRONA_LOG_ERROR(module, format, ...) \
  do { if (MADRONA_LOG_ENABLED(4)) \
    madronavm::logging::log_entry(madronavm::logging::LogLevel::ERROR, \
                                  module, format, ##__VA_ARGS__); } while(0)
// Module IDs (matching data/modules.json)
#define MADRONA_MODULE_VM     0x0001
#define MADRONA_MODULE_AUDIO  0x0002  
#define MADRONA_MODULE_DSP    0x0003
#define MADRONA_MODULE_MAIN   0x0004
// Context-specific macros
#define MADRONA_VM_LOG_ERROR(format, ...) \
  MADRONA_LOG_ERROR(MADRONA_MODULE_VM, format, ##__VA_ARGS__)
#define MADRONA_AUDIO_LOG_WARN(format, ...) \
  MADRONA_LOG_WARN(MADRONA_MODULE_AUDIO, format, ##__VA_ARGS__)
#define MADRONA_DSP_LOG_DEBUG(format, ...) \
  MADRONA_LOG_DEBUG(MADRONA_MODULE_DSP, format, ##__VA_ARGS__)
```
#### 1.2 Platform-Specific Implementation (`src/common/embedded_logging.cpp`)
```cpp
#include "common/embedded_logging.h"
#include <cstring>
namespace madronavm::logging {
// Static configuration (no heap allocation)
static LogConfig g_config;
static bool g_initialized = false;
// Lock-free circular buffer (audio-thread safe)
static LogEntry g_log_buffer[512];  // Static allocation
static volatile uint16_t g_write_index = 0;
static volatile uint16_t g_read_index = 0;
void initialize(const LogConfig& config) {
  g_config = config;
  g_write_index = 0;
  g_read_index = 0;
  // Platform-specific initialization
  #ifdef ARDUINO
    Serial.begin(g_config.baud_rate);
  #elif defined(ESP32)
    esp_log_level_set("*", ESP_LOG_INFO);
  #elif defined(STM32)
    // Initialize UART/ITM
  #endif
  g_initialized = true;
  MADRONA_LOG_INFO(MADRONA_MODULE_MAIN, "Logging initialized");
}
uint32_t get_timestamp_us() {
  #ifdef ARDUINO
    return micros();
  #elif defined(ESP32)
    return esp_timer_get_time();
  #elif defined(STM32)
    return HAL_GetTick() * 1000;  // Convert ms to us
  #else
    return 0;  // Desktop/test environment
  #endif
}
void log_entry(LogLevel level, uint16_t module_id, const char* format, 
               uint32_t arg1, uint32_t arg2) {
  if (!g_initialized || level < g_config.min_level) {
    return;
  }
  // Create log entry (no heap allocation)
  LogEntry entry;
  entry.timestamp_us = get_timestamp_us();
  entry.module_id = module_id;
  entry.level = static_cast<uint8_t>(level);
  entry.format_id = 0;  // TODO: Format string hashing for binary mode
  entry.arg1 = arg1;
  entry.arg2 = arg2;
  // Lock-free circular buffer write (audio-thread safe)
  uint16_t next_write = (g_write_index + 1) % 512;
  if (next_write != g_read_index) {  // Buffer not full
    g_log_buffer[g_write_index] = entry;
    g_write_index = next_write;
  }
  // If buffer full, drop message (overrun protection)
}
void flush() {
  while (g_read_index != g_write_index) {
    const LogEntry& entry = g_log_buffer[g_read_index];
    // Format and output log entry
    #ifdef ARDUINO
      Serial.printf("[%lu] [%04X] [%d] Message\n", 
                    entry.timestamp_us, entry.module_id, entry.level);
    #elif defined(ESP32)
      printf("[%lu] [%04X] [%d] Message\n", 
             entry.timestamp_us, entry.module_id, entry.level);
    #elif defined(STM32)
      // Output via ITM or UART
    #endif
    g_read_index = (g_read_index + 1) % 512;
  }
}
} // namespace madronavm::logging
```
### Phase 2: Systematic Replacement
#### 2.1 VM Module (`src/vm/vm.cpp`)
Replace iostream calls with embedded logging:
```cpp
// OLD:
std::cerr << "Bytecode is too small for header!" << std::endl;
// NEW (embedded-safe):
MADRONA_VM_LOG_ERROR("Bytecode too small: %lu bytes, need %lu", 
                     (uint32_t)m_bytecode.size(), 
                     (uint32_t)(sizeof(BytecodeHeader) / sizeof(uint32_t)));
```
#### 2.2 Audio Module (`src/audio/custom_audio_task.cpp`)
Replace with lock-free audio logging:
```cpp
// OLD:
std::cout << "Stream over/underflow detected." << std::endl;
// NEW (audio-thread safe):
MADRONA_AUDIO_LOG_WARN("Stream underflow: status=%lu", (uint32_t)status);
```
#### 2.3 DSP Validation (`include/dsp/validation.h`)
Replace with efficient embedded validation:
```cpp
// OLD:
std::cerr << "Error: " << module_name << " requires " << required_outputs 
          << " outputs, but got " << num_outputs << std::endl;
// NEW (no string operations):
MADRONA_DSP_LOG_ERROR("Module port mismatch: req=%lu got=%lu", 
                      (uint32_t)required_outputs, (uint32_t)num_outputs);
```
### Phase 3: Advanced Embedded Features
#### 3.1 Multiple Transport Support
```cpp
// Serial transport (default)
g_config.transport = Transport::SERIAL;
// SPI transport for external logger
g_config.transport = Transport::SPI;
// Wireless transport for remote debugging
g_config.transport = Transport::WIRELESS;
// Circular buffer for post-mortem analysis
g_config.transport = Transport::CIRCULAR;
```
#### 3.2 Binary Logging (Bandwidth Optimization)
```cpp
// Compact binary format for wireless transmission
struct __attribute__((packed)) BinaryLogEntry {
  uint16_t timestamp_ms;  // 16-bit timestamp (65s rollover)
  uint8_t module_level;   // 4-bit module, 4-bit level
  uint8_t format_id;      // Pre-defined format string ID
  uint16_t arg1;          // Compressed arguments
  uint16_t arg2;
};  // Total: 8 bytes vs 32+ bytes for text
```
#### 3.3 Power-Aware Logging
```cpp
// Power-save mode configuration
struct PowerConfig {
  bool sleep_between_flush = true;    // CPU sleep when idle
  uint32_t flush_interval_ms = 100;   // Reduce flush frequency
  LogLevel battery_level = LogLevel::WARN;  // Higher threshold on battery
  bool disable_trace_on_battery = true;    // Disable verbose logging
};
```
## Configuration Management
### Embedded Configuration
```cpp
// Compile-time configuration (no runtime overhead)
constexpr LogLevel kEmbeddedLogLevel = LogLevel::WARN;
constexpr uint16_t kCircularBufferSize = 128;  // Entries, not bytes
constexpr uint32_t kSerialBaudRate = 115200;
constexpr Transport kDefaultTransport = Transport::SERIAL;
// Platform-specific defaults
#ifdef ESP32
constexpr uint16_t kBufferSize = 256;     // More RAM available
#elif defined(TEENSY41)  
constexpr uint16_t kBufferSize = 512;     // Most RAM available
#elif defined(STM32F4)
constexpr uint16_t kBufferSize = 64;      // Limited RAM
#endif
```
### Build System Integration
```cmake
# CMake configuration for different log levels
add_compile_definitions(MADRONA_LOG_LEVEL=3)  # WARN level for release
add_compile_definitions(MADRONA_LOG_LEVEL=1)  # DEBUG level for development
# Platform-specific configuration
if(ESP32)
  add_compile_definitions(MADRONA_CIRCULAR_BUFFER_SIZE=256)
elseif(STM32)
  add_compile_definitions(MADRONA_CIRCULAR_BUFFER_SIZE=64)
endif()
```
## Testing Strategy
### Embedded-Specific Tests
```cpp
// Test memory usage constraints
TEST_CASE("Logging memory footprint", "[logging][memory]") {
  // Verify total memory usage < 2KB
  size_t memory_used = sizeof(g_log_buffer) + sizeof(LogConfig);
  REQUIRE(memory_used < 2048);
}
// Test real-time performance
TEST_CASE("Audio thread logging performance", "[logging][realtime]") {
  // Verify logging call takes < 10 microseconds
  uint32_t start = get_timestamp_us();
  MADRONA_AUDIO_LOG_WARN("Test message: %lu", 12345);
  uint32_t elapsed = get_timestamp_us() - start;
  REQUIRE(elapsed < 10);
}
// Test circular buffer overflow
TEST_CASE("Log buffer overflow protection", "[logging][buffer]") {
  // Fill buffer beyond capacity, verify no crash
  for (int i = 0; i < 1000; i++) {
    MADRONA_LOG_INFO(MADRONA_MODULE_MAIN, "Message %d", i);
  }
  // Verify system still functional
}
```
## Performance Considerations
### Embedded Performance Requirements
- **Audio Thread**: <10µs per log call (compile-time filtered)
- **Memory Usage**: <2KB total RAM overhead
- **Code Size**: <8KB flash memory for logging system
- **Power**: <1mA additional current draw for logging
### Real-time Audio Constraints  
- **Zero Allocation**: No heap allocation in audio callbacks
- **Lock-Free**: Circular buffer uses atomic operations only
- **Bounded Execution**: Maximum execution time guaranteed
- **Overflow Protection**: Buffer full detection prevents blocking
## Platform-Specific Considerations
### ESP32 Specific
```cpp
// Use ESP32 hardware timer for timestamps
#include "esp_timer.h"
uint32_t get_timestamp_us() { return esp_timer_get_time(); }
// WiFi logging transport
void setup_wifi_logging() {
  // Send logs to remote server via UDP
}
```
### Teensy 4.1 Specific  
```cpp
// Use ARM DWT cycle counter for high-precision timing
uint32_t get_timestamp_us() {
  return ARM_DWT_CYCCNT / (F_CPU / 1000000);
}
// Use USB serial for high-bandwidth logging
void setup_usb_logging() {
  SerialUSB.begin(0);  // USB native speed
}
```
### STM32 Specific
```cpp
// Use ITM (Instrumentation Trace Macrocell) for debugging
void log_via_itm(const char* message) {
  ITM_SendChar(*message);
}
// Use DMA for high-speed UART logging
void setup_dma_logging() {
  // Configure UART with DMA for non-blocking output
}
```
## Migration Timeline
### Week 1: Embedded Infrastructure
- [ ] Create lightweight embedded logging framework
- [ ] Implement circular buffer and basic transports
- [ ] Add platform-specific timing functions
- [ ] Create unit tests for memory usage and performance
### Week 2: Core Replacement
- [ ] Replace VM logging with embedded-safe calls
- [ ] Replace audio logging with lock-free implementation
- [ ] Replace DSP validation with efficient embedded logging
- [ ] Update build system for compile-time configuration
### Week 3: Advanced Embedded Features
- [ ] Add multiple transport support (SPI, I2C, wireless)
- [ ] Implement binary logging for bandwidth optimization
- [ ] Add power-aware logging modes
- [ ] Create platform-specific optimizations
### Week 4: Integration & Testing
- [ ] Test on actual embedded hardware (ESP32, Teensy, STM32)
- [ ] Measure memory usage and performance
- [ ] Validate real-time audio constraints
- [ ] Update documentation for embedded deployment
## Risk Assessment
### High Risk - Embedded Specific
- **Memory Overflow**: Circular buffer overflow in high-traffic scenarios
- **Real-time Violation**: Logging blocking audio processing
- **Power Consumption**: Excessive logging draining battery
### Medium Risk  
- **Platform Portability**: Different timing/serial APIs across platforms
- **Buffer Sizing**: Optimal buffer size varies by application
### Low Risk
- **Feature Compatibility**: Reduced feature set vs desktop logging
- **Debug Capability**: Less verbose output than desktop systems
## Future Embedded Enhancements
### Planned Additions
1. **Remote Debugging**: WiFi/Bluetooth log streaming
2. **Flash Logging**: Persistent logging to internal flash
3. **Logic Analyzer Integration**: Hardware debugging support
4. **Compressed Logging**: LZ4 compression for bandwidth-limited links
5. **Distributed Logging**: Multi-core logging for ESP32 dual-core
### Integration Opportunities
1. **RTOS Integration**: FreeRTOS task-aware logging
2. **Bootloader Logging**: Pre-main() system logging
3. **Hardware Profiling**: Integration with performance counters
4. **Error Recovery**: Automatic restart on critical errors
## Conclusion
This embedded logging system refactor provides madrona-vm with production-ready logging capabilities specifically designed for resource-constrained embedded systems. The approach prioritizes real-time safety, minimal memory usage, and deterministic performance while maintaining the debugging capabilities essential for embedded audio applications.
The lightweight design ensures compatibility with ESP32, Teensy 4.1, and STM32 platforms while providing a clear migration path from the current iostream-based system. The circular buffer approach eliminates file system dependencies and provides robust operation in embedded environments where traditional logging solutions would fail. 