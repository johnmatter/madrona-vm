#include "compiler/compiler.h"
#include <map>
#include <stdexcept>
#include "compiler/module_registry.h"
#include "vm/opcodes.h"
#include <cstring>
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
std::vector<uint32_t> Compiler::compile(const PatchGraph& graph) {
    auto sorted_node_ids = topological_sort(graph);
    ModuleRegistry registry;
    std::vector<uint32_t> instructions;
    // Maps a module's output port {node_id, port_name} to a register index.
    std::map<std::pair<uint32_t, std::string>, uint32_t> port_to_reg_map;
    uint32_t next_reg = 0;
    // Create a map of nodes by ID for quick lookups.
    std::map<uint32_t, Node> node_map;
    for(const auto& node : graph.nodes) {
        node_map[node.id] = node;
    }
    for (uint32_t node_id : sorted_node_ids) {
        const auto& node = node_map.at(node_id);
        const auto& module_info = registry.get_info(node.name);
        // --- 1. Handle Constant Inputs ---
        // For each constant, emit a LOAD_K instruction into a new register.
        std::map<std::string, uint32_t> constant_regs;
        for (const auto& constant : node.constants) {
            uint32_t reg = next_reg++;
            constant_regs[constant.port_name] = reg;
            instructions.push_back(static_cast<uint32_t>(OpCode::LOAD_K));
            instructions.push_back(reg);
            uint32_t val_as_u32;
            static_assert(sizeof(float) == sizeof(uint32_t));
            std::memcpy(&val_as_u32, &constant.value, sizeof(val_as_u32));
            instructions.push_back(val_as_u32);
        }
        // --- 2. Prepare for PROC instruction ---
        std::vector<uint32_t> in_regs;
        for (const auto& port_name : module_info.inputs) {
            // Check if the input is a constant for this node.
            if (constant_regs.count(port_name)) {
                in_regs.push_back(constant_regs.at(port_name));
                continue;
            }
            // Otherwise, find the connection that feeds this input port.
            bool found_connection = false;
            for (const auto& conn : graph.connections) {
                if (conn.to_node_id == node.id && conn.to_port_name == port_name) {
                    in_regs.push_back(port_to_reg_map.at({conn.from_node_id, conn.from_port_name}));
                    found_connection = true;
                    break;
                }
            }
            if (!found_connection) {
                 throw std::runtime_error("Unconnected input port: " + node.name + ":" + port_name);
            }
        }
        // Allocate new registers for all of this module's output ports.
        std::vector<uint32_t> out_regs;
        for (const auto& port_name : module_info.outputs) {
            uint32_t reg = next_reg++;
            out_regs.push_back(reg);
            port_to_reg_map[{node.id, port_name}] = reg;
        }
        // --- 3. Emit PROC instruction ---
        instructions.push_back(static_cast<uint32_t>(OpCode::PROC));
        instructions.push_back(registry.get_id(node.name));
        instructions.push_back(in_regs.size());
        instructions.push_back(out_regs.size());
        instructions.insert(instructions.end(), in_regs.begin(), in_regs.end());
        instructions.insert(instructions.end(), out_regs.begin(), out_regs.end());
    }
    instructions.push_back(static_cast<uint32_t>(OpCode::END));
    // --- 4. Prepend Header and return final bytecode ---
    std::vector<uint32_t> final_bytecode;
    BytecodeHeader header;
    header.magic_number = kMagicNumber;
    header.version = kBytecodeVersion;
    header.num_registers = next_reg;
    header.program_size_words = instructions.size() + sizeof(BytecodeHeader) / sizeof(uint32_t);
    final_bytecode.resize(sizeof(BytecodeHeader) / sizeof(uint32_t));
    std::memcpy(final_bytecode.data(), &header, sizeof(header));
    final_bytecode.insert(final_bytecode.end(), instructions.begin(), instructions.end());
    return final_bytecode;
}
} // namespace madronavm 