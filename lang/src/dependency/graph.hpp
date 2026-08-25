#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace invariants::dependency {

using GraphNodeId = std::string;

class Graph {
 private:
  std::unordered_map<GraphNodeId, std::unordered_set<GraphNodeId>> adjList;

 public:
  void addNode(std::string_view);
  void addEdge(std::string_view, std::string_view);
  const auto& adjacencyList() const;
};

}  // namespace invariants::dependency