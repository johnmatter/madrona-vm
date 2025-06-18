// Patch graph parser
#include "parser/parser.h"
#include "cJSON.h"
#include <stdexcept>
#include <iostream>
#include <string>
namespace madronavm {
// Helper to split "module_id:port_name" strings
static void parse_connection_str(const char* str, uint32_t& node_id, std::string& port_name) {
    std::string full_str(str);
    size_t colon_pos = full_str.find(':');
    if (colon_pos == std::string::npos) {
        throw std::runtime_error("Invalid connection string: " + full_str);
    }
    node_id = std::stoul(full_str.substr(0, colon_pos));
    port_name = full_str.substr(colon_pos + 1);
}
PatchGraph parse_json(std::string_view json_text) {
    PatchGraph graph;
    cJSON* root = cJSON_Parse(json_text.data());
    if (!root) {
        throw std::runtime_error("Failed to parse JSON");
    }
    cJSON* modules = cJSON_GetObjectItem(root, "modules");
    if (modules && modules->type == cJSON_Array) {
        int module_count = cJSON_GetArraySize(modules);
        for (int i = 0; i < module_count; ++i) {
            cJSON* module_item = cJSON_GetArrayItem(modules, i);
            if (!module_item) continue;
            Node node;
            cJSON* id_item = cJSON_GetObjectItem(module_item, "id");
            if (id_item) node.id = id_item->valueint;
            cJSON* name_item = cJSON_GetObjectItem(module_item, "name");
            if (name_item) node.name = name_item->valuestring;
            cJSON* data = cJSON_GetObjectItem(module_item, "data");
            if (data && data->type == cJSON_Object) {
                cJSON* constant_item = data->child;
                while (constant_item) {
                    node.constants.push_back({
                        constant_item->string,
                        (float)constant_item->valuedouble
                    });
                    constant_item = constant_item->next;
                }
            }
            graph.nodes.push_back(node);
        }
    }
    cJSON* connections = cJSON_GetObjectItem(root, "connections");
    if (connections && connections->type == cJSON_Array) {
        int conn_count = cJSON_GetArraySize(connections);
        for (int i = 0; i < conn_count; ++i) {
            cJSON* conn_item = cJSON_GetArrayItem(connections, i);
            if (!conn_item) continue;
            Connection conn;
            const char* from_str = nullptr;
            cJSON* from_item = cJSON_GetObjectItem(conn_item, "from");
            if (from_item) from_str = from_item->valuestring;
            const char* to_str = nullptr;
            cJSON* to_item = cJSON_GetObjectItem(conn_item, "to");
            if(to_item) to_str = to_item->valuestring;
            if (from_str && to_str) {
                parse_connection_str(from_str, conn.from_node_id, conn.from_port_name);
                parse_connection_str(to_str, conn.to_node_id, conn.to_port_name);
                graph.connections.push_back(conn);
            }
        }
    }
    cJSON_Delete(root);
    return graph;
}
} // namespace madronavm
