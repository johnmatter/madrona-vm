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
// Log entry structure (optimized for embedded)
struct __attribute__((packed)) LogEntry {
  uint32_t timestamp_us;  // Microsecond timestamp
  uint16_t component_id;  // System component identifier
  uint8_t level;          // Log level
  uint8_t padding;        // Alignment padding
  const char* format;     // Format string pointer (stored in flash/ROM)
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
void log_entry(
  LogLevel level,
  uint16_t component_id,
  const char* format,
  uint32_t arg1 = 0,
  uint32_t arg2 = 0
);
// Get current microsecond timestamp (platform-specific)
uint32_t get_timestamp_us();
// Flush pending log entries (call from main loop)
void flush();
// Buffer management functions
uint16_t get_buffer_usage();
bool is_buffer_full();
void clear_buffer();
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
#define MADRONA_LOG_CRITICAL(module, format, ...) \
  do { if (MADRONA_LOG_ENABLED(5)) \
    madronavm::logging::log_entry(madronavm::logging::LogLevel::CRITICAL, \
                                  module, format, ##__VA_ARGS__); } while(0)
// System component IDs for logging (separate from DSP module IDs in data/modules.json)
#define MADRONA_COMPONENT_VM     0x0001
#define MADRONA_COMPONENT_AUDIO  0x0002
#define MADRONA_COMPONENT_DSP    0x0003
#define MADRONA_COMPONENT_MAIN   0x0004
#define MADRONA_COMPONENT_COMPILER 0x0005
#define MADRONA_COMPONENT_PARSER   0x0006
// Context-specific macros
#define MADRONA_VM_LOG_ERROR(format, ...) \
  MADRONA_LOG_ERROR(MADRONA_COMPONENT_VM, format, ##__VA_ARGS__)
#define MADRONA_VM_LOG_WARN(format, ...) \
  MADRONA_LOG_WARN(MADRONA_COMPONENT_VM, format, ##__VA_ARGS__)
#define MADRONA_VM_LOG_INFO(format, ...) \
  MADRONA_LOG_INFO(MADRONA_COMPONENT_VM, format, ##__VA_ARGS__)
#define MADRONA_AUDIO_LOG_WARN(format, ...) \
  MADRONA_LOG_WARN(MADRONA_COMPONENT_AUDIO, format, ##__VA_ARGS__)
#define MADRONA_AUDIO_LOG_INFO(format, ...) \
  MADRONA_LOG_INFO(MADRONA_COMPONENT_AUDIO, format, ##__VA_ARGS__)
#define MADRONA_DSP_LOG_ERROR(format, ...) \
  MADRONA_LOG_ERROR(MADRONA_COMPONENT_DSP, format, ##__VA_ARGS__)
#define MADRONA_DSP_LOG_DEBUG(format, ...) \
  MADRONA_LOG_DEBUG(MADRONA_COMPONENT_DSP, format, ##__VA_ARGS__)
#define MADRONA_COMPILER_LOG_ERROR(format, ...) \
  MADRONA_LOG_ERROR(MADRONA_COMPONENT_COMPILER, format, ##__VA_ARGS__)
#define MADRONA_PARSER_LOG_ERROR(format, ...) \
  MADRONA_LOG_ERROR(MADRONA_COMPONENT_PARSER, format, ##__VA_ARGS__)
// Platform-specific buffer sizes
#ifdef ESP32
constexpr uint16_t kDefaultBufferSize = 256;
#elif defined(TEENSY41)
constexpr uint16_t kDefaultBufferSize = 512;
#elif defined(STM32F4)
constexpr uint16_t kDefaultBufferSize = 64;
#else
// Desktop default smaller than teensy to force my hand and optimize earlier
constexpr uint16_t kDefaultBufferSize = 256;
#endif
