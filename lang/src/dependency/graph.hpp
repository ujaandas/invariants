#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

namespace invariants::dependency {

using GraphNodeId = std::string;

class Graph {
 private:
  std::unordered_map<GraphNodeId, std::vector<GraphNodeId>> adjList;

 public:
  void addNode(std::string_view);
  void addEdge(std::string_view, std::string_view);
};

}  // namespace invariants::dependency