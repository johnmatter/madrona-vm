#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include "compiler/module_registry.h"
#include "parser/patch_graph.h"
#include "dsp/module.h"
#include "DSP/MLDSPOps.h"
namespace madronavm {
// Forward declaration
class AudioOut;
namespace dsp {
  class DSPModule;
}
class VM {
public:
    VM(const ModuleRegistry& registry, float sampleRate, bool testMode = false);
    ~VM();
    void load_program(std::vector<uint32_t> new_bytecode);
    void process(const float **inputs, float **outputs, int num_frames);
    void processBlock(float** outputs, int blockSize);
    void set_audio_out_module(AudioOut* pModule);
    const ml::DSPVector& getRegisterForTest(int index) const;
    void process(const PatchGraph* graph);
    float* get_output_buffer(int channel) const;
private:
    const ModuleRegistry& m_registry;
    std::vector<uint32_t> m_bytecode;
    std::vector<ml::DSPVector> m_registers;
    std::map<uint32_t, std::unique_ptr<dsp::DSPModule>> m_module_instances;
    float m_sampleRate;
    bool m_testMode;
    AudioOut* m_audio_out_module = nullptr;
    std::unique_ptr<dsp::DSPModule> create_module(uint32_t module_id);
    void execute_bytecode(const std::vector<uint32_t>& bytecode);
    std::vector<float> m_vm_memory;
};
} // namespace madronavm
