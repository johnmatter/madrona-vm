#include "common/embedded_logging.h"
#include <iostream>
using namespace madronavm::logging;
int main() {
  std::cout << "=== Madrona VM Embedded Logging Demo ===\n" << std::endl;
  // Initialize logging system
  LogConfig config;
  config.min_level = LogLevel::DEBUG;
  config.transport = Transport::SERIAL;
  initialize(config);
  // various logging levels
  MADRONA_LOG_DEBUG(MADRONA_COMPONENT_MAIN, "Debug: System initialized with sample rate %u Hz", 44100);
  MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Info: Loading patch with %u modules", 5);
  MADRONA_LOG_WARN(MADRONA_COMPONENT_AUDIO, "Warning: Audio buffer underrun, %u samples lost", 128);
  MADRONA_LOG_ERROR(MADRONA_COMPONENT_VM, "Error: Bytecode validation failed at address 0x%04X", 0x1234);
  MADRONA_LOG_CRITICAL(MADRONA_COMPONENT_DSP, "Critical: DSP module %u crashed with error %u", 42, 500);
  // component-specific macros
  MADRONA_VM_LOG_ERROR("VM error: Unknown opcode 0x%02X at PC=%u", 0xFF, 1024);
  MADRONA_AUDIO_LOG_WARN("Audio: Device disconnected, attempting reconnect in %u ms", 2000);
  MADRONA_DSP_LOG_DEBUG("DSP: Processing block with %u samples at %u Hz", 512, 48000);
  // typical usecases
  // VM
  uint32_t bytecode_size = 2048;
  uint32_t required_size = 4096;
  MADRONA_VM_LOG_ERROR("Bytecode too small: %u bytes, need %u", bytecode_size, required_size);
  // audio
  uint32_t buffer_size = 256;
  uint32_t sample_rate = 44100;
  MADRONA_AUDIO_LOG_INFO("Audio stream started: %u samples @ %u Hz", buffer_size, sample_rate);
  // DSP module validation
  uint32_t required_outputs = 2;
  uint32_t actual_outputs = 1;
  MADRONA_DSP_LOG_ERROR("Module port mismatch: req=%u got=%u", required_outputs, actual_outputs);
  // flush all pending messages
  flush();
  // buffer statistics
  std::cout << std::endl;
  std::cout << "Logging system statistics:" << std::endl;
  std::cout << "- LogEntry size: " << sizeof(LogEntry) << " bytes" << std::endl;
  std::cout << "- Buffer usage: " << get_buffer_usage() << " entries" << std::endl;
  std::cout << "- Buffer full: " << (is_buffer_full() ? "Yes" : "No") << std::endl;
  std::cout << std::endl;
  return 0;
}
