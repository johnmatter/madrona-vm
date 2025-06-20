#include "catch.hpp"
#include "dsp/phasor_gen.h"
#include "MLDSPGens.h" // For kFloatsPerDSPVector
#include <vector>
#include <cmath>
using namespace madronavm;
using namespace madronavm::dsp;
constexpr int sampleRate = 48000;
constexpr int bufferSize = 256;
constexpr float freq = 10.0f;
TEST_CASE("PhasorGen DSP Module", "[dsp][phasor_gen]") {
    auto phasor = PhasorGen(sampleRate);
    std::vector<float> freqBuffer(bufferSize, freq); // 10 Hz
    std::vector<float> outputBuffer(bufferSize, 0.0f);
    const float* inputs[] = { freqBuffer.data() };
    float* outputs[] = { outputBuffer.data() };
    phasor.process(inputs, 1, outputs, 1);
    SECTION("Output is within [0, 1] range") {
        for (int i = 0; i < kFloatsPerDSPVector; ++i) {
            REQUIRE(outputBuffer[i] >= 0.0f);
            REQUIRE(outputBuffer[i] <= 1.0f);
        }
    }
    SECTION("Output is not constant zero") {
        bool all_zero = true;
        for (int i = 0; i < kFloatsPerDSPVector; ++i) {
            if (outputBuffer[i] != 0.0f) {
                all_zero = false;
                break;
            }
        }
        REQUIRE_FALSE(all_zero);
    }
    SECTION("Output is monotonically increasing (with wrap around)") {
        const float expected_delta = freq / sampleRate;
        const float tolerance = expected_delta * 0.1f; // 10% tolerance
        for (int i = 1; i < kFloatsPerDSPVector; ++i) {
            float measured_delta = outputBuffer[i] - outputBuffer[i-1];
            // For a positive frequency, the delta should be positive.
            bool is_normal_increase = (measured_delta > 0) && (std::abs(measured_delta - expected_delta) < tolerance);
            // When wrapping around, the delta should be negative and close to -1.
            bool is_wrap_around = (measured_delta < 0) && (std::abs(measured_delta - (expected_delta - 1.0f)) < tolerance);
            REQUIRE((is_normal_increase || is_wrap_around));
        }
    }
} 