#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace madronavm {
// Represents an input that is a fixed constant value.
struct ConstantInput {
  std::string port_name;
  float value;
};
// Represents a single DSP module instance in the graph.
struct Node {
  uint32_t id;
  std::string name; // e.g., "sine_gen"
  std::vector<ConstantInput> constants;
};
// Represents a connection between two nodes.
struct Connection {
  uint32_t from_node_id;
  std::string from_port_name;
  uint32_t to_node_id;
  std::string to_port_name;
};
// The complete, in-memory representation of the patch.
struct PatchGraph {
  std::vector<Node> nodes;
  std::vector<Connection> connections;
};
} // namespace madronavm 