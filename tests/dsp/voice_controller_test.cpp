#include "dsp/voice_controller.h"
#include "catch.hpp"
#include <vector>
using namespace madronavm;
using namespace madronavm::dsp;
TEST_CASE("VoiceController Test", "[dsp]") {
    constexpr float sampleRate = 48000.0f;
    constexpr int blockSize = kFloatsPerDSPVector;
    const int numOutputs = VoiceController::kMaxVoices * VoiceController::kNumOutputsPerVoice;
    int currentSample = 0;
    VoiceController controller(sampleRate);
    std::vector<float*> outputs(numOutputs, nullptr);
    std::vector<std::vector<float>> outputData(numOutputs, std::vector<float>(blockSize));
    for(int i = 0; i < numOutputs; ++i) {
        outputs[i] = outputData[i].data();
    }
    // --- Initial state: no notes ---
    controller.process(nullptr, 0, outputs.data(), numOutputs);
    currentSample += blockSize;
    const float* gateOut = outputs[ml::kGate];
    const float* pitchOut = outputs[ml::kPitch];
    REQUIRE(gateOut[blockSize - 1] == 0.0f);
    // --- Send Note On ---
    controller.noteOn(60, 100, 0, 0); // Middle C, time is relative to block start
    controller.process(nullptr, 0, outputs.data(), numOutputs);
    currentSample += blockSize;
    // Check outputs for voice 0
    REQUIRE(gateOut[blockSize - 1] > 0.0f);
    REQUIRE(pitchOut[blockSize - 1] == Approx(60.0f));
    // --- Process a few more times, should hold ---
    controller.process(nullptr, 0, outputs.data(), numOutputs);
    currentSample += blockSize;
    controller.process(nullptr, 0, outputs.data(), numOutputs);
    currentSample += blockSize;
    REQUIRE(gateOut[blockSize - 1] > 0.0f);
    REQUIRE(pitchOut[blockSize - 1] == Approx(60.0f));
    // --- Send Note Off ---
    controller.noteOff(60, 0, 0, 0); // Time is relative to block start
    controller.process(nullptr, 0, outputs.data(), numOutputs);
    currentSample += blockSize;
    // The gate should now be zero.
    REQUIRE(gateOut[blockSize - 1] == 0.0f);
} 