# Project Specification: Repatchable DSP Synth System
## Summary
This project is a real-time, modular audio synthesis engine featuring:
- A DSP virtual machine with bytecode execution
- JSON-based patch graphs for defining signal flow
- The madronalib DSP library as the signal processing backend
- Vectorized audio processing using 64-sample DSPVector blocks
- Live audio playback with real-time patch execution
The system serves as a foundation for experimental synthesis applications and educational DSP exploration.
---
## Current Architecture
### 1. Patch Graph (JSON Format)
Patches are defined as JSON documents with modules and connections:
```json
{
  "modules": [
    {
      "id": 1,
      "name": "sine_gen",
      "data": { "freq": 440.0 }
    },
    {
      "id": 2,
      "name": "gain",
      "data": { "gain": 0.5 }
    },
    {
      "id": 3,
      "name": "audio_out",
      "data": {}
    }
  ],
  "connections": [
    { "from": "1:out", "to": "2:in" },
    { "from": "2:out", "to": "3:in_l" },
    { "from": "2:out", "to": "3:in_r" }
  ]
}
```
### 2. Module Registry
- Centralized module definitions in `data/modules.json`
- Stable module IDs using category-based numbering (see `docs/VM_DESIGN.md`)
- Port specifications (inputs/outputs) for each module type
- Examples: `sine_gen` (256), `phasor_gen` (257), `gain` (1027), `adsr` (1536)
### 3. Compiler
- Topological sorting for dependency-ordered execution
- Bytecode generation with opcodes: `LOAD_K`, `PROC`, `AUDIO_OUT`, `END`
- Register allocation for DSPVector storage
- Constant folding for module parameters
### 4. Virtual Machine
- Bytecode interpreter with program counter execution
- DSPVector registers (madronalib 64-sample vectors)
- Module factory for instantiating DSP objects
- Real-time processing in audio callback
### 5. DSP Modules
Implemented modules:
- Generators: `sine_gen`, `phasor_gen`
- Math: `add`, `mul`, `gain`
- Envelopes: `adsr`
- Utilities: `threshold` (comparator), `float`, `int`
- I/O: `audio_out`
All modules inherit from `madronavm::dsp::DSPModule` and use madronalib for DSP operations.
---
## Dependencies
| Component | Technology | Purpose |
|-----------|------------|---------|
| DSP Engine | [madronalib](https://github.com/madronalabs/madronalib) | Core signal processing (DSPVector, oscillators, filters) |
| Audio I/O | RtAudio (via madronalib) | Cross-platform audio device access |
| UI Framework | [ftxui](https://github.com/ArthurSonzogni/FTXUI) | Terminal-based UI components |
| Testing | [Catch2](https://github.com/catchorg/Catch2) | Unit and integration testing |
| Build System | CMake 3.10+ | Cross-platform builds |
| Language | C++17 | Core implementation |
## Current Implementation Status
### Completed Features
- JSON patch parsing and validation
- Bytecode compiler with topological sort
- Virtual machine with real-time execution
- Half a dozen DSP modules with good test coverage
- Audio device enumeration and playback
- Example patches (`a440.json`, `binaural.json`, ...)
- Comprehensive unit and integration tests
### In Progress
- Live patch reloading (requires thread-safe messaging)
- Parity with madronalib's collection of DSP units
### Potential Features
- Visual patch editor
- Parameters/attributes/flags. Something like that.
- MIDI/OSC
- Embed this in a eurorack module powered by ESP32, STM32H7, or teensy 4.1
    - bt/web gui for bytecode transfer
- Multiple output devices
- Sequencers
---
## Example Bytecode
The a440.json patch compiles to:
```
Header: [magic=0x41434142, version=1, registers=4, program_size=30]
LOAD_K    r0, 440.0     // Load frequency constant
PROC      1, 256, 1, 1, r0, r1  // sine_gen: node_id=1, module_id=256
LOAD_K    r2, 0.5       // Load gain constant
PROC      2, 1027, 2, 1, r1, r2, r3  // gain: node_id=2, module_id=1027
AUDIO_OUT 2, r3, r3     // Output to left/right channels
END                     // End of program
```
## File Structure
```
src/
  parser/     // JSON parsing (patch_graph.h/cpp)
  compiler/   // Bytecode generation (compiler.h/cpp, module_registry.h/cpp)
  vm/         // Virtual machine (vm.h/cpp, opcodes.h)
  dsp/        // DSP modules (sine_gen.h/cpp, etc.)
  audio/      // Audio device management
  ui/         // Terminal UI components
data/
  modules.json     // Module registry definitions
examples/
  *.json          // Example patches
tests/
  unit/           // Module-specific tests
  integration/    // End-to-end system tests
```
## Performance Characteristics
| Metric | Value |
|--------|-------|
| Audio Block Size | 64 samples (kFloatsPerDSPVector) |
| Processing Model | Vectorized (SIMD-optimized via madronalib) |
| Typical Latency | ~1.3ms @ 48kHz (64 samples) |
| Module Overhead | ~2-5 µs per module per block |
| Memory Model | Stack-allocated DSPVectors, heap module instances |
## Development Workflow
1. Add DSP module following `docs/DSP_MODULE_BOILERPLATE.md`
2. Build: `cmake --build build`
3. Run tests: `./build/run_tests`
4. Create patch JSON, test with `simple_vm_demo`
## Example Usage
```bash
# Build the project
cmake -B build && cmake --build build
# Run all tests
./build/run_tests
# Play a patch
./build/simple_vm_demo examples/a440.json
# Run specific module tests
./build/run_tests "[sine_gen]"
```
