#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* osc_sine_create(float sampleRate);
void osc_sine_process(void* instance, const float* freqIn, float* out);
void osc_sine_destroy(void* instance);

#ifdef __cplusplus
}
#endif
