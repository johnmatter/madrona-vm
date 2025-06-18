#pragma once
#include "parser/patch_graph.h"
#include <string_view>
namespace madronavm {
PatchGraph parse_json(std::string_view json_text);
} // namespace madronavm
