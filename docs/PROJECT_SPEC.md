# Project Specification: Repatchable DSP Synth System

## Summary

This project aims to build a **real-time, modular audio synthesis engine** using:

- A **custom DSP virtual machine** with a graph-based architecture
- A **bytecode intermediate representation** optimized for vectorized signal processing
- The **madronalib DSP library** as the low-level signal processing backend
- A runtime model that allows **live repatching** with no audible dropouts

The system is intended to serve as the backend for experimental synthesis applications, including standalone instruments, plugins, and embedded environments.

---

## Components

### 1. Patch Graph IR
The patch is represented as a **typed, directed acyclic graph**, where:

- **Nodes** = DSP modules (e.g., oscillator, filter, gain)
- **Edges** = Connections between output and input ports (audio or control)
- Each node has:
  - A unique ID
  - A type (module name)
  - A list of input/output ports
  - Parameters (fixed or signal-controlled)

Format options:
- Text (S-expression, JSON, YAML)
- Binary (MessagePack, FlatBuffers)

### 2. Compiler
- Performs graph validation and topological sort
- Resolves modules into **opcodes** and static memory slots
- Compiles graph into **bytecode**:
  - `LOAD_PARAM`, `READ_PORT`, `CALL_OP`, `WRITE_PORT`, etc.
- Optional optimization: inlining static parameter nodes

### 3. Virtual Machine
- Executes compiled patch bytecode per audio block
- Memory model:
  - Vector registers (`DSPVector`) for audio
  - Scalar control rate buffer
- Supports:
  - Module execution
  - Message handling (control thread ↔ audio thread)
  - Graph hot-swap at block boundaries

### 4. Control Interface
- Sends messages to control ports or modify graph:
  - `/patch/add`, `/patch/connect`, `/param/set`
- UI or scripting system interacts via a thread-safe queue
- Patch graph mutation occurs on control thread

---

## Core Requirements

| Feature              | Spec |
|----------------------|------|
| Audio block size     | 64–128 samples |
| Max latency on repatch | ≤ 1 block |
| Max voice count      | 16–32 (extensible) |
| Parameter update rate | ≥ 1kHz (or audio rate optional) |
| Thread safety        | All repatching occurs off audio thread |

---

## Toy Patch Graph Example

Below is a textual patch written in a Lisp-like DSL:

```lisp
(patch
  (nodes
    (osc1 (type "osc.sine") (freq 440))
    (gain1 (type "gain") (amp 0.5))
    (out (type "output")))
  (connections
    (osc1 "out" -> gain1 "in")
    (gain1 "out" -> out "in")))
```

The equivalent json is:
```json
{
  "nodes": {
    "osc1": { "type": "osc.sine", "params": { "freq": 440 } },
    "gain1": { "type": "gain", "params": { "amp": 0.5 } },
    "out": { "type": "output" }
  },
  "connections": [
    { "from": ["osc1", "out"], "to": ["gain1", "in"] },
    { "from": ["gain1", "out"], "to": ["out", "in"] }
  ]
}    
```

This would compile into bytecode such as:
```
LOAD_CONST   r1, 440
CALL_OP      osc_sine, r1 -> r2
LOAD_CONST   r3, 0.5
CALL_OP      gain, r2, r3 -> r4
WRITE_OUTPUT r4
```

## Development Roadmap

### Phase 1: Minimum Viable Signal Path
- Define graph schema + IR
- Implement VM with hardcoded patch
- Wrap madronalib ops into callable function table
- Real-time audio thread w/ audio callback (RtAudio or equivalent)

### Phase 2: Dynamic Graph and Messaging
- Implement JSON/S-expression parser
- Graph diff engine (add/remove/connect nodes)
- Double-buffering logic for safe graph swaps
- Command interface via OSC or socket

### Phase 3: UI & Live Patch Editor
- Minimal patch editor (CLI, web, or mlvg GUI)
- Patch save/load system
- Preset manager
- Support for MIDI and automation input

## Technology Choices

| Component | Tool / Language |
|-----------|----------------|
| DSP Backend | madronalib |
| Audio I/O | RtAudio / JUCE / PortAudio |
| VM/Graph | C++17 or C++20 |
| UI | mlvg, Web (React, Tauri), or CLI |
| Scripting (optional) | Lua or Scheme-style DSL |

## Component Roles
- **DSP Systems**: Implement VM ops, test audio-rate behavior
- **Graph Compiler**: Optimize patch translation and bytecode
- **Control Layer**: Command router, messaging system
- **UI/Frontend**: Visual patcher, integration with control plane

## Appendix: Module Spec Example
```json
{
  "osc.sine": {
    "inputs": [],
    "outputs": ["out"],
    "params": {
      "freq": { "type": "float", "default": 440 }
    },
    "impl": "madronalib::osc_sine"
  }
}
```

## Feedback Areas
- Graph encoding format (JSON vs Lisp-like DSL)
- Audio threading + repatch safe point model
- Choice of bytecode instruction model (stack vs register)
- Module parameterization and dynamic types