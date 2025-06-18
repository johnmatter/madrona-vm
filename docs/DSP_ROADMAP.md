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
-   [x] **`NoteIn`**
    -   Receives discrete note events and outputs signals.
    -   **Source**: `MLEventsToSignals` (`EventsToSignals`, `Events`.)
    -   **I/O**: `out[0]`: gate (0 or 1), `out[1]`: pitch (MIDI note number), `out[2]`: velocity (0-1).
-   [ ] **`SawOsc`**
    -   band-limited sawtooth oscillator.
    -   **Source**: `MLDSPGens.h` (`phasorToSaw`).
    -   **I/O**: `in[0]`: freq, `out[0]`: signal.
-   [ ] **`PulseOsc`**
    -   band-limited pulse/square wave oscillator.
    -   **Source**: `MLDSPGens.h` (`phasorToPulse`).
    -   **I/O**: `in[0]`: freq, `in[1]`: pulse width, `out[0]`: signal.
-   [ ] **`Biquad`** (as VCF)
    -   General purpose 2-pole, 2-zero filter.
    -   **Source**: `MLDSPFilters.h` (`Biquad`).
    -   **I/O**: `in[0]`: signal, `in[1]`: cutoff freq, `in[2]`: resonance, `out[0]`: signal.
-   [x] **`Gain`**
    -   amplifier to control signal level (our VCA).
    -   **Source**: `MLDSPOps.h`.
    -   **I/O**: `in[0]`: signal, `in[1]`: gain, `out[0]`: signal.
-   [x] **`ADSR`**
    -   Attack Decay Sustain Release envelope generator.
    -   **Source**: `MLDSPFilters.h` (`ADSR`)
    -   **I/O**: `in[0]`: gate, `in[1]`: attack, `in[2]`: decay, `in[3]`: sustain, `in[4]`: release, `out[0]`: envelope signal.
---
## 3. Phase 2: Comprehensive `madronalib` Integration
With a basic synth in place, the next phase is to systematically wrap the remaining `madronalib` functionality.
### Category 1: Generators (`MLDSPGens.h`)
- `SineOsc`: Sine wave oscillator. (Wraps `ml::SineGen`)
- `NoiseGen`: White noise generator. (Wraps `ml::NoiseGen`)
- `ImpulseGen`: Band-limited impulse generator. (Wraps `ml::ImpulseGen`)
### Category 2: Filters (`MLDSPFilters.h`)
- `Lopass`: 1-pole low-pass filter. (Wraps `ml::Lopass`)
- `Hipass`: 1-pole high-pass filter. (Wraps `ml::Hipass`)
- `Bandpass`: 2-pole band-pass filter. (Wraps `ml::Bandpass`)
- `DCBlock`: DC blocking filter. (Wraps `ml::DCBlock`)
### Category 3: Routing & Mixing (Stateless Wrappers) (`MLDSPRouting.h`)
- `Mixer`: Mixes multiple inputs with individual gain control. (Wraps `ml::mix`)
- `Crossfader`: Fades between two inputs. (Wraps `ml::multiplexLinear`)
### Category 4: Operations & Math (Stateless Wrappers) (`MLDSPOps.h`)
- `Add` / `Subtract` / `Multiply` / `Divide`: Vector arithmetic.
- `Clip`: Hard clipping. (Wraps `ml::clamp`)
- `Wrap`: Wraps signal. (Wraps `ml::wrap`)
### Category 5: Conversions & Scaling (`MLDSPProjections.h`, `MLDSPScale.h`)
- `mtof`: MIDI note to frequency. (Needs custom implementation)
- `ftom`: Frequency to MIDI note. (Needs custom implementation)
- `ScaleQuantizer`: Quantizes a pitch signal to a musical scale. (Wraps `ml::Scale`)
- `Linear`: Maps a signal from one range to another, linearly. (Needs custom vector implementation)
- `Log`: Maps a signal from one range to another, logarithmically. (Needs custom vector implementation)
### Category 6: Envelopes (`MLDSPFilters.h`)
- `ADSR`: ADSR envelope generator. (Wraps `ml::ADSR`)
### Category 7: Effects
- `Saturate`: A `tanh` saturator. (Needs custom implementation)
- `Delay`: A fractional delay. (Wraps `ml::FractionalDelay`)
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
