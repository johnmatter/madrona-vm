#include "catch.hpp"
#include "common/embedded_logging.h"
#include <chrono>
#include <thread>
using namespace madronavm::logging;
// Test helper to capture stdout
class StdoutCapture {
public:
  StdoutCapture() {
    original_stdout = stdout;
    stdout = tmpfile();
  }
  ~StdoutCapture() {
    fclose(stdout);
    stdout = original_stdout;
  }
  std::string getOutput() {
    fflush(stdout);
    rewind(stdout);
    std::string output;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), stdout)) {
      output += buffer;
    }
    return output;
  }
private:
  FILE* original_stdout;
};
TEST_CASE("Embedded logging initialization", "[logging][initialization]") {
  SECTION("Default configuration") {
    LogConfig config;
    REQUIRE(config.min_level == LogLevel::INFO);
    REQUIRE(config.transport == Transport::SERIAL);
    REQUIRE(config.buffer_size == 256);
    REQUIRE(config.binary_mode == false);
    REQUIRE(config.power_save == false);
    REQUIRE(config.baud_rate == 115200);
  }
  SECTION("Custom configuration") {
    LogConfig config;
    config.min_level = LogLevel::DEBUG;
    config.transport = Transport::CIRCULAR;
    config.buffer_size = 128;
    config.binary_mode = true;
    config.power_save = true;
    config.baud_rate = 9600;
    initialize(config);
    // Verify initialization doesn't crash
    REQUIRE(true);
  }
}
TEST_CASE("Logging memory footprint", "[logging][memory]") {
  SECTION("Memory usage constraints") {
    // Verify log entry structure size (depends on pointer size)
    size_t expected_entry_size = sizeof(uint32_t) + sizeof(uint16_t) + 2 * sizeof(uint8_t) + 
                                sizeof(const char*) + 2 * sizeof(uint32_t);
    REQUIRE(sizeof(LogEntry) == expected_entry_size);
    // Should be reasonably compact (20-24 bytes on most platforms)
    REQUIRE(sizeof(LogEntry) <= 32);  
    // Verify total static memory usage
    size_t buffer_size = sizeof(LogEntry) * 512;  // Static buffer
    size_t config_size = sizeof(LogConfig) + sizeof(bool) + 2 * sizeof(uint16_t);
    size_t total_memory = buffer_size + config_size;
    REQUIRE(total_memory < 16384);  // Less than 16KB total (increased for pointer storage)
  }
  SECTION("Log entry structure packing") {
    LogEntry entry;
    entry.timestamp_us = 1000000;
    entry.component_id = 0x1234;
    entry.level = 3;
    entry.padding = 0;
    entry.format = "Test format string";
    entry.arg1 = 0xDEADBEEF;
    entry.arg2 = 0xCAFEBABE;
    // Verify structure size (depends on pointer size)
    size_t expected_size = sizeof(uint32_t) + sizeof(uint16_t) + 2 * sizeof(uint8_t) + 
                          sizeof(const char*) + 2 * sizeof(uint32_t);
    REQUIRE(sizeof(entry) == expected_size);
    // Verify format string is stored correctly
    REQUIRE(entry.format != nullptr);
    REQUIRE(std::string(entry.format) == "Test format string");
  }
}
TEST_CASE("Circular buffer operations", "[logging][buffer]") {
  LogConfig config;
  config.transport = Transport::CIRCULAR;  // Don't output to stdout
  initialize(config);
  SECTION("Buffer overflow protection") {
    // Fill buffer beyond capacity
    for (int i = 0; i < 600; i++) {
      MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Message %d", i);
    }
    // Verify no crash and buffer is manageable
    REQUIRE(get_buffer_usage() < 512);
  }
  SECTION("Buffer status functions") {
    clear_buffer();
    REQUIRE(get_buffer_usage() == 0);
    REQUIRE_FALSE(is_buffer_full());
    // Add some entries
    for (int i = 0; i < 10; i++) {
      MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Test message %d", i);
    }
    REQUIRE(get_buffer_usage() == 10);
    REQUIRE_FALSE(is_buffer_full());
  }
  SECTION("Circular buffer wraparound") {
    clear_buffer();
    // Fill most of the buffer
    for (int i = 0; i < 510; i++) {
      MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Message %d", i);
    }
    REQUIRE(get_buffer_usage() == 510);
    // Add one more (should not overflow)
    MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Last message");
    REQUIRE(get_buffer_usage() == 511);
    // This should cause overflow protection
    MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Overflow message");
    REQUIRE(get_buffer_usage() == 511);  // Should stay at 511, not increase
  }
}
TEST_CASE("Log level filtering", "[logging][levels]") {
  LogConfig config;
  config.min_level = LogLevel::WARN;
  config.transport = Transport::CIRCULAR;
  initialize(config);
  clear_buffer();
  SECTION("Compile-time filtering") {
    // These should be filtered out at compile time if MADRONA_LOG_LEVEL is set high
    MADRONA_LOG_TRACE(MADRONA_COMPONENT_MAIN, "Trace message");
    MADRONA_LOG_DEBUG(MADRONA_COMPONENT_MAIN, "Debug message");
    MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Info message");
    // These should pass through
    MADRONA_LOG_WARN(MADRONA_COMPONENT_MAIN, "Warning message");
    MADRONA_LOG_ERROR(MADRONA_COMPONENT_MAIN, "Error message");
    MADRONA_LOG_CRITICAL(MADRONA_COMPONENT_MAIN, "Critical message");
    // Only WARN and above should be in buffer (3 messages)
    flush();  // Process any messages
    // Note: Actual filtering depends on compile-time MADRONA_LOG_LEVEL setting
    // At runtime, we should see runtime filtering working
  }
  SECTION("Runtime filtering") {
    clear_buffer();
    // Direct logging function call (bypasses compile-time filtering)
    log_entry(LogLevel::DEBUG, MADRONA_COMPONENT_MAIN, "Debug message");
    log_entry(LogLevel::INFO, MADRONA_COMPONENT_MAIN, "Info message");
    log_entry(LogLevel::WARN, MADRONA_COMPONENT_MAIN, "Warning message");
    log_entry(LogLevel::ERROR, MADRONA_COMPONENT_MAIN, "Error message");
    // Should only have WARN and ERROR (2 messages) due to min_level = WARN
    REQUIRE(get_buffer_usage() == 2);
  }
}
TEST_CASE("Performance requirements", "[logging][performance]") {
  LogConfig config;
  config.transport = Transport::CIRCULAR;
  initialize(config);
  SECTION("Logging call performance") {
    // Measure time for a single log call
    auto start = std::chrono::high_resolution_clock::now();
    MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Performance test message");
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // Should take less than 50 microseconds (very conservative for audio thread)
    REQUIRE(duration.count() < 50);
  }
  SECTION("Bulk logging performance") {
    clear_buffer();
    auto start = std::chrono::high_resolution_clock::now();
    // Log 100 messages
    for (int i = 0; i < 100; i++) {
      MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Bulk message %d", i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // Should be very fast - less than 1ms for 100 messages
    REQUIRE(duration.count() < 1000);
  }
}
TEST_CASE("Component-specific logging macros", "[logging][components]") {
  LogConfig config;
  config.transport = Transport::CIRCULAR;
  initialize(config);
  clear_buffer();
  SECTION("Component-specific macros work") {
    MADRONA_VM_LOG_ERROR("VM error message");
    MADRONA_AUDIO_LOG_WARN("Audio warning message");
    MADRONA_DSP_LOG_DEBUG("DSP debug message");
    MADRONA_COMPILER_LOG_ERROR("Compiler error message");
    MADRONA_PARSER_LOG_ERROR("Parser error message");
    // Should have logged some messages (exact count depends on log level)
    REQUIRE(get_buffer_usage() > 0);
  }
}
TEST_CASE("Timestamp functionality", "[logging][timestamp]") {
  SECTION("Timestamp generation") {
    uint32_t timestamp1 = get_timestamp_us();
    // Small delay
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    uint32_t timestamp2 = get_timestamp_us();
    // Second timestamp should be later
    REQUIRE(timestamp2 > timestamp1);
    // Difference should be reasonable (between 50-1000 microseconds)
    uint32_t diff = timestamp2 - timestamp1;
    REQUIRE(diff >= 50);
    REQUIRE(diff < 1000);
  }
}
TEST_CASE("Format string functionality", "[logging][format]") {
  LogConfig config;
  config.transport = Transport::CIRCULAR;  // Don't output to stdout
  initialize(config);
  clear_buffer();
  SECTION("Format strings are stored correctly") {
    const char* test_format = "Test value: %u, status: %u";
    log_entry(LogLevel::INFO, MADRONA_COMPONENT_MAIN, test_format, 42, 100);
    REQUIRE(get_buffer_usage() == 1);
    // Access the log entry (normally private, but for testing)
    // We can verify the format pointer is stored correctly by testing output
    flush();  // This should use the format string
    REQUIRE(get_buffer_usage() == 0);
  }
  SECTION("Different format strings work") {
    log_entry(LogLevel::ERROR, MADRONA_COMPONENT_VM, "VM error: code=%u", 404);
    log_entry(LogLevel::WARN, MADRONA_COMPONENT_AUDIO, "Audio underrun: samples=%u", 1024);
    log_entry(LogLevel::INFO, MADRONA_COMPONENT_DSP, "DSP processing: rate=%u Hz", 44100);
    REQUIRE(get_buffer_usage() == 3);
    // All should flush successfully
    flush();
    REQUIRE(get_buffer_usage() == 0);
  }
  SECTION("Null format string handling") {
    log_entry(LogLevel::WARN, MADRONA_COMPONENT_MAIN, nullptr, 123, 456);
    REQUIRE(get_buffer_usage() == 1);
    // Should not crash when flushing null format
    flush();
    REQUIRE(get_buffer_usage() == 0);
  }
}
TEST_CASE("Flush functionality", "[logging][flush]") {
  LogConfig config;
  config.transport = Transport::SERIAL;
  initialize(config);
  clear_buffer();
  SECTION("Flush processes all messages") {
    // Add some messages
    for (int i = 0; i < 5; i++) {
      MADRONA_LOG_INFO(MADRONA_COMPONENT_MAIN, "Message %d", i);
    }
    uint16_t messages_before = get_buffer_usage();
    REQUIRE(messages_before == 5);
    // Flush should process all messages
    flush();
    uint16_t messages_after = get_buffer_usage();
    REQUIRE(messages_after == 0);
  }
} 