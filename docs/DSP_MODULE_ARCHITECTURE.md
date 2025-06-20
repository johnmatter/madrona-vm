# DSP Module Architecture
## 1. Overview
This architecture introduces an abstract base class, `DSPModule`, from which all DSP processing units inherit. This class defines the standard interface for any module.
### 1.1. `DSPModule`
**File:** `include/dsp/module.h`
```cpp
#pragma once
#include "MLDSPGens.h" // For kFloatsPerDSPVector & DSPVector
class DSPModule {
public:
  explicit DSPModule(float sampleRate);
  virtual ~DSPModule() = default;
  // Process one block of audio.
  // inputs: An array of pointers to input buffers.
  // outputs: An array of pointers to output buffers.
  // Each buffer is assumed to have a size of kFloatsPerDSPVector.
  virtual void process(const float** inputs, float** outputs) = 0;
  // Called by the VM when the module is connected or reconfigured within the graph.
  // Modules can override this to perform one-time setup based on the number of
  // connections they have.
  virtual void configure(size_t numInputs, size_t numOutputs);
  void setSampleRate(float sampleRate);
protected:
  float mSampleRate;
};
```
The `const float** inputs` and `float** outputs` parameters represent arrays of pointers to the input and output audio buffers. This allows any module to have a variable number of connections while maintaining a consistent function signature.
**Inlet and Outlet Convention**
*   `inputs[0]` corresponds to the **leftmost inlet** of a module.
*   `inputs[1]` is the next inlet to the right, and so on.
*   `outputs[0]` corresponds to the **leftmost outlet**.
Module authors must document this intended order, and patch creators must adhere to it to ensure correct behavior. For example, in the `SineOsc` module, `inputs[0]` is used for frequency control.
### 1.2. Module Lifecycle
The interaction between the VM and a `DSPModule` follows a lifecycle:
1.  **Instantiation**: The VM creates a module instance (e.g., `new SineOsc(sampleRate)`).
2.  **Configuration**: The VM calls `module->configure(numInputs, numOutputs)` to inform the module about its place in the patch graph. This allows the module to perform any necessary setup. This step is repeated if the graph is later mutated.
3.  **Processing**: The VM calls `module->process()` on every audio block. This is the hot path and should be as efficient as possible.
4.  **Destruction**: The module is destroyed when the patch is torn down.
This separation ensures that configuration logic is kept out of the real-time processing path.
## 2. Module Implementation Examples
### 2.1. The `SineOsc` Module
The sine oscillator is a concrete implementation of `DSPModule`.
**File:** `include/dsp/sine_gen.h`
```cpp
#pragma once
#include "dsp/module.h"
#include "MLDSPGens.h" // For SineGen
class SineOsc : public DSPModule {
public:
  explicit SineOsc(float sampleRate);
  ~SineOsc() override = default;
  void process(const float** inputs, float** outputs) override;
private:
  ml::SineGen mOsc;
};
```
The implementation encapsulates the `ml::SineGen` instance and the processing logic.
**File:** `src/dsp/sine_gen.cpp`
```cpp
#include "dsp/sine_gen.h"
#include "MLDSPGens.h"
using namespace ml;
SineOsc::SineOsc(float sampleRate) : DSPModule(sampleRate) {
  mOsc.clear();
}
void SineOsc::process(const float** inputs, float** outputs) {
  const float* freqIn = inputs[0]; // Frequency inlet
  float* out = outputs[0];         // Audio outlet
  // Convert frequency to cycles per sample for madronalib
  DSPVector freq(freqIn[0] / mSampleRate);
  // Process one vector
  DSPVector result = mOsc(freq);
  // Copy result to the output buffer
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    out[i] = result[i];
  }
}
```
### 2.2. Example: A `Gain` Module
This architecture makes adding new modules straightforward. Here is an example of a simple `Gain` module with one signal input, one control-rate gain input, and one output.
**File:** `include/dsp/gain.h`
```cpp
#pragma once
#include "dsp/module.h"
class Gain : public DSPModule {
public:
  explicit Gain(float sampleRate);
  void process(const float** inputs, float** outputs) override;
};
```
**File:** `src/dsp/gain.cpp`
```cpp
#include "dsp/gain.h"
#include "MLDSPOps.h"
using namespace ml;
Gain::Gain(float sampleRate) : DSPModule(sampleRate) {}
void Gain::process(const float** inputs, float** outputs) {
  const float* signalIn = inputs[0];
  const float* gainIn = inputs[1]; // control input
  float* signalOut = outputs[0];
  DSPVector signalVec(signalIn);
  DSPVector gainVec(gainIn[0]); // Broadcast gain to a full vector
  DSPVector result = signalVec * gainVec;
  // Copy result to the output buffer
  for (int i = 0; i < kFloatsPerDSPVector; ++i) {
    signalOut[i] = result[i];
  }
}
```
## 3. Integration with the VM
The patch graph operates on `DSPModule*` pointers. The VM is responsible for:
1.  Instantiating concrete module classes (e.g., `new SineOsc(sampleRate)`).
2.  Managing a memory pool for input and output buffers.
3.  Orchestrating calls to `module->configure()` when the graph is built or changed.
4.  Orchestrating calls to `module->process()` for all modules in the graph on the audio thread, respecting topological order. 