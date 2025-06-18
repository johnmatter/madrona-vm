#pragma once
#include "parser/patch_graph.h"
#include <vector>
namespace madronavm {
class ModuleRegistry; // Forward declaration
class Compiler {
public:
  // Performs a topological sort on the graph and returns the sorted node IDs.
  static std::vector<uint32_t> topological_sort(const PatchGraph& graph);
  // Compiles the patch graph into a bytecode buffer.
  static std::vector<uint32_t> compile(const PatchGraph& graph, const ModuleRegistry& registry);
};
} // namespace madronavm 