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
TEST_CASE("VM correctly processes math modules", "[vm]") {
    ModuleRegistry registry(MODULE_DEFS_PATH);
    VM vm(registry, 48000, true);
    SECTION("Test add module") {
        const char* json_patch = R"({
            "modules": [
                {"id": 1, "name": "float", "data": {"value": 10.0}},
                {"id": 2, "name": "float", "data": {"value": 20.0}},
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
                {"id": 1, "name": "float", "data": {"value": 10.0}},
                {"id": 2, "name": "float", "data": {"value": 20.0}},
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
                {"id": 1, "name": "float", "data": {"value": 123.45}}
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
                {"id": 1, "name": "int", "data": {"value": 99.8}}
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