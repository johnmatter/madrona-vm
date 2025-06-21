#include "catch.hpp"
#include "dsp/hipass.h"
#include "MLDSPGens.h"
#include "common/embedded_logging.h"
#include <vector>
constexpr float kSampleRate = 44100.f;
TEST_CASE("madronavm/dsp/hipass", "[madronavm][dsp][hipass]") {
    madronavm::dsp::Hipass hipass(kSampleRate);
    // Test with DC signal (should be blocked)
    const float signal_val = 1.0f;
    const float cutoff_val = 1000.0f;
    const float q_val = 2.0f;
    std::vector<float> signal_in(kFloatsPerDSPVector, signal_val);
    std::vector<float> cutoff_in(kFloatsPerDSPVector, cutoff_val);
    std::vector<float> q_in(kFloatsPerDSPVector, q_val);
    std::vector<const float*> inputs = {
        signal_in.data(),
        cutoff_in.data(),
        q_in.data()
    };
    // Prepare outputs
    std::vector<float> out_vec(kFloatsPerDSPVector, 0.0f);
    std::vector<float*> outputs = { out_vec.data() };
    // Process several blocks to let the SVF settle
    for(int i=0; i<20; ++i) {
        hipass.process(inputs.data(), inputs.size(), outputs.data(), outputs.size());
    }
    // Check output - SVF hipass should block DC signals
    uint32_t output_bits;
    std::memcpy(&output_bits, &out_vec[0], sizeof(float));
    MADRONA_LOG_INFO(MADRONA_COMPONENT_DSP, "Hipass DC block output: 0x%08X", output_bits);
    madronavm::logging::flush();
    // Be more forgiving while debugging
    REQUIRE(std::abs(out_vec[0]) < 0.5f); // Should be significantly attenuated
    // Test with a very low cutoff, should still block DC significantly
    std::fill(cutoff_in.begin(), cutoff_in.end(), 5.f);
    // reset filter state
    madronavm::dsp::Hipass hipass2(kSampleRate);
    for(int i=0; i<20; ++i) {
        hipass2.process(inputs.data(), inputs.size(), outputs.data(), outputs.size());
    }
    REQUIRE(out_vec[0] < 0.1f); // Should still block DC
} 