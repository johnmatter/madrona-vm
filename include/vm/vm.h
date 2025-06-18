#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <map>
#include "DSP/MLDSPOps.h"
#include "compiler/module_registry.h"
#include "dsp/module.h"
namespace madronavm {
class VM {
public:
  explicit VM(const ModuleRegistry& registry, float sampleRate = 44100.0f, bool testMode = false);
  ~VM();
  // Load a new bytecode program. This is called from the control thread.
  void load_program(std::vector<uint32_t> new_bytecode);
  // Execute the currently loaded program for one block of audio.
  // This is called from the real-time audio thread.
  void process(const float **inputs, float **outputs, int num_frames);
private:
  const ModuleRegistry& m_registry;
  float m_sampleRate;
  bool m_testMode;
  std::vector<uint32_t> m_bytecode;
  std::vector<ml::DSPVector> m_registers; // The VM's memory
  std::map<uint32_t, std::unique_ptr<DSPModule>> m_module_instances; // Module ID -> instance
  // Helper to create module instances
  std::unique_ptr<DSPModule> create_module(uint32_t module_id);
  // TODO: thread-safe mechanism for swapping bytecode
};
} // namespace madronavm
