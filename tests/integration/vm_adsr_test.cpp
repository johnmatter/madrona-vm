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
TEST_CASE("ADSR VM Integration Test", "[vm][adsr]") {
    ModuleRegistry registry(MODULE_DEFS_PATH);
    VM vm(registry, 48000, true);
    SECTION("Test adsr module") {
        const char* json_patch = R"({
            "modules": [
                {"id": 1, "name": "float", "data": {"in": 1.0}},
                {"id": 2, "name": "float", "data": {"in": 0.1}},
                {"id": 3, "name": "float", "data": {"in": 0.2}},
                {"id": 4, "name": "float", "data": {"in": 0.5}},
                {"id": 5, "name": "float", "data": {"in": 0.3}},
                {"id": 6, "name": "adsr", "data": {}}
            ],
            "connections": [
                {"from": "1:out", "to": "6:gate"},
                {"from": "2:out", "to": "6:attack"},
                {"from": "3:out", "to": "6:decay"},
                {"from": "4:out", "to": "6:sustain"},
                {"from": "5:out", "to": "6:release"}
            ]
        })";
        auto graph = parse_json(json_patch);
        auto bytecode = Compiler::compile(graph, registry);
        vm.load_program(std::move(bytecode));
        vm.processBlock(nullptr, 64);
        // After one block with gate high, output should be > 0
        const auto& result_reg = vm.getRegisterForTest(10);
        REQUIRE(result_reg[0] > 0.0f);
    }
} 