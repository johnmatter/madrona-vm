#include "catch.hpp"
#include "dsp/lopass.h"
#include "MLDSPGens.h"
#include "common/embedded_logging.h"
#include <vector>
constexpr float kSampleRate = 44100.f;
TEST_CASE("madronavm/dsp/lopass", "[madronavm][dsp][lopass]") {
    madronavm::dsp::Lopass lopass(kSampleRate);
    // Prepare inputs
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
    for(int i=0; i<10; ++i) {
        lopass.process(inputs.data(), inputs.size(), outputs.data(), outputs.size());
    }
    // Check output - SVF lowpass should pass most of a DC signal
    // but may not be exactly 1.0 due to the filter design
    uint32_t output_bits;
    std::memcpy(&output_bits, &out_vec[0], sizeof(float));
    MADRONA_LOG_INFO(MADRONA_COMPONENT_DSP, "Lopass 1kHz cutoff output: 0x%08X", output_bits);
    madronavm::logging::flush();
    REQUIRE(out_vec[0] > 0.8f); // Should pass most of the signal
    REQUIRE(out_vec[0] <= 1.0f); // But not exceed input
    // Test with a very low cutoff - DC should still pass through completely
    // because DC is 0 Hz, which is below any positive cutoff frequency
    std::fill(cutoff_in.begin(), cutoff_in.end(), 5.f); // very low cutoff
    // process several times to let the filter settle
    for(int i=0; i<20; ++i) {
        lopass.process(inputs.data(), inputs.size(), outputs.data(), outputs.size());
    }
    std::memcpy(&output_bits, &out_vec[0], sizeof(float));
    MADRONA_LOG_INFO(MADRONA_COMPONENT_DSP, "Lopass 5Hz cutoff output: 0x%08X", output_bits);
    madronavm::logging::flush();
    // DC should pass through completely even with very low cutoff
    // because 0 Hz (DC) is always below any positive cutoff frequency
    REQUIRE(out_vec[0] > 0.8f); // Should still pass DC signal
    REQUIRE(out_vec[0] <= 1.0f); // But not exceed input
} 