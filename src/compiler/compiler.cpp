#include "compiler/compiler.h"
#include <map>
#include <stdexcept>
namespace madronavm {
// Implements Kahn's algorithm for topological sorting. This is a standard
// algorithm for finding a linear ordering of the edges of a directed acyclic
// graph (DAG). See:
// https://web.stanford.edu/class/archive/cs/cs106x/cs106x.1192/lectures/Lecture25/Lecture25.pdf
//
// The algorithm works by maintaining a queue of nodes with an in-degree of zero.
// It then processes nodes from the queue, decrementing the in-degree of each
// neighbor. If a neighbor's in-degree becomes zero, it is added to the queue.
// This process continues until the queue is empty.
//
std::vector<uint32_t> Compiler::topological_sort(const PatchGraph& graph) {
    std::vector<uint32_t> sorted_nodes; // The list of sorted node IDs.
    std::map<uint32_t, int> in_degree;      // Stores the in-degree of each node.
    // Adjacency list representation of the graph.
    std::map<uint32_t, std::vector<uint32_t>> adj;
    // Queue for nodes with an in-degree of zero.
    std::vector<uint32_t> queue;
    // Initialize in-degree for all nodes to 0.
    for (const auto& node : graph.nodes) {
        in_degree[node.id] = 0;
    }
    // Build the adjacency list and calculate in-degrees from connections.
    for (const auto& conn : graph.connections) {
        adj[conn.from_node_id].push_back(conn.to_node_id);
        in_degree[conn.to_node_id]++;
    }
    // Enqueue all nodes with an initial in-degree of 0.
    // These are the starting points of the graph.
    for (const auto& pair : in_degree) {
        if (pair.second == 0) {
            queue.push_back(pair.first);
        }
    }
    // Process nodes from the queue.
    while (!queue.empty()) {
        uint32_t u = queue.front();
        queue.erase(queue.begin());
        sorted_nodes.push_back(u);
        // For the current node, go through all its neighbors.
        if (adj.count(u)) {
            for (uint32_t v : adj[u]) {
                // Decrement the in-degree of each neighbor.
                in_degree[v]--;
                // If a neighbor's in-degree becomes 0, it's ready to be processed.
                if (in_degree[v] == 0) {
                    queue.push_back(v);
                }
            }
        }
    }
    // If the number of sorted nodes is not equal to the total number of nodes,
    // the graph must contain a cycle.
    if (sorted_nodes.size() != graph.nodes.size()) {
        throw std::runtime_error("Graph contains a cycle, cannot sort topologically.");
    }
    return sorted_nodes;
}
} // namespace madronavm 