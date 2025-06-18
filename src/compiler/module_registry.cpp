#include "compiler/module_registry.h"
#include <stdexcept>
namespace madronavm {
ModuleRegistry::ModuleRegistry() {
    // Populate the registry with module data based on the design spec
    // and the modules present in `simple_patch.json`.
    // --- Module IDs ---
    // Note: 'sine_osc' from the patch maps to 'SineGen' from the spec.
    // 'gain' maps to 'Multiply'.
    name_to_id["sine_osc"] = 0x100;
    name_to_id["gain"] = 0x401;
    name_to_id["audio_out"] = 0x001;
    // --- Module Port Information ---
    name_to_info["sine_osc"] = { {"freq"}, {"out"} };
    name_to_info["gain"] = { {"in", "gain"}, {"out"} };
    name_to_info["audio_out"] = { {"in_l", "in_r"}, {} };
}
uint32_t ModuleRegistry::get_id(const std::string& name) const {
    auto it = name_to_id.find(name);
    if (it == name_to_id.end()) {
        throw std::runtime_error("Unknown module name: " + name);
    }
    return it->second;
}
const ModuleInfo& ModuleRegistry::get_info(const std::string& name) const {
    auto it = name_to_info.find(name);
    if (it == name_to_info.end()) {
        throw std::runtime_error("Could not find info for module: " + name);
    }
    return it->second;
}
} // namespace madronavm 