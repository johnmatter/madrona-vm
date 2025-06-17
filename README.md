# Madronalib VM Synth

A modular synthesis engine built on the madronalib DSP framework. This project implements a virtual machine that executes a custom bytecode format, allowing for dynamic and flexible audio synthesis graphs.

## Why do we need another node-based visual DSP programming language?

[in this interregnum a great variety of morbid symptoms appear](https://brill.com/view/journals/powr/1/2/article-p379_379.xml)

## Core Concepts

The system is built around a few core ideas:

-   **Patch Graph**: A directed acyclic graph of DSP modules, which can be defined in JSON.
-   **Compiler**: A component that translates the high-level patch graph into a linear bytecode sequence for efficient execution.
-   **Virtual Machine (VM)**: The real-time audio component that interprets the bytecode and orchestrates the DSP processing.
-   **DSP Modules**: Self-contained, object-oriented DSP units (e.g., oscillators, filters) that perform the actual audio work.

For a detailed breakdown of the architecture, see the [Project Specification](docs/PROJECT_SPEC.md) and the [DSP Module Architecture](docs/DSP_MODULE_ARCHITECTURE.md) design documents.

## Getting Started

Follow these instructions to get a copy of the project up and running on your local machine.

### Prerequisites

You will need a C++17 compliant compiler (like GCC or Clang) and CMake (version 3.16 or higher).

-   [CMake v3.16 or higher](https://cmake.org/download/)
-   A C++17-compliant compiler (e.g., `g++` or `clang++`)

### 1. Clone the Repository

Clone recursively to include submodules (madronalib).

```bash
git clone --recursive https://github.com/johnmatter/madrona-vm.git
cd madrona-vm
```

If you have already cloned the repository without the `--recursive` flag, you can initialize the submodules separately:

```bash
git submodule update --init --recursive
```

### 2. Build the Project


```bash
mkdir build
cd build
cmake ..
make
```


### 3. Run the Tests

After a successful build, you can run the test suite from the `build` directory.

```bash
./run_tests
```

This will execute all the unit tests and confirm that the DSP modules are functioning as expected.

## Project Status & Roadmap

-   [x] **DSP Module Architecture**: A C++ class-based architecture for creating new DSP modules is in place.
-   [ ] **Patch Graph Parser**: Responsible for reading a patch definition (e.g., from a JSON file) and constructing the in-memory graph representation.
-   [ ] **Compiler**: ** Translate the graph into bytecode for the VM.
-   [ ] **Virtual Machine (VM)**: Core interpreter that runs the bytecode on the audio thread.

## Usage

*(This section will be updated once the VM and a command-line interface are available.)*

To run the synthesizer, you will eventually execute a command like:

```bash
./madronavm --patch examples/simple_patch.json
```
