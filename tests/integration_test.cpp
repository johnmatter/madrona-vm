#include "catch.hpp"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "compiler/module_registry.h"
#include "vm/vm.h"
#include "vm/opcodes.h"
#include "dsp/audio_out.h"
#include "ui/device_selector.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <thread>
#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "examples"
#endif
#ifndef MODULE_DEFS_PATH
#define MODULE_DEFS_PATH "data/modules.json"
#endif
namespace madronavm {
TEST_CASE("Integration Test: Offline Pipeline", "[integration]") {
  SECTION("Complete Pipeline: Parse -> Compile -> VM Execute") {
    std::string patch_path(TEST_DATA_DIR);
    patch_path += "/simple_patch.json";
    std::ifstream patch_file(patch_path);
    REQUIRE(patch_file.is_open());
    std::string json_content((std::istreambuf_iterator<char>(patch_file)),
                             std::istreambuf_iterator<char>());
    patch_file.close();
    std::cout << "Loaded patch: " << patch_path << std::endl;
    PatchGraph graph;
    REQUIRE_NOTHROW(graph = parse_json(json_content));
    REQUIRE(graph.nodes.size() == 3);
    REQUIRE(graph.connections.size() == 3);
    ModuleRegistry registry(MODULE_DEFS_PATH);
    std::vector<uint32_t> bytecode;
    REQUIRE_NOTHROW(bytecode = Compiler::compile(graph, registry));
    REQUIRE(bytecode.size() >= sizeof(BytecodeHeader) / sizeof(uint32_t));
    BytecodeHeader header;
    std::memcpy(&header, bytecode.data(), sizeof(header));
    REQUIRE(header.magic_number == kMagicNumber);
    REQUIRE(header.version == kBytecodeVersion);
    REQUIRE(header.num_registers > 0);
    std::cout << "Successfully compiled to bytecode (" << bytecode.size()
              << " words, " << header.num_registers << " registers)" << std::endl;
    VM vm(registry, 44100.0f, true); // testMode = true
    REQUIRE_NOTHROW(vm.load_program(std::move(bytecode)));
    std::cout << "Successfully loaded bytecode into VM" << std::endl;
    const int num_blocks = 10;
    const int block_size = 64; // DSPVector size
    const float* inputs = nullptr;
    float* outputs = nullptr;
    for (int block = 0; block < num_blocks; ++block) {
      REQUIRE_NOTHROW(vm.process(&inputs, &outputs, block_size));
    }
    std::cout << "Successfully processed " << num_blocks << " audio blocks" << std::endl;
  }
}
TEST_CASE("Integration Test: Real-time Audio Driver", "[integration][realtime]") {
    constexpr float sampleRate = 48000.0f;
    const int test_duration_ms = 500; // Half a second
    // Prompt user to select an audio device
    unsigned int selected_device_id = ui::DeviceSelector::selectAudioDevice();
    if (selected_device_id == 0) {
        std::cout << "No device selected or user quit. Skipping real-time test." << std::endl;
        return;
    }
    std::string patch_path(TEST_DATA_DIR);
    patch_path += "/simple_patch.json";
    std::ifstream patch_file(patch_path);
    REQUIRE(patch_file.is_open());
    std::string json_content((std::istreambuf_iterator<char>(patch_file)),
                             std::istreambuf_iterator<char>());
    patch_file.close();
    auto graph = parse_json(json_content);
    ModuleRegistry registry(MODULE_DEFS_PATH);
    auto bytecode = Compiler::compile(graph, registry);
    // The VM will run on the audio thread.
    VM vm(registry, sampleRate, false); // testMode = false, for real audio out
    vm.load_program(std::move(bytecode));
    std::atomic<int> processBlockCount{0};
    std::atomic<int> lastBlockSize{0};
    // Create the "real" audio driver that will run on its own thread
    AudioOut audio_driver(sampleRate, false, selected_device_id); // false = not test mode
    // Link the audio driver to our real VM
    audio_driver.setVMCallback(
        [&vm, &processBlockCount, &lastBlockSize](float** outputs, int size) {
            const float* inputs = nullptr;
            vm.process(&inputs, outputs, size);
            processBlockCount++;
            lastBlockSize = size;
        }
    );
    // Let the audio run for a bit
    std::cout << "Running real-time audio test for " << test_duration_ms << "ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(test_duration_ms));
    int actual_blocks = processBlockCount.load();
    int last_block_size = lastBlockSize.load();
    // The SignalProcessBuffer inside madronalib should always call our function
    // with kFloatsPerDSPVector-sized chunks.
    REQUIRE(last_block_size == kFloatsPerDSPVector);
    // Calculate the expected number of calls.
    int expected_blocks = static_cast<int>((sampleRate * (test_duration_ms / 1000.0f)) / kFloatsPerDSPVector);
    // There can be timing variations in thread scheduling, so we check if it's within a reasonable range.
    int lower_bound = static_cast<int>(expected_blocks * 0.9);
    int upper_bound = static_cast<int>(expected_blocks * 1.1) + 2; // +2 for scheduling jitter
    std::cout << "  Real-time Test: Ran for " << test_duration_ms << "ms. "
              << "Expected blocks: ~" << expected_blocks << ". "
              << "Actual blocks: " << actual_blocks << "." << std::endl;
    REQUIRE(actual_blocks > 0);
    REQUIRE(actual_blocks >= lower_bound);
    REQUIRE(actual_blocks <= upper_bound);
}
TEST_CASE("Integration Test: Error Handling", "[integration]") {
  SECTION("Invalid JSON handling") {
    std::string invalid_json = "this is not json at all!";
    REQUIRE_THROWS(parse_json(invalid_json));
  }
  SECTION("Valid JSON but empty patch handling") {
    std::string empty_patch_json = R"({"invalid": "json structure"})";
    PatchGraph graph;
    REQUIRE_NOTHROW(graph = parse_json(empty_patch_json));
    REQUIRE(graph.nodes.empty());
    REQUIRE(graph.connections.empty());
  }
  SECTION("Missing module in registry") {
    std::string json_with_unknown_module = R"({
      "modules": [
        { "id": 1, "name": "unknown_module", "data": {} }
      ],
      "connections": []
    })";
    auto graph = parse_json(json_with_unknown_module);
    ModuleRegistry registry(MODULE_DEFS_PATH);
    REQUIRE_THROWS(Compiler::compile(graph, registry));
  }
  SECTION("Malformed bytecode handling") {
    ModuleRegistry registry(MODULE_DEFS_PATH);
    VM vm(registry, 44100.0f, true); // testMode = true
    std::vector<uint32_t> empty_bytecode;
    REQUIRE_NOTHROW(vm.load_program(std::move(empty_bytecode)));
    const float* inputs = nullptr;
    float* outputs = nullptr;
    REQUIRE_NOTHROW(vm.process(&inputs, &outputs, 64));
  }
}
} // namespace madronavm 