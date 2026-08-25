#include "graph.hpp"

using namespace invariants::dependency;

void Graph::addNode(std::string_view id) {
  adjList.try_emplace(GraphNodeId(id));
}

void Graph::addEdge(std::string_view from, std::string_view to) {
  addNode(from);
  addNode(to);
  adjList[GraphNodeId(from)].insert(GraphNodeId(to));
}

const auto& Graph::adjacencyList() const { return adjList; }