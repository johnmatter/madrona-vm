# DSP Module Implementation Roadmap

## 1. Overview

This document outlines the plan for extending our DSP module library by wrapping `madronalib`'s deep DSP framework.

---

## 2. Phase 1: Minimal Subtractive Synthesizer

The first priority is to implement the core set of modules required to build a classic monophonic subtractive synthesizer. This will provide a baseline for testing the entire system (parser, compiler, VM) and will result in a usable instrument.

### 2.1. Required Modules

A subtractive synth is built from a few key components:

-   **Voltage-Controlled Oscillator (VCO)**: generate raw waveforms.
-   **Voltage-Controlled Filter (VCF)**: shape the sound.
-   **Voltage-Controlled Amplifier (VCA)**: control the amplitude.
-   **Envelope Generator (EG)**: modulate parameters over time.
-   **Note Input**: convert note-on/off events into continuous gate and pitch signals.

### 2.2. Implementation Plan

-   [ ] **`SawOsc`**
    -   band-limited sawtooth oscillator.
    -   **Source**: `MLDSPGens.h` (`PhasorGen` -> `phasorToSaw`).
    -   **I/O**: `in[0]`: freq, `out[0]`: signal.

-   [ ] **`PulseOsc`**
    -   band-limited pulse/square wave oscillator.
    -   **Source**: `MLDSPGens.h` (`PhasorGen` -> `phasorToPulse`).
    -   **I/O**: `in[0]`: freq, `in[1]`: pulse width, `out[0]`: signal.

-   [ ] **`NoiseGen`**
    -   white noise generator.
    -   **Source**: `MLDSPGens.h` (`NoiseGen`).
    -   **I/O**: `out[0]`: signal.

-   [ ] **`MoogLadderFilter`**
    -   classic 4-pole resonant low-pass filter.
    -   **Source**: `MLDSPFilters.h` (`MLMoogLadder`).
    -   **I/O**: `in[0]`: signal, `in[1]`: cutoff freq, `in[2]`: resonance, `out[0]`: signal.

-   [ ] **`Gain`**
    -   amplifier to control signal level (our VCA).
    -   **Source**: `MLDSPOps.h`.
    -   **I/O**: `in[0]`: signal, `in[1]`: gain, `out[0]`: signal.

-   [ ] **`ADSR`**
    -   Attack Decay Sustain Release envelope generator.
    -   **Source**: `MLDSPGens.h` (`RampGen` or `LineGen` can be adapted. A dedicated ADSR might need to be built from these primitives.).
    -   **I/O**: `in[0]`: gate, `in[1]`: attack, `in[2]`: decay, `in[3]`: sustain, `in[4]`: release, `out[0]`: envelope signal.

-   [ ] **`NoteIn`**
    -   Receives discrete note events and outputs signals.
    -   **Source**: Custom Logic.
    -   **I/O**: `out[0]`: gate (0 or 1), `out[1]`: pitch (MIDI note number), `out[2]`: velocity (0-1).

---

## 3. Phase 2: Comprehensive `madronalib` Integration

With a basic synth in place, the next phase is to systematically wrap the remaining `madronalib` functionality.

The implementation will be organized by the `madronalib` header files.

### 3.1. Generators (`MLDSPGens.h`)

-   **`TriangleOsc`**: A triangle waveform oscillator.
-   **`ImpulseGen`**: An anti-aliased impulse generator.
-   **LFOs**: Specialized low-frequency oscillators with various shapes (sine, triangle, ramp, etc.) and rate controls. While any oscillator can be an LFO, dedicated modules can provide more convenient features like tempo sync.

### 3.2. Filters (`MLDSPFilters.h`)

-   **`MLStateVarFilter`**: A versatile state-variable filter providing simultaneous low-pass, high-pass, band-pass, and notch outputs.
-   **`MLBiquad`**: A direct-form biquad filter implementation, useful for creating custom EQs.
-   **`MLFormantFilter`**: A filter that models the resonances of the human vocal tract.
-   **All-pass and Comb filters**: For creating phasing effects and physical modeling resonators.

### 3.3. Routing & Mixing (`MLDSPRouting.h`)

-   **`Mixer`**: A multi-channel mixer with level controls for each input.
-   **`Crossfader`**: A module to fade between two input signals using a control signal.
-   **`MatrixMixer`**: For advanced routing scenarios.

### 3.4. Signal Transformers & Utilities

-   **`MLDSPOps.h` / `MLDSPFunctional.h`**:
    -   **`Clip` / `Wrap`**: Signal clipping and wrapping utilities.
    -   **`Abs` / `Math`**: Basic math functions (`pow`, `sqrt`, etc.) exposed as modules.
    -   **Logic**: Boolean logic modules (`AND`, `OR`, `>` etc.) for control signals.
-   **`MLDSPProjections.h` / `MLDSPScale.h`**:
    -   **`mtof` / `ftom`**: MIDI-to-frequency conversion and vice-versa.
    -   **`linToExp` / `expToLin`**: For mapping control signals between linear and exponential curves.
    -   **`ScaleQuantizer`**: A module to snap pitch signals to a specific musical scale, using `madronalib`'s scale file loading capabilities.

---

## 4. Control Plane Integration (MIDI & OSC)

The general plan is as follows:

1.  **MIDI/OSC Server**: A component on a non-real-time thread will listen for incoming MIDI or OSC messages.
2.  **Message Parsing**: This server will parse messages (e.g., Note On, CC, OSC address).
3.  **Dispatch to VM**: The parsed data will be sent to the VM via a thread-safe queue.
4.  **VM Action**: The VM will then update the state of the DSP graph accordingly. For note events, this means sending gate/pitch information to a `NoteIn` module. For CC messages, it could mean writing a new value directly to a module's input buffer (e.g., a filter cutoff frequency).

---

## 5. Stretch goofs

- Simulate ORTEC and LeCroy modules
    - http://www.nuclearphysicslab.com/npl/npl-home/spectroscopy/software_and_hardware/nim-modules/ortec-nim-devices/
