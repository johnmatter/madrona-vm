# Madrona VM: Design Specification
This document outlines the architecture for the Madrona Virtual Machine (VM), its bytecode format, and the process of parsing and compiling a patch description into an executable program.
## 1. High-Level Architecture
The system translates a high-level, declarative patch format into a low-level, linear sequence of instructions that can be executed efficiently on a real-time audio thread.
The process involves three main stages:
1.  **Parsing**: A JSON patch description is parsed into an in-memory, graph-based representation.
2.  **Compilation**: The patch graph is compiled into a linear sequence of bytecode instructions. This involves a topological sort of the graph to determine execution order and the allocation of memory for audio signals.
3.  **Execution**: The Virtual Machine (VM) executes the bytecode for each block of audio samples, processing audio from inputs to outputs.
## 2. The Patch Format (JSON)
Patches are defined in a JSON format. This format describes the DSP modules to be instantiated and the connections between them.
### Example: `simple_patch.json`
```json
{
  "version": 1,
  "modules": [
    { "id": 1, "name": "sine_gen", "data": { "freq": 440.0 } },
    { "id": 2, "name": "gain", "data": { "gain": 0.5 } },
    { "id": 3, "name": "audio_out" }
  ],
  "connections": [
    { "from": "1:out", "to": "2:in" },
    { "from": "2:out", "to": "3:in_l" },
    { "from": "2:out", "to": "3:in_r" }
  ]
}
```
### Schema:
*   **`version`**: The version of the patch format.
*   **`modules`**: An array of DSP module objects.
    *   **`id`**: A unique integer identifying the module within the patch.
    *   **`name`**: A string matching the registered name of a `DSPModule` implementation (e.g., "sine\_osc").
    *   **`data`**: An optional object containing constant values for the module's inputs (e.g., the frequency of an oscillator).
*   **`connections`**: An array of connection objects.
    *   **`from`**: A string in the format `"module_id:port_name"` specifying the source of the audio signal.
    *   **`to`**: A string in the format `"module_id:port_name"` specifying the destination.
## 3. The Parser and Graph Representation
The parser's responsibility is to convert the JSON patch into a validated, in-memory `PatchGraph`. This graph is the Intermediate Representation (IR) that the compiler will operate on.
### `PatchGraph` C++ Representation:
```cpp
#include <vector>
#include <string>
#include <map>
// Represents an input that is a fixed constant value.
struct ConstantInput {
    std::string port_name;
    float value;
};
// Represents a single DSP module instance in the graph.
struct Node {
    uint32_t id;
    std::string name; // e.g., "sine_gen"
    std::vector<ConstantInput> constants;
};
// Represents a connection between two nodes.
struct Connection {
    uint32_t from_node_id;
    std::string from_port_name;
    uint32_t to_node_id;
    std::string to_port_name;
};
// The complete, in-memory representation of the patch.
struct PatchGraph {
    std::vector<Node> nodes;
    std::vector<Connection> connections;
};
```
The parser will read the JSON, validate it (e.g., ensure all module IDs in `connections` exist), and populate these C++ structs.
## 4. The Compiler
The compiler translates the `PatchGraph` IR into a bytecode buffer.
### Key Steps:
1.  **Topological Sort**: The compiler performs a topological sort on the nodes in the graph to create a linear execution order. This ensures that a module is always processed after its inputs have been calculated.
2.  **Memory Allocation**: The compiler determines how many temporary audio buffers (`DSPVector`s) are needed. It allocates a "register" (an index into a block of memory owned by the VM) for the output of each module.
3.  **Instruction Emission**: The compiler walks the sorted graph and generates bytecode instructions for each node.
## 5. Bytecode Specification
The bytecode is a simple, linear array of 32-bit unsigned integers (`uint32_t`).
### VM Memory Model
The VM owns a flat block of memory large enough to hold all the `DSPVector` audio buffers required for the patch. The bytecode references these buffers by their index, or "register."
### Instruction Set
| OpCode (Hex) | Instruction | Operands                                                              | Description                                                                                                                                     |
| :----------- | :---------- | :-------------------------------------------------------------------- | :---------------------------------------------------------------------------------------------------------------------------------------------- |
| `0x01`       | `LOAD_K`    | `dest_reg`, `value`                                                   | Loads a floating-point constant (`value`) into the specified destination register (`dest_reg`). The float is bit-cast to a `uint32_t`.          |
| `0x02`       | `PROC`      | `module_id`, `num_inputs`, `num_outputs`, `in_regs...`, `out_regs...` | Executes the `process` method of a `DSPModule`.                                                                                                 |
| `0xFF`       | `END`       | (None)                                                                | Marks the end of the program for the current audio block.                                                                                       |
### Planned Module Registry
Instead of having a unique opcode for every DSP module, the `PROC` instruction takes a `module_id` as an operand. This ID is a stable, versioned identifier looked up in the VM's module registry. This approach is more scalable and means the VM's execution loop does not need to change when we add new modules.
To keep the registry organized and aligned with the `madronalib` source, we will adopt a three-digit hex numbering convention: `0xCMM`, where `C` is the category and `MM` is the module index. The category digit (`C`) maps directly to the `madronalib` header file where the DSP object is defined.
| `C` | Category | `madronalib` Source |
| :-- | :--- | :--- |
| `0` | System & I/O | `source/app/` |
| `1` | Generators | `source/DSP/MLDSPGens.h` |
| `2` | Filters | `source/DSP/MLDSPFilters.h` |
| `3` | Routing & Mixing | `source/DSP/MLDSPRouting.h` |
| `4` | Operations & Math | `source/DSP/MLDSPOps.h` |
| `5` | Conversions & Scaling | `source/DSP/MLDSPScale.h` & `MLDSPProjections.h`|
| `6` | Envelopes & Control | `source/DSP/MLDSPFilters.h` (e.g. `ADSR`) |
| `7` | Effects | `various` |
The following table outlines the planned module IDs and correslonsing madronalib source.
| Module ID (Hex) | Module Name | `madronalib` Source | Description |
| :--- | :--- | :--- | :--- |
| **Category 0** | **System & I/O** | `n/a` | |
| `0x000` | `NoteIn` | `EventsToSignals` | Converts note events to control signals. |
| `0x001` | `AudioOut` | `n/a` | Final audio output sink. (VM-internal concept) |
| **Category 1** | **Generators** | `MLDSPGens.h` | |
| `0x100` | `SineGen` | `SineGen` | Sine wave oscillator. |
| `0x101` | `SawGen` | `SawGen` | Sawtooth wave oscillator. |
| `0x102` | `PulseGen` | `PulseGen` | Pulse wave oscillator. |
| `0x103` | `NoiseGen` | `NoiseGen` | White noise generator. |
| `0x104` | `ImpulseGen`| `ImpulseGen` | Band-limited impulse generator. |
| **Category 2** | **Filters** | `MLDSPFilters.h` | |
| `0x200` | `Lopass` | `Lopass` | State Variable Filter (SVF) low-pass output. |
| `0x201` | `Hipass` | `Hipass` | State Variable Filter (SVF) high-pass output. |
| `0x202` | `Bandpass` | `Bandpass` | State Variable Filter (SVF) band-pass output. |
| `0x203` | `DCBlock` | `DCBlocker` | DC-blocking filter. |
| `0x204` | `Biquad` | `n/a` | Generic 2-pole, 2-zero filter. (use specific versions) |
| **Category 3** | **Routing**| `MLDSPRouting.h` | Stateless wrappers. |
| `0x300` | `Mixer` | `mix` | Mix multiple inputs. |
| `0x301` | `Crossfader`| `multiplexLinear`| Crossfade between two inputs. |
| **Category 4** | **Math** | `MLDSPOps.h` | Stateless wrappers. |
| `0x400` | `Add` | `+` | `in1 + in2` |
| `0x401` | `Multiply` | `*` | `in1 * in2` |
| `0x402` | `Clip` | `clamp` | Clip signal to `[-in2, in2]`. |
| **Category 5** | **Scaling** | `MLDSPScale.h` & `MLDSPProjections.h` | |
| `0x500` | `mtof` | `n/a (must implement)` | MIDI note to frequency. |
| `0x501` | `ftom` | `n/a (must implement)` | Frequency to MIDI note. |
| `0x502` | `ScaleQuantizer`| `Scale` | Quantize to a musical scale. |
| `0x503` | `Linear`| `linear` (from `MLDSPProjections.h`)| Linear range mapping. |
| `0x504` | `Log`| `log` (from `MLDSPProjections.h`)| Logarithmic range mapping. |
| **Category 6** | **Envelopes**| `MLDSPFilters.h` | |
| `0x600` | `ADSR` | `ADSR` | ADSR envelope generator. |
| **Category 7** | **Effects** | `various` | |
| `0x700` | `Saturate` | `n/a (must implement)` | `tanh(in1)`. |
| `0x701` | `Delay` | `FractionalDelay` | A fractional delay. |
### Bytecode Layout Example
Consider a `gain` module, which is just a `Multiply` operation. Its bytecode might look like this, using the new module ID `0x401`:
```
// Word Index | Value (Hex)     | Instruction | Comment
//----------- | --------------- | ----------- | -------------------------------------
// 10         | 0x00000002      | PROC        | Process instruction
// 11         | 0x00000401      | operand     | Module ID #0x401 (the Multiply module)
// 12         | 0x00000002      | operand     | 2 inputs (signal and gain value)
// 13         | 0x00000001      | operand     | Output to register 1
```
## 6. The Virtual Machine (VM)
The VM is the audio-thread-safe engine that executes the bytecode.
### C++ Interface:
```cpp
#include <vector>
#include <cstdint>
class VM {
public:
    VM();
    // Load a new bytecode program. This is called from the control thread.
    void load_program(std::vector<uint32_t> new_bytecode);
    // Execute the currently loaded program for one block of audio.
    // This is called from the real-time audio thread.
    void process(const float** inputs, float** outputs, int num_frames);
private:
    std::vector<uint32_t> m_bytecode;
    std::vector<DSPVector> m_registers; // The VM's memory
    // ... thread-safe mechanism for swapping bytecode ...
};
```
### Execution Loop
The `process` method contains a loop that walks through the `m_bytecode` vector:
1.  A program counter (`pc`) starts at 0.
2.  The VM reads the instruction at `m_bytecode[pc]`.
3.  A `switch` statement handles the opcode.
4.  For `PROC`, the VM gathers pointers to the input and output `DSPVector`s from its `m_registers` pool based on the register indices in the bytecode. It then finds the corresponding `DSPModule` instance and calls its `process` method.
5.  The `pc` is advanced according to the size of the current instruction.
6.  The loop continues until it hits an `END` instruction or the end of the buffer.
## 7. Conventions and Compatibility
To ensure the system is maintainable and extensible, we will adhere to the following conventions and compatibility strategies.
### Bytecode and Module Conventions
*   **Module Registry**: The VM will maintain a central registry that maps module names (e.g., `"sine_gen"`) to internal, stable integer IDs. The bytecode's `PROC` instruction will use this registry ID, not the patch-local `id` from the JSON file. This decouples the bytecode from the user's patch structure.
*   **Port Indexing**: Within a `DSPModule`'s definition, its input and output ports will be assigned fixed, zero-based indices. The compiler will use these indices to map connections to the correct registers. The bytecode will be independent of the string names of ports.
*   **Constants**: Constants defined in the `data` section of a module in JSON will be handled by the `LOAD_K` instruction at compile time. They are not handled dynamically by the `PROC` instruction.
### Intermediate Representation (IR) Compatibility Plan
The primary goal is to allow older patches to run on newer versions of the VM.
1.  **JSON Versioning**: The `version` field in `simple_patch.json` is the primary switch for compatibility. When the JSON schema changes in a non-backward-compatible way, this version number **must** be incremented. The parser will be responsible for understanding older versions and converting them into the latest `PatchGraph` representation.
2.  **Bytecode Header**: The bytecode buffer itself will contain a header to ensure the VM doesn't execute an incompatible program. The layout will be:
    *   `word 0`: Magic Number (e.g., `0xMADRONA_`) to identify the bytecode format.
    *   `word 1`: Bytecode Version (e.g., `1`).
    *   `word 2`: Program size in 32-bit words, including the header.
    *   `word 3`: Number of `DSPVector` registers required for execution.
## 8. Implementation Plan
This section outlines a recommended, step-by-step approach to building the VM system. The strategy is to build and test each component in a logical order, from parsing the input patch to generating the final audio output.
### Step 1: Finalize `PatchGraph` and Implement the Parser
The first step is to be able to load a patch description into a structured, in-memory representation.
*   **Action**: Define the C++ structs for `PatchGraph`, `Node`, and `Connection` in `include/parser/parser.h`, matching the specification in this document.
*   **Action**: Implement the parser logic in `src/parser/parser.cpp`. This function should take a JSON string, use the `cJSON` library (available via `madronalib`), and populate an instance of the `PatchGraph` struct.
*   **Goal**: A function that can successfully parse `examples/simple_patch.json` into a valid `PatchGraph` object.
### Step 2: Create the Compiler and Implement Topological Sort
The compiler's first responsibility is to determine a linear execution order from the patch graph.
*   **Action**: Create new files: `include/compiler/compiler.h` and `src/compiler/compiler.cpp`.
*   **Action**: In the new compiler class, implement a method that performs a topological sort on the nodes of the `PatchGraph`.
*   **Goal**: A method that takes a `PatchGraph` and returns a `std::vector<uint32_t>` of node IDs in the correct processing order. This can be unit-tested independently.
### Step 3: Implement the Compiler's Bytecode Generation
With the execution order determined, the compiler can translate the graph into bytecode.
*   **Action**: Extend the compiler to manage memory allocation for the VM's registers. It will need to map the output of each module to a specific register index.
*   **Action**: Implement the main compilation logic. This will iterate through the sorted nodes and emit the corresponding bytecode instructions (`LOAD_K`, `PROC`, `END`) into a `std::vector<uint32_t>`.
*   **Goal**: A compiler that can transform a `PatchGraph` object into a complete, valid bytecode buffer.
### Step 4: Flesh out the VM's Structure and Execution Loop
Now we build the engine that will run the bytecode.
*   **Action**: Implement the `VM` class in `include/vm/vm.h` and `src/vm/vm.cpp` according to the C++ interface specified in this document. This includes the `m_bytecode` vector, the `m_registers` pool for `DSPVector`s, and the thread-safe `load_program` method.
*   **Action**: Implement the main `process` method's execution loop with a program counter (`pc`) and a `switch` statement to interpret opcodes.
*   **Goal**: A VM that can load a bytecode buffer and step through the instructions, even if the `PROC` instruction is not yet fully functional.
### Step 5: Create the Module Registry and Integrate DSP Modules
The `PROC` instruction needs a way to find and execute the actual DSP code.
*   **Action**: Implement the "Module Registry" as a central map connecting stable module IDs (e.g., `0x100` for `SineGen`) to their corresponding C++ `DSPModule` objects.
*   **Action**: Connect the VM's `PROC` instruction handler to this registry. The handler will use the `module_id` operand to look up the correct module, gather the specified input/output registers, and call its `process` method.
*   **Goal**: A VM that can fully execute a patch, processing audio by calling the appropriate DSP modules in the correct order.
### Step 6: End-to-End Integration and Testing
The final step is to tie everything together and verify the system works as a whole.
*   **Action**: Create a new test or example application that:
    1.  Loads `examples/simple_patch.json`.
    2.  Uses the **Parser** to create the `PatchGraph`.
    3.  Uses the **Compiler** to generate bytecode.
    4.  Loads the bytecode into the **VM**.
    5.  Calls the VM's `process` method in a loop and verifies the audio output.
*   **Goal**: A working, testable, end-to-end audio processing pipeline based on the full design.