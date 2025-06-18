#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <string_view>
namespace madronavm {
// Describes the inputs and outputs of a module.
// In the future, this could be loaded from configuration files.
struct ModuleInfo {
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
};
// A registry to map module names to stable IDs and provide metadata.
class ModuleRegistry {
public:
    explicit ModuleRegistry(std::string_view json_path);
    // Gets the stable ID for a given module name. Throws if not found.
    uint32_t get_id(const std::string& name) const;
    // Gets the port information for a given module name. Throws if not found.
    const ModuleInfo& get_info(const std::string& name) const;
private:
    std::map<std::string, uint32_t> name_to_id;
    std::map<std::string, ModuleInfo> name_to_info;
};
} // namespace madronavm 