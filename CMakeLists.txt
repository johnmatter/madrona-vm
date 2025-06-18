cmake_minimum_required(VERSION 3.16)
project(madronalib_vm_synth)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Add madronalib, but don't build its examples
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable building madronalib examples")
add_subdirectory(external/madronalib)
# Include directories
include_directories(include)
include_directories(external/madronalib/include)
include_directories(external/madronalib/source)
include_directories(external/madronalib/source/DSP)
include_directories(external/madronalib/source/app)
include_directories(external/madronalib/source/matrix)
include_directories(external/madronalib/external/sse2neon)
include_directories(external/madronalib/external/cJSON)
# Collect source files
file(GLOB_RECURSE SRC_FILES
  "src/dsp/*.cpp"
)
# Create test executable
add_executable(run_tests
  tests/TestMain.cpp
  tests/test_vm.cpp
  tests/dsp/sine_gen_test.cpp
  tests/dsp/voice_controller_test.cpp
  tests/dsp/adsr_test.cpp
  tests/dsp/gain_test.cpp
  tests/parser/parser_test.cpp
  src/dsp/module.cpp
  src/dsp/sine_gen.cpp
  src/dsp/adsr.cpp
  src/dsp/audio_out.cpp
  src/dsp/gain.cpp
  src/dsp/voice_controller.cpp
  src/parser/parser.cpp
  src/vm/vm.cpp
  ${SRC_FILES}
)
target_include_directories(run_tests PRIVATE external/madronalib/Tests)
# Define the path to test data
target_compile_definitions(run_tests PRIVATE "TEST_DATA_DIR=\"${CMAKE_SOURCE_DIR}/examples\"")
# Link madronalib
target_link_libraries(run_tests madronalib)
