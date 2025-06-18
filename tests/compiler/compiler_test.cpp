#include "compiler/compiler.h"
#include "parser/parser.h"
#include "catch.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "examples"
#endif
TEST_CASE("Compiler correctly performs topological sort", "[compiler]") {
    std::string path(TEST_DATA_DIR);
    path += "/simple_patch.json";
    std::ifstream t(path);
    if (!t.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        throw std::runtime_error("Could not open test patch file: " + path);
    }
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    auto graph = madronavm::parse_json(str);
    auto sorted_nodes = madronavm::Compiler::topological_sort(graph);
    REQUIRE(sorted_nodes.size() == 3);
    REQUIRE(sorted_nodes[0] == 1);
    REQUIRE(sorted_nodes[1] == 2);
    REQUIRE(sorted_nodes[2] == 3);
}
TEST_CASE("Compiler detects cycles in graph", "[compiler]") {
    madronavm::PatchGraph graph;
    graph.nodes = { {1, "sine_osc", {}}, {2, "gain", {}} };
    graph.connections = {
        {1, "out", 2, "in"},
        {2, "out", 1, "freq"} // Cycle back
    };
    REQUIRE_THROWS(madronavm::Compiler::topological_sort(graph));
} 