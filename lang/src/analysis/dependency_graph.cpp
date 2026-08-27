#include "dependency_graph.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>

namespace invariants::analysis {

DependencyGraph::DependencyGraph(std::size_t num_fields)
    : numFields(num_fields),
      outgoingAdj(num_fields),
      incomingAdj(num_fields),
      inDeg(num_fields, 0) {}

void DependencyGraph::addEdge(FieldId from, FieldId to) {
  if (from >= numFields || to >= numFields) {
    throw std::out_of_range("Field ID out of bounds for dependency graph.");
  }

  if (from == to) {
    throw std::runtime_error(
        "Self-referential dependency detected on field ID: " +
        std::to_string(from));
  }

  // Prevent duplicate edges
  if (hasEdge(from, to)) {
    return;
  }

  outgoingAdj[from].push_back(to);
  incomingAdj[to].push_back(from);
  inDeg[to]++;
}

bool DependencyGraph::hasEdge(binder::FieldId from, binder::FieldId to) const {
  if (from >= numFields || to >= numFields) return false;
  const auto& edges = outgoingAdj[from];
  return std::find(edges.begin(), edges.end(), to) != edges.end();
}

const std::vector<binder::FieldId>& DependencyGraph::getDependencies(
    binder::FieldId field) const {
  if (field >= numFields) {
    throw std::out_of_range("Field ID out of bounds.");
  }
  return incomingAdj[field];
}

const std::vector<binder::FieldId>& DependencyGraph::getDependents(
    binder::FieldId field) const {
  if (field >= numFields) {
    throw std::out_of_range("Field ID out of bounds.");
  }
  return outgoingAdj[field];
}

std::vector<FieldId> DependencyGraph::order() const {
  std::vector<std::size_t> currInDeg = inDeg;
  std::queue<binder::FieldId> zeroInDegQ;

  // Initialize queue with all source nodes (in-degree == 0)
  for (binder::FieldId id = 0; id < numFields; ++id) {
    if (currInDeg[id] == 0) {
      zeroInDegQ.push(id);
    }
  }

  std::vector<binder::FieldId> order;
  order.reserve(numFields);

  // Khan's algo
  while (!zeroInDegQ.empty()) {
    binder::FieldId current = zeroInDegQ.front();
    zeroInDegQ.pop();
    order.push_back(current);

    for (binder::FieldId neighbor : outgoingAdj[current]) {
      if (--currInDeg[neighbor] == 0) {
        zeroInDegQ.push(neighbor);
      }
    }
  }

  // If order does not contain all nodes, at least one cycle
  if (order.size() != numFields) {
    throw std::runtime_error(
        "Cyclic dependency detected in specification invariants.");
  }

  return order;
}

}  // namespace invariants::analysis