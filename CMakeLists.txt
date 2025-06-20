cmake_minimum_required(VERSION 3.16)
project(madronalib_vm_synth)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Add madronalib, but don't build its examples
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable building madronalib examples")
add_subdirectory(external/madronalib)
# Add ftxui
add_subdirectory(external/ftxui)
# Include directories
include_directories(include)
include_directories(external/madronalib/include)
include_directories(external/madronalib/source)
include_directories(external/madronalib/source/DSP)
include_directories(external/madronalib/source/app)
include_directories(external/madronalib/source/matrix)
include_directories(external/madronalib/external/sse2neon)
include_directories(external/madronalib/external/cJSON)
include_directories(external/madronalib/external/rtaudio)
include_directories(external/ftxui/include)
# Collect source files
file(GLOB_RECURSE SRC_FILES
  "src/dsp/*.cpp"
  "src/parser/*.cpp"
  "src/compiler/*.cpp"
  "src/vm/*.cpp"
)
# On Apple, we have some extra files for device selection
if(APPLE)
    file(GLOB_RECURSE AUDIO_FILES "src/audio/*.cpp")
else()
    set(AUDIO_FILES "")
endif()
file(GLOB_RECURSE UI_FILES "src/ui/*.cpp")
# Collect test files
file(GLOB_RECURSE UNIT_TEST_FILES "tests/unit/**/*.cpp")
file(GLOB_RECURSE INTEGRATION_TEST_FILES "tests/integration/**/*.cpp")
# Create test executable
add_executable(run_tests
  tests/TestMain.cpp
  ${UNIT_TEST_FILES}
  ${INTEGRATION_TEST_FILES}
  ${SRC_FILES}
  ${AUDIO_FILES}
  ${UI_FILES}
)
target_include_directories(run_tests PRIVATE external/madronalib/Tests)
# Define the path to test data
target_compile_definitions(run_tests PRIVATE "TEST_DATA_DIR=\"${CMAKE_SOURCE_DIR}/examples\"")
target_compile_definitions(run_tests PRIVATE "MODULE_DEFS_PATH=\"${CMAKE_SOURCE_DIR}/data/modules.json\"")
# Link madronalib
target_link_libraries(run_tests madronalib component)
# Create demo executable
add_executable(simple_vm_demo
  examples/simple_vm_demo.cpp
  ${SRC_FILES}
  ${AUDIO_FILES}
  ${UI_FILES}
)
target_include_directories(simple_vm_demo PRIVATE external/madronalib/Tests)
target_compile_definitions(simple_vm_demo PRIVATE "MODULE_DEFS_PATH=\"${CMAKE_SOURCE_DIR}/data/modules.json\"")
target_link_libraries(simple_vm_demo madronalib component)
