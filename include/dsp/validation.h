#pragma once
#include <vector>
#include <iostream>
namespace madronavm::dsp {
// A helper to verify that a module has the minimum required number
// of output ports and that all required inputs are non-null.
inline bool validate_ports(const char* module_name,
                           int num_inputs, const float** inputs, const std::vector<int>& required_inputs,
                           int num_outputs, int required_outputs) {
    if (num_outputs < required_outputs) {
        // In a real engine, use a proper logging system
        std::cerr << "Error: " << module_name << " requires " << required_outputs << " outputs, but got " << num_outputs << std::endl;
        return false;
    }
    for (int idx : required_inputs) {
        if (idx >= num_inputs || inputs[idx] == nullptr) {
            std::cerr << "Error: " << module_name << " requires a connection on input " << idx << std::endl;
            return false;
        }
    }
    return true;
}
} // namespace madronavm::dsp 