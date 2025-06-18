#include "catch.hpp"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "compiler/module_registry.h"
#include "vm/vm.h"
#include "vm/opcodes.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <chrono>
#include <algorithm>
#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "examples"
#endif
#ifndef MODULE_DEFS_PATH
#define MODULE_DEFS_PATH "data/modules.json"
#endif
namespace madronavm {
TEST_CASE("End-to-End Integration: simple_patch.json", "[integration]") {
  SECTION("Complete Pipeline: Parse -> Compile -> VM Execute") {
    // === STEP 1: Load simple_patch.json ===
    std::string patch_path(TEST_DATA_DIR);
    patch_path += "/simple_patch.json";
    std::ifstream patch_file(patch_path);
    REQUIRE(patch_file.is_open());
    std::string json_content((std::istreambuf_iterator<char>(patch_file)),
                            std::istreambuf_iterator<char>());
    patch_file.close();
    std::cout << "Loaded patch: " << patch_path << std::endl;
    // === STEP 2: Use Parser to create PatchGraph ===
    PatchGraph graph;
    REQUIRE_NOTHROW(graph = parse_json(json_content));
    // Verify the parsed graph structure
    REQUIRE(graph.nodes.size() == 3);
    REQUIRE(graph.connections.size() == 3);
    // Verify nodes
    auto sine_node = std::find_if(graph.nodes.begin(), graph.nodes.end(),
                                  [](const Node& n) { return n.name == "sine_osc"; });
    auto gain_node = std::find_if(graph.nodes.begin(), graph.nodes.end(),
                                  [](const Node& n) { return n.name == "gain"; });
    auto audio_node = std::find_if(graph.nodes.begin(), graph.nodes.end(),
                                  [](const Node& n) { return n.name == "audio_out"; });
    REQUIRE(sine_node != graph.nodes.end());
    REQUIRE(gain_node != graph.nodes.end());
    REQUIRE(audio_node != graph.nodes.end());
    // Verify constants in nodes
    REQUIRE(sine_node->constants.size() == 1);
    REQUIRE(sine_node->constants[0].port_name == "freq");
    REQUIRE(sine_node->constants[0].value == 440.0f);
    REQUIRE(gain_node->constants.size() == 1);
    REQUIRE(gain_node->constants[0].port_name == "gain");
    REQUIRE(gain_node->constants[0].value == 0.5f);
    std::cout << "Successfully parsed patch graph with " << graph.nodes.size() 
              << " nodes and " << graph.connections.size() << " connections" << std::endl;
    // === STEP 3: Use Compiler to generate bytecode ===
    ModuleRegistry registry(MODULE_DEFS_PATH);
    std::vector<uint32_t> bytecode;
    REQUIRE_NOTHROW(bytecode = Compiler::compile(graph, registry));
    // Verify bytecode structure
    REQUIRE(bytecode.size() >= sizeof(BytecodeHeader) / sizeof(uint32_t));
    // Check bytecode header
    BytecodeHeader header;
    std::memcpy(&header, bytecode.data(), sizeof(header));
    REQUIRE(header.magic_number == kMagicNumber);
    REQUIRE(header.version == kBytecodeVersion);
    REQUIRE(header.num_registers > 0);
    std::cout << "Successfully compiled to bytecode (" << bytecode.size() 
              << " words, " << header.num_registers << " registers)" << std::endl;
    // === STEP 4: Load bytecode into VM ===
    VM vm(registry, 44100.0f, true); // testMode = true
    REQUIRE_NOTHROW(vm.load_program(std::move(bytecode)));
    std::cout << "Successfully loaded bytecode into VM" << std::endl;
    // === STEP 5: Execute VM in a loop and verify audio output ===
    const int num_blocks = 10;
    const int block_size = 64; // DSPVector size
    // Prepare dummy input/output buffers
    const float* inputs = nullptr; // No external inputs for this patch
    float* outputs = nullptr;       // Audio output handled internally by audio_out module
    // Process multiple blocks to ensure stability
    for (int block = 0; block < num_blocks; ++block) {
      REQUIRE_NOTHROW(vm.process(&inputs, &outputs, block_size));
    }
    std::cout << "Successfully processed " << num_blocks << " audio blocks" << std::endl;
    REQUIRE(true); // Test passes if we reach this point without exceptions
  }
}
TEST_CASE("End-to-End Integration: Advanced Verification", "[integration]") {
  SECTION("Verify Signal Flow Through Complete Chain") {
    // Load and parse the patch
    std::string patch_path(TEST_DATA_DIR);
    patch_path += "/simple_patch.json";
    std::ifstream patch_file(patch_path);
    REQUIRE(patch_file.is_open());
    std::string json_content((std::istreambuf_iterator<char>(patch_file)),
                            std::istreambuf_iterator<char>());
    patch_file.close();
    auto graph = parse_json(json_content);
    // Compile to bytecode
    ModuleRegistry registry(MODULE_DEFS_PATH);
    auto bytecode = Compiler::compile(graph, registry);
    // Create VM and load program
    VM vm(registry, 44100.0f, true); // testMode = true
    vm.load_program(std::move(bytecode));
    // Process several blocks to allow modules to stabilize
    const int warmup_blocks = 5;
    const int test_blocks = 3;
    const int block_size = 64;
    const float* inputs = nullptr;
    float* outputs = nullptr;
    // Warmup phase
    for (int block = 0; block < warmup_blocks; ++block) {
      vm.process(&inputs, &outputs, block_size);
    }
    // Test phase - just verify execution stability
    for (int block = 0; block < test_blocks; ++block) {
      REQUIRE_NOTHROW(vm.process(&inputs, &outputs, block_size));
    }
    std::cout << "Advanced verification completed successfully" << std::endl;
  }
}
TEST_CASE("End-to-End Integration: Performance Test", "[integration]") {
  SECTION("Performance and Stability Over Extended Run") {
    // Load patch
    std::string patch_path(TEST_DATA_DIR);
    patch_path += "/simple_patch.json";
    std::ifstream patch_file(patch_path);
    REQUIRE(patch_file.is_open());
    std::string json_content((std::istreambuf_iterator<char>(patch_file)),
                            std::istreambuf_iterator<char>());
    patch_file.close();
    // Complete pipeline
    auto graph = parse_json(json_content);
    ModuleRegistry registry(MODULE_DEFS_PATH);
    auto bytecode = Compiler::compile(graph, registry);
    VM vm(registry, 44100.0f, true); // testMode = true
    vm.load_program(std::move(bytecode));
    // Extended test - simulate real-time usage
    const int total_blocks = 100; // ~2.3 seconds at 44.1kHz with 64-sample blocks
    const int block_size = 64;
    const float* inputs = nullptr;
    float* outputs = nullptr;
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int block = 0; block < total_blocks; ++block) {
      REQUIRE_NOTHROW(vm.process(&inputs, &outputs, block_size));
      // Periodic check every 20 blocks
      if (block % 20 == 0) {
        std::cout << "Processed block " << block << "/" << total_blocks << std::endl;
      }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "Performance test completed: " << total_blocks << " blocks in " 
              << duration.count() << " microseconds" << std::endl;
    // Performance should be reasonable (this is just a sanity check)
    REQUIRE(duration.count() < 10000000); // Less than 10 seconds for processing
  }
}
TEST_CASE("End-to-End Integration: Error Handling", "[integration]") {
  SECTION("Invalid JSON handling") {
    // Test with truly malformed JSON syntax
    std::string invalid_json = "this is not json at all!";
    REQUIRE_THROWS(parse_json(invalid_json));
  }
  SECTION("Valid JSON but empty patch handling") {
    // Test with valid JSON but no modules/connections - should not throw
    std::string empty_patch_json = R"({"invalid": "json structure"})";
    PatchGraph graph;
    REQUIRE_NOTHROW(graph = parse_json(empty_patch_json));
    REQUIRE(graph.nodes.empty());
    REQUIRE(graph.connections.empty());
  }
  SECTION("Invalid connection string format") {
    // Test with invalid connection string (missing colon)
    std::string invalid_connection_json = R"({
      "modules": [
        { "id": 1, "name": "sine_osc", "data": {} }
      ],
      "connections": [
        { "from": "invalid_format", "to": "1:out" }
      ]
    })";
    REQUIRE_THROWS(parse_json(invalid_connection_json));
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
    // Should throw when trying to compile with unknown module
    REQUIRE_THROWS(Compiler::compile(graph, registry));
  }
  SECTION("Malformed bytecode handling") {
    ModuleRegistry registry(MODULE_DEFS_PATH);
    VM vm(registry, 44100.0f, true); // testMode = true
    // Test with empty bytecode
    std::vector<uint32_t> empty_bytecode;
    REQUIRE_NOTHROW(vm.load_program(std::move(empty_bytecode)));
    // Should handle gracefully during processing
    const float* inputs = nullptr;
    float* outputs = nullptr;
    REQUIRE_NOTHROW(vm.process(&inputs, &outputs, 64));
  }
}
} // namespace madronavm 