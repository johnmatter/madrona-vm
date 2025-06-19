#include "compiler/compiler.h"
#include "parser/parser.h"
#include "compiler/module_registry.h"
#include "catch.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "vm/opcodes.h"
#include <cstring>
#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "examples"
#endif
#ifndef MODULE_DEFS_PATH
#define MODULE_DEFS_PATH "data/modules.json"
#endif
TEST_CASE("Compiler correctly performs topological sort", "[compiler]") {
    std::string path(TEST_DATA_DIR);
    path += "/simple_patch.json";
    std::ifstream t(path);
    if (!t.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        throw std::runtime_error("Could not open test patch file: " + path);
    }
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    auto graph = madronavm::parse_json(str);
    auto sorted_nodes = madronavm::Compiler::topological_sort(graph);
    REQUIRE(sorted_nodes.size() == 3);
    REQUIRE(sorted_nodes[0] == 1);
    REQUIRE(sorted_nodes[1] == 2);
    REQUIRE(sorted_nodes[2] == 3);
}
TEST_CASE("Compiler detects cycles in graph", "[compiler]") {
    madronavm::PatchGraph graph;
    graph.nodes = { {1, "sine_osc", {}}, {2, "gain", {}} };
    graph.connections = {
        {1, "out", 2, "in"},
        {2, "out", 1, "freq"} // Cycle back
    };
    REQUIRE_THROWS(madronavm::Compiler::topological_sort(graph));
}
TEST_CASE("Compiler correctly generates bytecode", "[compiler]") {
    // 1. Load the module definitions
    madronavm::ModuleRegistry registry(MODULE_DEFS_PATH);
    // 2. Load the test patch
    std::string path(TEST_DATA_DIR);
    path += "/simple_patch.json";
    std::ifstream t(path);
    if (!t.is_open()) {
        throw std::runtime_error("Could not open test patch file: " + path);
    }
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    // 3. Compile it
    auto graph = madronavm::parse_json(str);
    auto bytecode = madronavm::Compiler::compile(graph, registry);
    // 4. Verify the bytecode
    REQUIRE(bytecode.size() >= sizeof(madronavm::BytecodeHeader) / sizeof(uint32_t));
    // Check header
    madronavm::BytecodeHeader header;
    std::memcpy(&header, bytecode.data(), sizeof(header));
    REQUIRE(header.magic_number == madronavm::kMagicNumber);
    REQUIRE(header.version == madronavm::kBytecodeVersion);
    REQUIRE(header.num_registers == 4);
    // Check instructions
    // This is the expected sequence of opcodes and operands after the header.
    float freq_val = 440.0f;
    uint32_t freq_as_u32;
    std::memcpy(&freq_as_u32, &freq_val, sizeof(freq_val));
    float gain_val = 0.5f;
    uint32_t gain_as_u32;
    std::memcpy(&gain_as_u32, &gain_val, sizeof(gain_val));
    std::vector<uint32_t> expected_instructions = {
        // Node 1: sine_osc (ID 0x100 = 256)
        (uint32_t)madronavm::OpCode::LOAD_K, 0, freq_as_u32,          // LOAD_K r0, 440.0
        (uint32_t)madronavm::OpCode::PROC, 256, 1, 1, 0, 1,          // PROC sine_osc in:[r0] out:[r1]
        // Node 2: gain (ID 0x401 = 1025)
        (uint32_t)madronavm::OpCode::LOAD_K, 2, gain_as_u32,           // LOAD_K r2, 0.5
        (uint32_t)madronavm::OpCode::PROC, 1025, 2, 1, 1, 2, 3,       // PROC gain in:[r1, r2] out:[r3]
        // Node 3: audio_out
        (uint32_t)madronavm::OpCode::AUDIO_OUT, 2, 3, 3,          // AUDIO_OUT 2 in:[r3, r3]
        // End of program
        (uint32_t)madronavm::OpCode::END
    };
    REQUIRE(header.program_size_words == expected_instructions.size() + (sizeof(madronavm::BytecodeHeader) / sizeof(uint32_t)));
    // Compare the instructions part of the bytecode
    std::vector<uint32_t> actual_instructions(bytecode.begin() + (sizeof(madronavm::BytecodeHeader) / sizeof(uint32_t)), bytecode.end());
    REQUIRE(actual_instructions == expected_instructions);
}
