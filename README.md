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
## Usage
### Run the Demo Application
```bash
# Build the project
mkdir build && cd build
cmake .. && make
# Run the end-to-end demo
cd .. && ./build/simple_vm_demo
```
The demo will:
1. Load `examples/simple_patch.json`
2. Parse it into a PatchGraph
3. Compile it to bytecode
4. Execute it in the VM
5. Process 2 seconds of audio (1378 blocks)
### Run the Test Suite
```bash
cd build
./run_tests                    # Run all tests
./run_tests "[integration]"    # Run just end-to-end integration tests
