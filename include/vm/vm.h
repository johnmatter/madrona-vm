#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include "compiler/module_registry.h"
#include "dsp/module.h"
#include "DSP/MLDSPOps.h"
// Forward declaration
class AudioOut;
namespace madronavm {
class VM {
public:
    VM(const ModuleRegistry& registry, float sampleRate, bool testMode = false);
    ~VM();
    void load_program(std::vector<uint32_t> new_bytecode);
    void process(const float **inputs, float **outputs, int num_frames);
    void processBlock(float** outputs, int blockSize);
    void set_audio_out_module(AudioOut* pModule);
    const ml::DSPVector& getRegisterForTest(int index) const;
private:
    const ModuleRegistry& m_registry;
    std::vector<uint32_t> m_bytecode;
    std::vector<ml::DSPVector> m_registers;
    std::map<uint32_t, std::unique_ptr<DSPModule>> m_module_instances;
    float m_sampleRate;
    bool m_testMode;
    AudioOut* m_audio_out_module = nullptr;
    std::unique_ptr<DSPModule> create_module(uint32_t module_id);
};
} // namespace madronavm
