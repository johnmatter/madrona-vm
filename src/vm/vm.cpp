// Virtual machine implementation
#include "vm/vm.h"
#include "vm/opcodes.h"
#include "dsp/sine_gen.h"
#include "dsp/gain.h"
#include "dsp/audio_out.h"
#include <iostream>
namespace madronavm {
VM::VM(const ModuleRegistry& registry, float sampleRate, bool testMode) 
  : m_registry(registry), m_sampleRate(sampleRate), m_testMode(testMode) {}
VM::~VM() {}
std::unique_ptr<DSPModule> VM::create_module(uint32_t module_id) {
  // Map module IDs to their implementations based on data/modules.json
  switch (module_id) {
    case 256: // sine_osc
      return std::make_unique<::SineGen>(m_sampleRate);
    case 1025: // gain  
      return std::make_unique<::Gain>(m_sampleRate);
    case 1: // audio_out
      return std::make_unique<::AudioOut>(m_sampleRate, m_testMode);
    default:
      throw std::runtime_error("Unknown module ID: " + std::to_string(module_id));
  }
}
void VM::load_program(std::vector<uint32_t> new_bytecode) {
  // TODO: make this thread-safe
  m_bytecode = std::move(new_bytecode);
  // Clear any existing module instances
  m_module_instances.clear();
  if (m_bytecode.size() < sizeof(BytecodeHeader) / sizeof(uint32_t)) {
    // TODO: error handling
    std::cerr << "Bytecode is too small for header!" << std::endl;
    m_bytecode.clear();
    return;
  }
  auto* header = reinterpret_cast<const BytecodeHeader*>(m_bytecode.data());
  if (header->magic_number != kMagicNumber) {
    std::cerr << "Invalid bytecode magic number!" << std::endl;
    m_bytecode.clear();
    return;
  }
  if (header->version != kBytecodeVersion) {
    std::cerr << "Bytecode version mismatch!" << std::endl;
    m_bytecode.clear();
    return;
  }
  m_registers.resize(header->num_registers);
}
void VM::process(const float **inputs, float **outputs, int num_frames) {
  if (m_bytecode.empty()) {
    return;
  }
  size_t pc = sizeof(BytecodeHeader) / sizeof(uint32_t);
  while (pc < m_bytecode.size()) {
    OpCode opcode = static_cast<OpCode>(m_bytecode[pc]);
    switch (opcode) {
    case OpCode::LOAD_K: {
      uint32_t dest_reg = m_bytecode[pc + 1];
      uint32_t value_bits = m_bytecode[pc + 2];
      float value = *reinterpret_cast<float*>(&value_bits);
      m_registers[dest_reg] = value;
      pc += 3;
      break;
    }
    case OpCode::PROC: {
      uint32_t module_id = m_bytecode[pc + 1];
      uint32_t num_inputs = m_bytecode[pc + 2];
      uint32_t num_outputs = m_bytecode[pc + 3];
      // Create module instance if it doesn't exist
      if (m_module_instances.find(module_id) == m_module_instances.end()) {
        m_module_instances[module_id] = create_module(module_id);
      }
      // Gather input register pointers
      std::vector<const float*> input_ptrs(num_inputs);
      for (uint32_t i = 0; i < num_inputs; ++i) {
        uint32_t reg_idx = m_bytecode[pc + 4 + i];
        input_ptrs[i] = m_registers[reg_idx].getConstBuffer();
      }
      // Gather output register pointers  
      std::vector<float*> output_ptrs(num_outputs);
      for (uint32_t i = 0; i < num_outputs; ++i) {
        uint32_t reg_idx = m_bytecode[pc + 4 + num_inputs + i];
        output_ptrs[i] = m_registers[reg_idx].getBuffer();
      }
      // Call the module's process method
      m_module_instances[module_id]->process(input_ptrs.data(), output_ptrs.data());
      pc += 4 + num_inputs + num_outputs;
      break;
    }
    case OpCode::END: {
      return; // End of program for this block
    }
    default: {
      // TODO: error handling for unknown opcode
      std::cerr << "Unknown opcode: " << std::hex << m_bytecode[pc] << std::endl;
      return;
    }
    }
  }
}
} // namespace madronavm
