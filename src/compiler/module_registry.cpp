#include "compiler/module_registry.h"
#include "cJSON.h"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <vector>
namespace madronavm {
ModuleRegistry::ModuleRegistry(std::string_view json_path) {
    std::ifstream t(json_path.data());
    if (!t.is_open()) {
        throw std::runtime_error(std::string("Failed to open module registry file: ") + json_path.data());
    }
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    cJSON* root = cJSON_Parse(str.c_str());
    if (!root) {
        throw std::runtime_error("Failed to parse module registry JSON");
    }
    cJSON* modules = cJSON_GetObjectItem(root, "modules");
    if (!modules || modules->type != cJSON_Array) {
        cJSON_Delete(root);
        throw std::runtime_error("Invalid module registry format: 'modules' array not found.");
    }
    int module_count = cJSON_GetArraySize(modules);
    for (int i = 0; i < module_count; ++i) {
        cJSON* module_item = cJSON_GetArrayItem(modules, i);
        if (!module_item) continue;
        cJSON* name_item = cJSON_GetObjectItem(module_item, "name");
        cJSON* id_item = cJSON_GetObjectItem(module_item, "id");
        cJSON* info_item = cJSON_GetObjectItem(module_item, "info");
        if (!name_item || !id_item || !info_item || name_item->type != cJSON_String) {
             continue; // Skip malformed entries
        }
        std::string name = name_item->valuestring;
        uint32_t id = id_item->valueint;
        ModuleInfo info;
        cJSON* inputs = cJSON_GetObjectItem(info_item, "inputs");
        if (inputs && inputs->type == cJSON_Array) {
            for (int j = 0; j < cJSON_GetArraySize(inputs); ++j) {
                cJSON* input_item = cJSON_GetArrayItem(inputs, j);
                if (input_item && input_item->type == cJSON_String) {
                    info.inputs.push_back(input_item->valuestring);
                }
            }
        }
        cJSON* outputs = cJSON_GetObjectItem(info_item, "outputs");
        if (outputs && outputs->type == cJSON_Array) {
            for (int j = 0; j < cJSON_GetArraySize(outputs); ++j) {
                cJSON* output_item = cJSON_GetArrayItem(outputs, j);
                if (output_item && output_item->type == cJSON_String) {
                    info.outputs.push_back(output_item->valuestring);
                }
            }
        }
        name_to_id[name] = id;
        name_to_info[name] = info;
    }
    cJSON_Delete(root);
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