#include "catch.hpp"
#include "vm/vm.h"
#include "compiler/compiler.h"
#include "parser/parser.h"
#include "compiler/module_registry.h"
#include <iostream>
#ifndef MODULE_DEFS_PATH
#define MODULE_DEFS_PATH "data/modules.json"
#endif
using namespace madronavm;
using namespace madronavm::dsp;
TEST_CASE("VM correctly processes math modules", "[vm]") {
    ModuleRegistry registry(MODULE_DEFS_PATH);
    VM vm(registry, 48000, true);
    SECTION("Test add module") {
        const char* json_patch = R"({
            "modules": [
                {"id": 1, "name": "float", "data": {"in": 10.0}},
                {"id": 2, "name": "float", "data": {"in": 20.0}},
                {"id": 3, "name": "add", "data": {}}
            ],
            "connections": [
                {"from": "1:out", "to": "3:in1"},
                {"from": "2:out", "to": "3:in2"}
            ]
        })";
        auto graph = parse_json(json_patch);
        auto bytecode = Compiler::compile(graph, registry);
        vm.load_program(std::move(bytecode));
        vm.processBlock(nullptr, 64);
        // The compiler allocates registers for constants and module outputs.
        // r0: const 10.0
        // r1: output of float(r0)
        // r2: const 20.0
        // r3: output of float(r2)
        // r4: output of add(r1, r3)
        const auto& result_reg = vm.getRegisterForTest(4);
        REQUIRE(result_reg[0] == 30.0f);
    }
    SECTION("Test mul module") {
        const char* json_patch = R"({
            "modules": [
                {"id": 1, "name": "float", "data": {"in": 10.0}},
                {"id": 2, "name": "float", "data": {"in": 20.0}},
                {"id": 3, "name": "mul", "data": {}}
            ],
            "connections": [
                {"from": "1:out", "to": "3:in1"},
                {"from": "2:out", "to": "3:in2"}
            ]
        })";
        auto graph = parse_json(json_patch);
        auto bytecode = Compiler::compile(graph, registry);
        vm.load_program(std::move(bytecode));
        vm.processBlock(nullptr, 64);
        const auto& result_reg = vm.getRegisterForTest(4);
        REQUIRE(result_reg[0] == 200.0f);
    }
    SECTION("Test float module with constant") {
        const char* json_patch = R"({
            "modules": [
                {"id": 1, "name": "float", "data": {"in": 123.45}}
            ],
            "connections": []
        })";
        auto graph = parse_json(json_patch);
        auto bytecode = Compiler::compile(graph, registry);
        vm.load_program(std::move(bytecode));
        vm.processBlock(nullptr, 64);
        // r0: const 123.45
        // r1: output of float(r0)
        const auto& result_reg = vm.getRegisterForTest(1);
        REQUIRE(result_reg[0] == 123.45f);
    }
    SECTION("Test int module with constant") {
        const char* json_patch = R"({
            "modules": [
                {"id": 1, "name": "int", "data": {"in": 99.8}}
            ],
            "connections": []
        })";
        auto graph = parse_json(json_patch);
        auto bytecode = Compiler::compile(graph, registry);
        vm.load_program(std::move(bytecode));
        vm.processBlock(nullptr, 64);
        // r0: const 99.8
        // r1: output of int(r0)
        const auto& result_reg = vm.getRegisterForTest(1);
        REQUIRE(result_reg[0] == 99.0f);
    }
}
TEST_CASE("Float/Int value retention", "[vm][dsp]") {
    // TODO: Implement a test to verify that Float and Int modules retain their
    // value when their input is disconnected. This requires the ability to
    // update the patch graph and re-compile/reload the VM on the fly, which
    // is not yet supported.
    // The test would look something like this:
    // 1. Create a graph with a Float module connected to a constant.
    // 2. Process a block, verify the Float module's output is correct.
    // 3. Create a new graph with the connection removed.
    // 4. Reload the VM with the new graph's bytecode.
    // 5. Process another block.
    // 6. Verify the Float module's output is the *same* as the last block,
    //    demonstrating that it has retained its value.
} 