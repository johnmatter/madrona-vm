#include "dsp/threshold.h"
#include "MLDSPOps.h" // For kFloatsPerDSPVector
#include "catch.hpp"
using namespace madronavm::dsp;
TEST_CASE("Threshold DSP Module", "[dsp][threshold]") {
    constexpr float sampleRate = 48000.0f;
    Threshold threshold(sampleRate);
    SECTION("Basic threshold functionality") {
        // Create test buffers
        float signal_buffer[kFloatsPerDSPVector];
        float threshold_buffer[kFloatsPerDSPVector];
        float output_buffer[kFloatsPerDSPVector];
        // Test with threshold at 0.5
        for (int i = 0; i < kFloatsPerDSPVector; ++i) {
            signal_buffer[i] = static_cast<float>(i) / kFloatsPerDSPVector; // 0.0 to 1.0 ramp
            threshold_buffer[i] = 0.5f;
        }
        const float* inputs[2] = { signal_buffer, threshold_buffer };
        float* outputs[1] = { output_buffer };
        threshold.process(inputs, 2, outputs, 1);
        // First half should be 0.0 (below threshold), second half should be 1.0 (above threshold)
        for (int i = 0; i < kFloatsPerDSPVector; ++i) {
            if (signal_buffer[i] > 0.5f) {
                REQUIRE(output_buffer[i] == 1.0f);
            } else {
                REQUIRE(output_buffer[i] == 0.0f);
            }
        }
    }
    SECTION("Time-varying threshold") {
        float signal_buffer[kFloatsPerDSPVector];
        float threshold_buffer[kFloatsPerDSPVector];
        float output_buffer[kFloatsPerDSPVector];
        // Signal: sine wave from 0 to 1
        for (int i = 0; i < kFloatsPerDSPVector; ++i) {
            signal_buffer[i] = 0.5f + 0.5f * sinf(2.0f * M_PI * i / kFloatsPerDSPVector);
            threshold_buffer[i] = 0.7f; // Threshold at 0.7
        }
        const float* inputs[2] = { signal_buffer, threshold_buffer };
        float* outputs[1] = { output_buffer };
        threshold.process(inputs, 2, outputs, 1);
        // Check that output is 1.0 only when signal > 0.7
        for (int i = 0; i < kFloatsPerDSPVector; ++i) {
            if (signal_buffer[i] > 0.7f) {
                REQUIRE(output_buffer[i] == 1.0f);
            } else {
                REQUIRE(output_buffer[i] == 0.0f);
            }
        }
    }
} 