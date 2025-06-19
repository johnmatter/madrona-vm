#include "dsp/adsr.h"
#include "catch.hpp"
#include <vector>
// Helper function to process a block and check a condition
void processAndCheck(ADSR& adsr, const float* inputs[], float* outputs[],
                     int numSamples, float expectedValue, bool check) {
    for (int i = 0; i < numSamples / kFloatsPerDSPVector; ++i) {
        adsr.process(inputs, 5, outputs, 1);
    }
    if (check) {
        // Check the last sample of the last block
        REQUIRE(outputs[0][kFloatsPerDSPVector - 1] == Approx(expectedValue).margin(0.01));
    }
}
TEST_CASE("ADSR Test", "[dsp]") {
    constexpr float sampleRate = 48000.0f;
    constexpr int blockSize = kFloatsPerDSPVector;
    std::vector<float> gateIn(blockSize, 0.0f);
    std::vector<float> attackIn(blockSize, 0.01f);   // 10ms attack
    std::vector<float> decayIn(blockSize, 0.1f);    // 100ms decay
    std::vector<float> sustainIn(blockSize, 0.5f);
    std::vector<float> releaseIn(blockSize, 0.2f); // 200ms release
    std::vector<float> out(blockSize, 0.0f);
    ADSR adsrModule(sampleRate);
    const float* inputs[] = {
        gateIn.data(), attackIn.data(), decayIn.data(),
        sustainIn.data(), releaseIn.data()
    };
    float* outputs[] = { out.data() };
    // Initial state should be idle
    adsrModule.process(inputs, 5, outputs, 1);
    REQUIRE(out[blockSize - 1] == 0.0f);
    // --- Trigger gate on ---
    std::fill(gateIn.begin(), gateIn.end(), 1.0f);
    // Attack phase (process for a bit longer than attack time)
    float lastSample = 0.0f;
    adsrModule.process(inputs, 5, outputs, 1);
    lastSample = out[blockSize-1];
    REQUIRE(lastSample > 0.0f); // Should be increasing
    // process for just a few more blocks to check it's still rising
    processAndCheck(adsrModule, inputs, outputs, 128, 0.0f, false);
    REQUIRE(out[blockSize-1] > lastSample); // Still increasing
    // Decay phase (should eventually settle at sustain)
    processAndCheck(adsrModule, inputs, outputs, 9600, 0.5f, true); // ~200ms
    // Sustain phase
    processAndCheck(adsrModule, inputs, outputs, 4800, 0.5f, true);
    // --- Trigger gate off ---
    std::fill(gateIn.begin(), gateIn.end(), 0.0f);
    // Release phase (should eventually be zero)
    processAndCheck(adsrModule, inputs, outputs, 12000, 0.0f, true); // ~250ms
    // Should remain idle
    processAndCheck(adsrModule, inputs, outputs, 4800, 0.0f, true);
} 