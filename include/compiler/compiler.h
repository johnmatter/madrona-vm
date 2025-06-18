#pragma once
#include "parser/patch_graph.h"
#include <vector>
namespace madronavm {
class Compiler {
public:
  // Performs a topological sort on the graph and returns the sorted node IDs.
  static std::vector<uint32_t> topological_sort(const PatchGraph& graph);
};
} // namespace madronavm 