#pragma once
#include <vector>
#include "common/embedded_logging.h"
namespace madronavm::dsp {
// A helper to verify that a module has the minimum required number
// of output ports and that all required inputs are non-null.
inline bool validate_ports(const char* module_name,
                           int num_inputs, const float** inputs, const std::vector<int>& required_inputs,
                           int num_outputs, int required_outputs) {
    if (num_outputs < required_outputs) {
        MADRONA_DSP_LOG_ERROR("Port mismatch: req=%u got=%u", 
                              (uint32_t)required_outputs, (uint32_t)num_outputs);
        return false;
    }
    for (int idx : required_inputs) {
        if (idx >= num_inputs || inputs[idx] == nullptr) {
            MADRONA_DSP_LOG_ERROR("Missing input connection: idx=%u", (uint32_t)idx);
            return false;
        }
    }
    return true;
}
} // namespace madronavm::dsp 