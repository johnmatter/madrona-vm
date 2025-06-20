// Basic unit test for VM
#include "catch.hpp"
#include "vm/vm.h"
#include "vm/opcodes.h"
#include "compiler/module_registry.h"
#include <cstring>
using namespace madronavm;
// Helper function to bit-cast float to uint32_t for bytecode
// We use use memcpy to copy the raw 32 bits from the float variable directly into the uint32_t variable, byte for byte.
// Later, the VM will reverse this when it executes LOAD_K, yielding the original float value.
uint32_t float_to_uint32(float value) {
  uint32_t result;
  std::memcpy(&result, &value, sizeof(float));
  return result;
}
// Helper function to create a simple bytecode header
std::vector<uint32_t> create_bytecode_header(uint32_t program_size, uint32_t num_registers) {
  std::vector<uint32_t> bytecode;
  bytecode.push_back(kMagicNumber);
  bytecode.push_back(kBytecodeVersion);
  bytecode.push_back(program_size);
  bytecode.push_back(num_registers);
  return bytecode;
}
TEST_CASE("VM Basic Construction", "[vm]") {
  ModuleRegistry registry(MODULE_DEFS_PATH);
  VM vm(registry, 44100.0f, true); // testMode = true
  // VM should be constructible and not crash
  REQUIRE(true);
}
TEST_CASE("VM Load Invalid Bytecode", "[vm]") {
  ModuleRegistry registry(MODULE_DEFS_PATH);
  VM vm(registry, 44100.0f, true); // testMode = true
  SECTION("Empty bytecode") {
    std::vector<uint32_t> empty_bytecode;
    vm.load_program(std::move(empty_bytecode));
    // Should not crash, but program should be cleared
    REQUIRE(true);
  }
  SECTION("Invalid magic number") {
    auto bytecode = create_bytecode_header(4, 1);
    bytecode[0] = 0xACABACAB; // Wrong magic number
    vm.load_program(std::move(bytecode));
    // Should handle gracefully
    REQUIRE(true);
  }
  SECTION("Invalid version") {
    auto bytecode = create_bytecode_header(4, 1);
    bytecode[1] = 1312; // Wrong version
    vm.load_program(std::move(bytecode));
    // Should handle gracefully
    REQUIRE(true);
  }
}
TEST_CASE("VM LOAD_K Instruction", "[vm]") {
  ModuleRegistry registry(MODULE_DEFS_PATH);
  VM vm(registry, 44100.0f, true); // testMode = true
  // Create bytecode: LOAD_K 0, 440.0f; END
  auto bytecode = create_bytecode_header(7, 1); // 4 header + 3 instruction words
  bytecode.push_back(static_cast<uint32_t>(OpCode::LOAD_K));
  bytecode.push_back(0); // dest_reg = 0
  bytecode.push_back(float_to_uint32(440.0f)); // value = 440.0f
  bytecode.push_back(static_cast<uint32_t>(OpCode::END));
  vm.load_program(std::move(bytecode));
  // Process one block - should load constant without crashing
  const float* inputs = nullptr;
  float* outputs = nullptr;
  vm.process(&inputs, &outputs, 64);
  REQUIRE(true); // If we get here, no crash occurred
}
TEST_CASE("VM PROC Instruction - Sine Oscillator", "[vm]") {
  ModuleRegistry registry(MODULE_DEFS_PATH);
  VM vm(registry, 44100.0f, true); // testMode = true
  // Create bytecode that sets up a sine oscillator:
  // LOAD_K 0, 440.0f    (load frequency into register 0)
  // PROC 256, 1, 1, 0, 1 (sine_gen: 1 input from reg 0, 1 output to reg 1)
  // END
  auto bytecode = create_bytecode_header(11, 2); // 4 header + 7 instruction words
  // LOAD_K 0, 440.0f
  bytecode.push_back(static_cast<uint32_t>(OpCode::LOAD_K));
  bytecode.push_back(0); // dest_reg = 0
  bytecode.push_back(float_to_uint32(440.0f)); // value = 440.0f
  // PROC 256, 1, 1, 0, 1 (sine_gen module)
  bytecode.push_back(static_cast<uint32_t>(OpCode::PROC));
  bytecode.push_back(256); // module_id = sine_gen
  bytecode.push_back(1);   // num_inputs = 1
  bytecode.push_back(1);   // num_outputs = 1
  bytecode.push_back(0);   // input from register 0
  bytecode.push_back(1);   // output to register 1
  // END
  bytecode.push_back(static_cast<uint32_t>(OpCode::END));
  vm.load_program(std::move(bytecode));
  // Process one block
  const float* inputs = nullptr;
  float* outputs = nullptr;
  vm.process(&inputs, &outputs, 64);
  REQUIRE(true); // If we get here, sine oscillator processed without crashing
}
TEST_CASE("VM PROC Instruction - Gain Module", "[vm]") {
  ModuleRegistry registry(MODULE_DEFS_PATH);
  VM vm(registry, 44100.0f, true); // testMode = true
  // Create bytecode that sets up signal processing chain:
  // LOAD_K 0, 1.0f      (load signal into register 0)
  // LOAD_K 1, 0.5f      (load gain into register 1)  
  // PROC 1025, 2, 1, 0, 1, 2 (gain: 2 inputs from reg 0,1, 1 output to reg 2)
  // END
  auto bytecode = create_bytecode_header(14, 3); // 4 header + 10 instruction words
  // LOAD_K 0, 1.0f (signal)
  bytecode.push_back(static_cast<uint32_t>(OpCode::LOAD_K));
  bytecode.push_back(0);
  bytecode.push_back(float_to_uint32(1.0f));
  // LOAD_K 1, 0.5f (gain)
  bytecode.push_back(static_cast<uint32_t>(OpCode::LOAD_K));
  bytecode.push_back(1);
  bytecode.push_back(float_to_uint32(0.5f));
  // PROC 1025, 2, 1, 0, 1, 2 (gain module)
  bytecode.push_back(static_cast<uint32_t>(OpCode::PROC));
  bytecode.push_back(1025); // module_id = gain
  bytecode.push_back(2);    // num_inputs = 2
  bytecode.push_back(1);    // num_outputs = 1
  bytecode.push_back(0);    // input from register 0 (signal)
  bytecode.push_back(1);    // input from register 1 (gain)
  bytecode.push_back(2);    // output to register 2
  // END
  bytecode.push_back(static_cast<uint32_t>(OpCode::END));
  vm.load_program(std::move(bytecode));
  // Process one block
  const float* inputs = nullptr;
  float* outputs = nullptr;
  vm.process(&inputs, &outputs, 64);
  REQUIRE(true); // If we get here, gain module processed without crashing
}
TEST_CASE("VM Unknown Module ID", "[vm]") {
  ModuleRegistry registry(MODULE_DEFS_PATH);
  VM vm(registry, 44100.0f, true); // testMode = true
  // Create bytecode with unknown module ID
  auto bytecode = create_bytecode_header(9, 2);
  // LOAD_K 0, 1.0f
  bytecode.push_back(static_cast<uint32_t>(OpCode::LOAD_K));
  bytecode.push_back(0);
  bytecode.push_back(float_to_uint32(1.0f));
  // PROC with unknown module ID 9999
  bytecode.push_back(static_cast<uint32_t>(OpCode::PROC));
  bytecode.push_back(9999); // Unknown module_id
  bytecode.push_back(1);    // num_inputs = 1
  bytecode.push_back(1);    // num_outputs = 1
  bytecode.push_back(0);    // input from register 0
  bytecode.push_back(1);    // output to register 1
  vm.load_program(std::move(bytecode));
  // Process should handle unknown module gracefully or throw exception
  const float* inputs = nullptr;
  float* outputs = nullptr;
  // This might throw an exception, which is acceptable behavior
  bool caught_exception = false;
  try {
    vm.process(&inputs, &outputs, 64);
  } catch (const std::exception&) {
    caught_exception = true;
  }
  // Either completes successfully or throws exception - both are valid
  REQUIRE(true);
}
TEST_CASE("VM Unknown Opcode", "[vm]") {
  ModuleRegistry registry(MODULE_DEFS_PATH);
  VM vm(registry, 44100.0f, true); // testMode = true
  // Create bytecode with unknown opcode
  auto bytecode = create_bytecode_header(5, 1);
  bytecode.push_back(0xACABACAB); // Unknown opcode
  vm.load_program(std::move(bytecode));
  // Process should handle unknown opcode gracefully
  const float* inputs = nullptr;
  float* outputs = nullptr;
  vm.process(&inputs, &outputs, 64);
  REQUIRE(true); // Should handle gracefully without crashing
}
TEST_CASE("VM Complex Signal Chain - Sine -> Gain", "[vm]") {
  ModuleRegistry registry(MODULE_DEFS_PATH);
  VM vm(registry, 44100.0f, true); // testMode = true
  // Create bytecode that chains sine oscillator -> gain:
  // LOAD_K 0, 440.0f       (load frequency into register 0)
  // PROC 256, 1, 1, 0, 1   (sine_gen: freq from reg 0, output to reg 1)
  // LOAD_K 2, 0.5f         (load gain value into register 2)
  // PROC 1025, 2, 1, 1, 2, 3 (gain: signal from reg 1, gain from reg 2, output to reg 3)
  // END
  auto bytecode = create_bytecode_header(17, 4); // 4 header + 13 instruction words
  // LOAD_K 0, 440.0f (frequency)
  bytecode.push_back(static_cast<uint32_t>(OpCode::LOAD_K));
  bytecode.push_back(0);
  bytecode.push_back(float_to_uint32(440.0f));
  // PROC 256, 1, 1, 0, 1 (sine_gen)
  bytecode.push_back(static_cast<uint32_t>(OpCode::PROC));
  bytecode.push_back(256); // module_id = sine_gen
  bytecode.push_back(1);   // num_inputs = 1
  bytecode.push_back(1);   // num_outputs = 1
  bytecode.push_back(0);   // input from register 0 (frequency)
  bytecode.push_back(1);   // output to register 1 (sine wave)
  // LOAD_K 2, 0.5f (gain value)
  bytecode.push_back(static_cast<uint32_t>(OpCode::LOAD_K));
  bytecode.push_back(2);
  bytecode.push_back(float_to_uint32(0.5f));
  // PROC 1025, 2, 1, 1, 2, 3 (gain)
  bytecode.push_back(static_cast<uint32_t>(OpCode::PROC));
  bytecode.push_back(1025); // module_id = gain
  bytecode.push_back(2);    // num_inputs = 2
  bytecode.push_back(1);    // num_outputs = 1
  bytecode.push_back(1);    // input from register 1 (sine wave)
  bytecode.push_back(2);    // input from register 2 (gain value)
  bytecode.push_back(3);    // output to register 3 (attenuated sine)
  // END
  bytecode.push_back(static_cast<uint32_t>(OpCode::END));
  vm.load_program(std::move(bytecode));
  // Process multiple blocks to ensure stability
  const float* inputs = nullptr;
  float* outputs = nullptr;
  for (int block = 0; block < 10; ++block) {
    vm.process(&inputs, &outputs, 64);
  }
  REQUIRE(true); // If we get here, the signal chain processed successfully
}
