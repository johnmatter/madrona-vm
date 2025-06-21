#include "common/embedded_logging.h"
#include <cstring>
#include <chrono>
namespace madronavm::logging {
// Static configuration (no heap allocation)
static LogConfig g_config;
static bool g_initialized = false;
// Lock-free circular buffer (audio-thread safe)
static LogEntry g_log_buffer[512];  // Static allocation
static volatile uint16_t g_write_index = 0;
static volatile uint16_t g_read_index = 0;
// Level names for text output
static const char* level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"
};
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
  MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Logging initialized");
}
uint32_t get_timestamp_us() {
  #ifdef ARDUINO
    return micros();
  #elif defined(ESP32)
    return esp_timer_get_time();
  #elif defined(STM32)
    return HAL_GetTick() * 1000;  // Convert ms to us
  #else
    // Desktop - use high resolution clock
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
    return static_cast<uint32_t>(duration.count());
  #endif
}
void log_entry(LogLevel level, uint16_t component_id, const char* format,
               uint32_t arg1, uint32_t arg2) {
  if (!g_initialized || level < g_config.min_level) {
    return;
  }
  // Create log entry (no heap allocation)
  LogEntry entry;
  entry.timestamp_us = get_timestamp_us();
  entry.component_id = component_id;
  entry.level = static_cast<uint8_t>(level);
  entry.padding = 0;      // Zero padding for consistent packing
  entry.format = format;  // Store pointer to format string (in flash/ROM)
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
    // Get level name
    const char* level_name = "UNKNOWN";
    if (entry.level < 6) {
      level_name = level_names[entry.level];
    }
    // Format and output log entry based on transport
    switch (g_config.transport) {
      case Transport::SERIAL:
      default:
                #ifdef ARDUINO
          Serial.printf("[%8lu] [%04X] [%s] ",
                        entry.timestamp_us, entry.component_id, level_name);
          if (entry.format) {
            Serial.printf(entry.format, entry.arg1, entry.arg2);
          } else {
            Serial.print("(null format)");
          }
          Serial.println();
        #elif defined(ESP32)
          printf("[%8lu] [%04X] [%s] ",
                 entry.timestamp_us, entry.component_id, level_name);
          if (entry.format) {
            printf(entry.format, entry.arg1, entry.arg2);
          } else {
            printf("(null format)");
          }
          printf("\n");
        #elif defined(STM32)
          // Output via ITM or UART
          // TODO: Implement STM32-specific output
        #else
          // Desktop environment
          printf("[%8u] [%04X] [%s] ",
                 entry.timestamp_us, entry.component_id, level_name);
          if (entry.format) {
            printf(entry.format, entry.arg1, entry.arg2);
          } else {
            printf("(null format)");
          }
          printf("\n");
        #endif
        break;
      case Transport::CIRCULAR:
        // Keep in circular buffer, don't output
        break;
      // TODO: Implement other transports (SPI, I2C, WIRELESS, ITM)
    }
    g_read_index = (g_read_index + 1) % 512;
  }
}
// buffer status
uint16_t get_buffer_usage() {
  uint16_t write_idx = g_write_index;
  uint16_t read_idx = g_read_index;
  if (write_idx >= read_idx) {
    return write_idx - read_idx;
  } else {
    return (512 - read_idx) + write_idx;
  }
}
bool is_buffer_full() {
  uint16_t next_write = (g_write_index + 1) % 512;
  return next_write == g_read_index;
}
void clear_buffer() {
  g_write_index = 0;
  g_read_index = 0;
}
} // namespace madronavm::logging
