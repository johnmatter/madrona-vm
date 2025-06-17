cmake_minimum_required(VERSION 3.16)
project(madronalib_vm_synth)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add madronalib
add_subdirectory(external/madronalib)

# Include directories
include_directories(include)
include_directories(external/madronalib/include)
include_directories(external/madronalib/source)
include_directories(external/madronalib/source/DSP)
include_directories(external/madronalib/source/app)
include_directories(external/madronalib/external/sse2neon)

# Collect source files
file(GLOB_RECURSE SRC_FILES
  "src/dsp/*.cpp"
)

# Create test executable
add_executable(run_tests
  tests/TestMain.cpp
  tests/dsp/sine_osc_test.cpp
  ${SRC_FILES}
)

target_include_directories(run_tests PRIVATE external/madronalib/Tests)

# Link madronalib
target_link_libraries(run_tests madronalib) 