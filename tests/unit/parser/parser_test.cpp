#include "parser/parser.h"
#include "catch.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "examples"
#endif
TEST_CASE("Parser correctly parses a simple patch", "[parser]") {
    std::string path(TEST_DATA_DIR);
    path += "/a440.json";
    std::ifstream t(path);
    if (!t.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        throw std::runtime_error("Could not open test patch file: " + path);
    }
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    auto graph = madronavm::parse_json(str);
    REQUIRE(graph.nodes.size() == 3);
    REQUIRE(graph.connections.size() == 3);
    // Check modules
    auto& node1 = graph.nodes[0];
    REQUIRE(node1.id == 1);
    REQUIRE(node1.name == "sine_gen");
    REQUIRE(node1.constants.size() == 1);
    REQUIRE(node1.constants[0].port_name == "freq");
    REQUIRE(node1.constants[0].value == 440.0f);
    auto& node2 = graph.nodes[1];
    REQUIRE(node2.id == 2);
    REQUIRE(node2.name == "gain");
    REQUIRE(node2.constants.size() == 1);
    REQUIRE(node2.constants[0].port_name == "gain");
    REQUIRE(node2.constants[0].value == 0.5f);
    auto& node3 = graph.nodes[2];
    REQUIRE(node3.id == 3);
    REQUIRE(node3.name == "audio_out");
    REQUIRE(node3.constants.empty());
    // Check connections
    auto& conn1 = graph.connections[0];
    REQUIRE(conn1.from_node_id == 1);
    REQUIRE(conn1.from_port_name == "out");
    REQUIRE(conn1.to_node_id == 2);
    REQUIRE(conn1.to_port_name == "in");
    auto& conn2 = graph.connections[1];
    REQUIRE(conn2.from_node_id == 2);
    REQUIRE(conn2.from_port_name == "out");
    REQUIRE(conn2.to_node_id == 3);
    REQUIRE(conn2.to_port_name == "in_l");
    auto& conn3 = graph.connections[2];
    REQUIRE(conn3.from_node_id == 2);
    REQUIRE(conn3.from_port_name == "out");
    REQUIRE(conn3.to_node_id == 3);
    REQUIRE(conn3.to_port_name == "in_r");
}
